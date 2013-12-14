#pragma once
#include <stdint.h>
#include <fstream>
#include <math>
#include "load_bin.h"
#include "range.h"

namespace quiltdb {

template <class T>
std::vector<T> Linspace(T start, T end, int k) {
  double itv = (double)(end-start) / (double) k;
  std::vector<T> ret;
  for (int i = 0; i <= k; i++) {
    ret.push_back((T)std::round(itv*(double)i) + start);
  }
  return ret;
}

typedef Range<uint64_t> Block;
using std::string;

// row majored sparse matrix
template<typename T>
class SMP {
 public:
  GSMP() : row_os(NULL), col_ix(NULL) { }
  ~GSMP() { delete [] row_os; delete [] col_ix; }

  void Read(const string& name);
  // all public, just make life easier
  Block row;
  Block col;
  uint64_t row_num, col_num, nnz;
  T* row_os;
  T* col_ix;
};

template<typename T>
void SMP<T> Read(const string& name) {
  // load data
  uint64_t nrows = load_bin<T>(name+".rowcnt", &row_os);
  nnz = load_bin<T>(name+".colidx", &col_ix);

  uint64_t tmp1, tmp2;
  std::ifstream in(name+".range");
  CHECK(in.good());
  in >> row.start() >> row.end() >> col.start() >> col.end();
  CHECK(row.Valid());
  CHECK(col.Valid());
  row_num = row.Size();
  col_num = col.Size();
  CHECK_EQ(nrows-1, row_num);
}



} // namespace quiltdb
