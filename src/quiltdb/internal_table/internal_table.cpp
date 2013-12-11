#include "internal_table.hpp"

namespace quiltdb {

InternalTable::InternalTable(int32_t _table_id, const TableConfig &_table_config):
  table_id_(_table_id),
  vsize_(_table_config.vsize_),
  vadd_func_(_table_config.vadd_func_),
  vsub_func_(_table_config.vsub_func_),
  loop_(_table_config.loop_),
  apply_updates_(_table_config.apply_updates_),
  forward_updates_(_table_config.forward_updates_),
  do_user_cbk_(_table_config.do_user_cbk_){}

// This method is not concurrent.
InternalTable::~InternalTable(){
  tbb::concurrent_hash_map<int64_t, uint8_t*>::iterator del_acc;
  for(del_acc = storage_.begin(); del_acc != storage_.end(); del_acc++){
    delete[] del_acc->second;
    del_acc->second = 0;
  } 
}

int32_t InternalTable::GetID(){
  return table_id_;
}  

int InternalTable::ApplyUpdates(UpdateBuffer *_udpates, int32_t _num_bytes){
  return 0;
}

void InternalTable::IncRaw(int64_t _key, uint8_t *_delta){

  tbb::concurrent_hash_map<int64_t, uint8_t* >::accessor
    value_acc;
    
  // if the key does not exist, insert it and initialize the value;
  // if it exists, that takes no effect

  if(storage_.insert(value_acc, _key)){
    value_acc->second = new uint8_t[vsize_];
    memset(value_acc->second, 0, vsize_);
  }
  uint8_t *value_ptr = value_acc->second;
  vadd_func_(value_ptr, _delta, vsize_);

}
}
