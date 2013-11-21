#ifndef __QUILTDB_INTERNAL_TABLE_HPP__
#define __QUILTDB_INTERNAL_TABLE_HPP__

#include <stdint.h>
#include <tbb/concurrent_hash_map.h>
#include <quiltdb/propagator/propagator.hpp>
#include <quiltdb/receiver/receiver.hpp>

namespace quiltdb {

typedef int (*ValueAddFunc)(uint8_t *, uint8_t *, int32_t);
typedef int (*ValueSubFunc*)(uint8_t *, uint8_t *, int32_t);

struct TableConfig{

  int32_t vsize_;

  ValueAddFunc vadd_func_;
  ValueSubFunc vsub_func_;
  
};

class InternalTable {
public:

  template<typename ValueType>
  ValueType Get(int64_t _key);
  
  // do vadd_func_(value, _delta);
  template<typename ValueType>
  void Inc(ValueType _delta);

  InternalTable(TableConfig &_table_config);
  
  // called by Receiver to apply a set of updates
  int ApplyUpdates(uint8_t *_updates, int32_t _num_bytes);

  // called by Receiver to remove my own updates
  // do vsub_func(_v1, _v2);
  int Sub(uint8_t *_v1, const uint8_t* _v2, int32_t _num_bytes);

  // called by Aggretator
  // do vadd_func(_v1, _v2);
  // must _num_bytes == sizeof(ValueType)
  int Add(uint8_t *_v1, const uint8_t* _v2, int32_t _num_bytes);  

  int32_t table_id_;
  int32_t vsize_;
  ValueAddFunc vadd_func_;
  ValueSubFunc vsub_func_;

  tbb::concurrent_hash_map<int64_t, uint8_t* > storage_;

};

}

#endif
