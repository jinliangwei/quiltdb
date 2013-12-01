#ifndef __QUILTDB_INTERNAL_TABLE_HPP__
#define __QUILTDB_INTERNAL_TABLE_HPP__

#include <stdint.h>
#include <tbb/concurrent_hash_map.h>
#include <string.h>
#include <glog/logging.h>
#include <iostream>

#include <quiltdb/utils/memstruct.hpp>
#include <quiltdb/include/common.hpp>
#include <quiltdb/comm/propagator.hpp>

namespace quiltdb {

struct TableConfig{

  int32_t vsize_;

  ValueAddFunc vadd_func_;
  ValueSubFunc vsub_func_;
};

class InternalTable {
public:

  InternalTable(int32_t _table_id, const TableConfig &_table_config);
  ~InternalTable();

  template<typename ValueType>
  ValueType Get(int64_t _key);
  
  // do vadd_func_(value, _delta);
  template<typename ValueType>
  void Inc(int64_t _key, ValueType _delta);

  int32_t GetID();
  
  // called by Receiver to apply a set of updates
  int ApplyUpdates(UpdateBuffer *_updates, int32_t _num_bytes);

  ValueAddFunc get_vadd_func(){
    return vadd_func_;
  }
  ValueSubFunc get_vsub_func(){
    return vsub_func_;
  }

  void set_propagator(Propagator *_prop){
    propagator = _prop;
  }

private:
  int32_t table_id_;
  int32_t vsize_;
  ValueAddFunc vadd_func_;
  ValueSubFunc vsub_func_;
  
  uint8_t *default_v_;
  tbb::concurrent_hash_map<int64_t, uint8_t*> storage_;
  
  Propagator *propagator;
};

template<typename ValueType>
ValueType InternalTable::Get(int64_t _key){
  
  tbb::concurrent_hash_map<int64_t, uint8_t*>::const_accessor
    value_acc;
  
  if(storage_.find(value_acc, _key)){
    return *(reinterpret_cast<ValueType *>(value_acc->second));
  }

  // not found the key, create it
  tbb::concurrent_hash_map<int64_t, uint8_t*>::accessor
    insert_value_acc;
  
  // Someone might have gained the lock and inserted the key.
  if(storage_.insert(insert_value_acc, _key)){
    insert_value_acc->second = new uint8_t[sizeof(ValueType)];
    memset(insert_value_acc->second, 0, sizeof(ValueType));
  }
  
  return *(reinterpret_cast<ValueType *>(insert_value_acc->second));

}

template<typename ValueType>
void InternalTable::Inc(int64_t _key, ValueType _delta){
  
  tbb::concurrent_hash_map<int64_t, uint8_t* >::accessor
    value_acc;
    
  // if the key does not exist, insert it and initialize the value;
  // if it exists, that takes no effect

  if(storage_.insert(value_acc, _key)){
    value_acc->second = new uint8_t[sizeof(ValueType)];
    memset(value_acc->second, 0, sizeof(ValueType));
  }
  uint8_t *value_ptr = value_acc->second;
  uint8_t *delta_ptr = reinterpret_cast<uint8_t*>(&_delta);
  
  vadd_func_(value_ptr, delta_ptr, vsize_);

  propagator->Inc(table_id_, _key, (uint8_t *) &_delta, sizeof(ValueType));
}

}

#endif
