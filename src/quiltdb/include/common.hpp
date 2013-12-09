#ifndef __QUILT_COMMON_HPP__
#define __QUILT_COMMON_HPP__

#include <stdint.h>
#include <string>

// a set of data structures that are shared among varios components
typedef int (*ValueAddFunc)(uint8_t *, uint8_t *, int32_t);
typedef int (*ValueSubFunc)(uint8_t *, uint8_t *, int32_t);

typedef void (UpdateBufferCbk*)(int32_t _table_id, 
				UpdateBuffer *_update_buffer);

struct NodeInfo{
  int32_t node_id_;
  std::string recv_pull_ip_;
  std::string recv_pull_port_;
  std::string recv_push_ip_;
  std::string recv_push_port_;
};

enum UpdateType{EInc};

#endif
