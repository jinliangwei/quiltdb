#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include "bin_data.h"
#include "range.h"
// #include "util/common.h"
// #include "util/key.h"
// #include "util/xarray.h"
// #include "util/eigen3.h"

// DISALLOW_COPY_AND_ASSIGN disallows the copy and operator= functions.
// It goes in the private: declarations in a class.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

namespace quiltdb {

using std::string;

typedef Range<size_t> Seg;
// row-majored sparse matrix. Usually it is a block of a large sparse matrix.
template<typename I = size_t, typename V = double>
class RSpMat {
 public:
  static Seg RowSeg(const string& name);
  static Seg ColSeg(const string& name);

  RSpMat() : offset_(NULL), index_(NULL), value_(NULL) { }
  ~RSpMat() { Clear(); }

  // size
  size_t rows() { return row_.size(); }
  size_t cols() { return col_.size(); }
  size_t nnz() { return nnz_; }
  // property
  bool has_value() { return value_ != NULL; }
  bool square() { return col_ == row_; }
  // access data
  I* index() { return index_; }
  size_t *offset() { return offset_; }
  V* value() { return value_; }
  // read from binary files
  // name.size : plain text
  // name.rowcnt : size_t format
  // name.colidx : I format
  // name.value : V format
  void Load(const string& name, Seg row = Seg::All());
  void Clear();
  void Init(Seg row, Seg col, size_t nnz, bool has_value);

  // TODO use template
  // void ToEigen3(DSMat *mat);
 template<typename I2>
 void VSlice(const Seg& col, RSpMat<I2,V> *res);
 private:
  DISALLOW_COPY_AND_ASSIGN(RSpMat);
  // the row and column range in the global matrix
  Seg row_;
  Seg col_;
  // number of rows, columns, non-zero entries
  // size_t rows_, cols_,
  size_t nnz_;

  // TODO use unique_ptr or shared_ptr
  // row offset
  size_t* offset_;
  // column index
  I* index_;
  // values
  V* value_;

  // empty in default. will be filled value if
  // XArray<Key> global_key_;
};

template<typename I, typename V>
Seg RSpMat<I,V>::RowSeg(const string& name) {
  std::ifstream in(name+".size");
  CHECK(in.good()) << "open " << name << ".size failed.";
  Seg row;
  in >> row.start() >> row.end();
  CHECK(row.Valid()) << "invalid row range " << row.ToString();
  return row;
}

template<typename I, typename V>
Seg RSpMat<I,V>::ColSeg(const string& name) {
  std::ifstream in(name+".size");
  CHECK(in.good()) << "open " << name << ".size failed.";
  Seg row, col;
  in >> row.start() >> row.end() >> col.start() >> col.end();
  CHECK(col.Valid()) << "invalid column range " << col.ToString();;
  return col;
}

template<typename I, typename V>
void RSpMat<I,V>::Clear()  {
  delete [] value_;
  delete [] offset_;
  delete [] index_;
  value_ = NULL;
  offset_ = NULL;
  index_ = NULL;
  row_ = Seg::Invalid();
  col_ = Seg::Invalid();
  nnz_ = 0;
}

template<typename I, typename V>
void RSpMat<I,V>::Init(Seg row, Seg col, size_t nnz, bool has_value) {
  Clear();
  row_ = row;
  col_ = col;
  nnz_ = nnz;
  offset_ = new size_t[rows() + 5];
  memset(offset_, 0, sizeof(size_t)*rows());
  index_ = new I[nnz_ + 5];
  if (has_value)
    value_ = new V[nnz_ + 5];
}
template<typename I, typename V>
void RSpMat<I,V>::Load(const string& name, Seg row)  {
  Clear();

  // read matrix size
  auto all = RowSeg(name);
  if (row == Seg::All()) {
    row_ = all;
  } else {
    CHECK(row.SubsetEq(all)) << "try to read rows " << row.ToString()
                       << " from data with rows " << all.ToString();
    row_ = row;
  }
  col_ = ColSeg(name);

  // load row offset
  size_t total_rows = bin_length<size_t>(name+".rowcnt") - 1;
  CHECK_EQ(total_rows, all.size());
  load_bin<size_t>(name+".rowcnt", &offset_, row_.start(), rows()+1);

  // load column index
  nnz_ = load_bin<I>(name+".colidx", &index_, offset_[0], offset_[rows()]);

  // load values if any
  size_t nval = bin_length<V>(name+".value");
  if (nval != 0) {
    load_bin<V>(name+".value", &value_, offset_[0], offset_[rows()]);
  }

  // shift the offset if necessary
  size_t start = offset_[0];
  if (start != 0) {
    for (size_t i = 0; i <= rows(); ++i) {
      offset_[i] -= start;
    }
  }
}

// slice a column block
template<typename I, typename V>
template<typename I2>
void RSpMat<I,V>::VSlice(const Seg& col, RSpMat<I2,V> *res) {
  CHECK(col_.SupsetEq(col));
  res->Init(row_, col, nnz(), has_value());

  for (size_t i = 0; i < rows(); ++i) {
    size_t o = res->offset()[i];
    for (size_t j = offset_[i]; j < offset_[i+1]; ++j) {
      I c = index_[j];
      if (col.In(c)) {
        res->index()[o] = (I2) (c - col.start());
        if (has_value())
          res->value()[o] = value_[j];
        ++ o;
      }
    }
    res->offset()[i+1] = o;
  }
}


// template<typename I, typename V>
// void RSpMat<I,V>::ToEigen3(DSMat *mat) {
//   IVec reserve(rows_);
//   for (size_t i = 0; i < rows_; ++i) {
//     reserve[i] = (int) (offset_[i+1] - offset_[i]);
//   }
//   mat->resize(rows_, cols_);
//   mat->reserve(reserve);
//   for (size_t i = 0; i < rows_; ++i) {
//     for (size_t j = offset_[i]; j < offset_[i+1]; ++j) {
//       mat->insert(i, index_[j] - col_.start()) =
//           value_ == NULL ? 1 : value_[j];
//     }
//   }
//   mat->makeCompressed();
// }

} // namespace
