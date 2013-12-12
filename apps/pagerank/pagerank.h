#pragma once
#include <math.h>
#include <fstream>
#include <stdint.h>
#include "bin_data.h"
#include "range.h"

namespace quiltdb {

template <class T>
std::vector<T> Linspace(T start, T end, int k) {
  double itv = (double)(end-start) / (double) k;
  std::vector<T> ret;
  for (int i = 0; i <= k; i++) {
    ret.push_back((T)round(itv*(double)i) + start);
  }
  return ret;
}




} // namespace quiltdb
