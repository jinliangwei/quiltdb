#ifndef __QUILT_COMMON_HPP__
#define __QUILT_COMMON_HPP__

#include <stdint.h>
#include <string>

// a set of data structures that are shared among varios components

struct NodeInfo{
  
  int32_t node_id_;
  std::string node_ip_;
  std::string port_;

};

enum UpdateType{EInc};

#endif
