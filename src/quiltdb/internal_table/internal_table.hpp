#ifndef __QUILTDB_INTERNAL_TABLE_HPP__
#define __QUILTDB_INTERNAL_TABLE_HPP__

#include <stdint.h>
#include <tbb/concurrent_hash_map.h>
#include <boost/shared_array.hpp>
#include <quiltdb/utils/memstruct.hpp>

namespace quiltdb {

typedef int (*ValueAddFunc)(uint8_t *, uint8_t *, int32_t);
typedef int (*ValueSubFunc)(uint8_t *, uint8_t *, int32_t);

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
  void Inc(int64_t _key, ValueType _delta);

  int32_t GetID();

  InternalTable(int32_t _table_id, const TableConfig &_table_config);
  
  // called by Receiver to apply a set of updates
  int ApplyUpdates(UpdateBuffer *_updates, int32_t _num_bytes);

  // called by Receiver to remove my own updates
  // do vsub_func(_v, _delta);
  int Sub(uint8_t * _v, const uint8_t* _delta, int32_t _num_bytes);

  // called by Aggretator
  // do vadd_func(_delta, _delta);
  // must _num_bytes == sizeof(ValueType)
  int Add(uint8_t *_v, const uint8_t* _v2, int32_t _num_bytes);  

  int32_t table_id_;
  int32_t vsize_;
  ValueAddFunc vadd_func_;
  ValueSubFunc vsub_func_;

  tbb::concurrent_hash_map<int64_t, boost::shared_array<uint8_t> > storage_;

};

template<typename ValueType>
ValueType InternalTable::Get(int64_t _key){
  
  tbb::concurrent_hash_map<int64_t, boost::shared_array<uint8_t> >::const_accessor
    value_acc;
  
  if(storage_.find(value_acc, _key)){
    return *(reinterpret_cast<ValueType *>(value_acc->second.get()));
  }

  // not found the key, create it
  tbb::concurrent_hash_map<int64_t, boost::shared_array<uint8_t> >::accessor
    insert_value_acc;
  
  // Someone might have gained the lock and inserted the key.
  if(storage_.insert(insert_value_acc, _key)){    
    (insert_value_acc->second).reset(
				 reinterpret_cast<uint8_t *>(new ValueType()));
  }
  
  return *(reinterpret_cast<ValueType *>(insert_value_acc->second.get()));

}

template<typename ValueType>
void InternalTable::Inc(int64_t _key, ValueType _delta){
  
  tbb::concurrent_hash_map<int64_t, boost::shared_array<uint8_t> >::accessor
    value_acc;
    
  // if the key does not exist, insert it and initialize the value;
  // if it exists, that takes no effect

  if(storage_.insert(value_acc, _key)){
    value_acc->second.reset(
			     reinterpret_cast<uint8_t *>(new ValueType()));
  }
  uint8_t *value_ptr = (value_acc->second).get();
  uint8_t *delta_ptr = reinterpret_cast<uint8_t *>(&_delta);
  
  vadd_func_(value_ptr, delta_ptr, vsize_);
}

}

#endif
