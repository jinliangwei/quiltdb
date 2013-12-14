#include "pagerank.h"
#include <gflags/gflags.h>

DEFINE_string(data_input, "/media/muli/data/webgraph-subdomain/bin/part-", "");
DEFINE_string(data_output, "../data/subdomain", "");
DEFINE_int32(data_part_num, 8, "");
DEFINE_int32(grid_row_num, 2, "");
DEFINE_int32(grid_col_num, 2, "");

DEFINE_int32(my_row_rank, 0, "");
DEFINE_int32(my_col_rank, 0, "");

typedef std::vector<std::vector<string> > NodeFiles;
typedef std::vector<Block> Blocks;

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
  std::ifstream in(files[0]+".range");
  size_t tmp;
  in >> tmp >> tmp >> tmp >> tmp;
  std::vector<size_t> cols = Linspace<size_t>(0, tmp, FLAGS_grid_col_num);
  for (int i = 0; i < FLAGS_grid_col_num; ++i) {
    col_blocks->push_back(Block(cols[i], cols[i+1]));
  }
}

// slice a column block from X, and store it by uint_32
void Slice(const SMP<uint64_t>& X, const Block& col, SMP<uint32_t> *Y) {
  CHECK(X.col.Limit(col) == col);
  delete [] Y->row_os;
  delete [] Y->col_ix;
  Y->row = X.row;
  Y->col = col;
  CHECK_GE(X.row_os[0], X.row.start());
  Y->row_os = new uint32_t[X.row_num+5];
  Y->row_os[0] = 0;
  Y->col_ix = new uint32_t[X.nnz];
  for (size_t i = 0; i < X.row_num; ++i) {
    Y->row_os[i+1] = X.row_os[i];
    for (size_t j = X.row_os[i]; j < X.row_os[i+1]; ++j) {
      int64_t c = X.col_ix[j];
      if (col.In(c)) {
        Y->col_ix[Y->row_os[i+1]++] = c - col.start();
      }
    }
  }
}

// extract the data needed by this machine, and store it
void PreprocessData(const NodeFiles& node_files, const Blocks& col_blocks
                    std::vector<string> *out_files) {
  // output names
  std::vector<string> in_files = node_files[FLAGS_my_row_rank];
  int n = in_files.size();

  ;
  for (int i = 0; i < n; ++i) {
    std::ostringstream ss;
    ss << FLAGS_data_output << "_" << FLAGS_my_row_rank << "_"
       << FLAGS_my_col_rank << "_" << i;
    out_files->push_back(ss.string());
  }
  // check if output already exists
  bool cached = true;
  for (int i = 0; i < n; ++i) {
    struct stat file_stat;
    if (!stat(*out_files[i], &file_stat))
      cached = false;
  }
  // TODO it is better to check the range
  if (cached)
    return;
  // do convert
  for (int i = 0; i < n; ++i) {
    SMP<uint64_t> X;
    X.Read(in_files[i]);
    SMP<uint32_t> Y;
    Slice(X, col_blocks[FLAGS_my_col_rank], &Y);
    Y.Write(*out_files[i]);
  }
}

int main(int argc, char *argv[]) {

  // some configuration, will use gflag later..

  // preprocess data
  NodeFiles node_files;
  std::vector<Block> col_blocks;
  std::vector<string> in_files;
  AssignData(&node_files, &col_blocks);
  PreprocessData(node_files, col_blocks, &in_files);


  // do actual computing



  return 0;
}
