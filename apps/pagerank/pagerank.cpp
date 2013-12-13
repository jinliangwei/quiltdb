#include "pagerank.h"
#include "rspmat.h"
#include <vector>
#include "range.h"
#include <gflags/gflags.h>

using namespace quiltdb;

// DEFINE_string(data_input, "/media/muli/data/webgraph-subdomain/bin/part-", "");
DEFINE_string(data_input, "data/stanford-", "");
DEFINE_string(data_output, "../data/subdomain", "");
DEFINE_int32(data_part_num, 1, "");
DEFINE_int32(grid_row_num, 1, "");
DEFINE_int32(grid_col_num, 1, "");

DEFINE_int32(my_row_rank, 0, "");
DEFINE_int32(my_col_rank, 0, "");
DEFINE_double(alpha, .8, "");

using std::string;
typedef Range<uint64_t> Block;
typedef std::vector<std::vector<string> > NodeFiles;
typedef std::vector<Block> Blocks;

//// eigen3
#include <eigen3/Eigen/Dense>
typedef Eigen::Matrix<double, Eigen::Dynamic, 1> DVec;

#define LL LOG(ERROR)

// partitioning the data
void AssignData(NodeFiles *node_files, Blocks *col_blocks) {
  std::vector<string> files;
  char tmp[128];
  for (int i = 0; i < FLAGS_data_part_num; ++i) {
    snprintf(tmp, 128, "%03d", i);
    files.push_back(FLAGS_data_input + string(tmp));
  }

  // assign files into machines
  std::vector<int> parts = Linspace<int>(0, FLAGS_data_part_num, FLAGS_grid_row_num);
  for (int i = 0; i < FLAGS_grid_row_num; i++) {
    std::vector<string> names;
    for (int j = parts[i]; j < parts[i+1]; ++j)
      names.push_back(files[j]);
    node_files->push_back(names);
  }

  // get the column range
  Block colseg = RSpMat<>::ColSeg(files[0]);
  std::vector<size_t> cols =
      Linspace<size_t>(colseg.start(), colseg.end(), FLAGS_grid_col_num);
  for (int i = 0; i < FLAGS_grid_col_num; ++i) {
    col_blocks->push_back(Block(cols[i], cols[i+1]));
  }
}

//   ss << FLAGS_data_output << "_" << FLAGS_my_row_rank << "_"
//      << FLAGS_my_col_rank << "_" << i;


#include <quiltdb/include/quiltdb.hpp>

DEFINE_string(config_file, "", "configuration file");
DEFINE_int32(myhid, 0, "my h id");
DEFINE_int32(myvid, 1, "my v id");

int main(int argc, char *argv[]) {


  // preprocess data
  NodeFiles node_files;
  std::vector<Block> col_blocks;
  // std::vector<string> in_files;
  AssignData(&node_files, &col_blocks);
  // PreprocessData(node_files, col_blocks, &in_files);

  std::vector<string> in_files = node_files[FLAGS_my_row_rank];
  int nf = in_files.size();

  Block col = col_blocks[FLAGS_my_col_rank];

  RSpMat<size_t> tmp;
  RSpMat<uint32_t> *adjs = new RSpMat<uint32_t>[nf];
  for (int i = 0; i < nf; ++i) {
    // load data
    tmp.Load(in_files[i]);
    tmp.VSlice(col, adjs+i);
  }


  // do actual computing
  // w = alpha * X * w + (1-alpha)*1/n
  // TODO
  CHECK_EQ(nf, 1);
  CHECK(adjs[0].square());

    int f = 0;

    uint32_t* index = adjs[f].index();
    size_t* offset = adjs[f].offset();
    double penalty = (1 - FLAGS_alpha) / (double) adjs[f].rows();

    DVec w = DVec::Ones(col.size()) * penalty;
    DVec u(adjs[f].rows());

  for (int it = 0; it < 40; ++it) {
    for (size_t i = 0; i < adjs[f].rows(); ++i) {
      double v = 0;
      double degree = offset[i+1] - offset[i];
      for (uint32_t j = offset[i]; j < offset[i+1]; ++j) {
        v += w[index[j]] / degree;
      }
      u[i] = v*FLAGS_alpha + penalty;
    }
    LL << "iter " << it << " err " << (u-w).norm() / w.norm()
       << " 1-norm " << w.cwiseAbs().sum();
    w = u;
  }

  return 0;
}
