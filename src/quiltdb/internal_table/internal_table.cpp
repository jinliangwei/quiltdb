#include "internal_table.hpp"

namespace quiltdb {

InternalTable::InternalTable(int32_t _table_id, const TableConfig &_table_config):
  table_id_(_table_id),
  vsize_(_table_config.vsize_),
  vadd_func_(_table_config.vadd_func_),
  vsub_func_(_table_config.vsub_func_){}

// This method is not concurrent.
InternalTable::~InternalTable(){
  tbb::concurrent_hash_map<int64_t, uint8_t*>::iterator del_acc;
  for(del_acc = storage_.begin(); del_acc != storage_.end(); del_acc++){
    delete[] del_acc->second;
    storage_.erase(del_acc->first);
  } 
}

int32_t InternalTable::GetID(){
  return table_id_;
}  

int InternalTable::ApplyUpdates(UpdateBuffer *_udpates, int32_t _num_bytes){
  return 0;
}

int InternalTable::Sub(uint8_t * _v, const uint8_t* _delta, int32_t _num_bytes){
  return 0;
}

int InternalTable::Add(uint8_t *_v, const uint8_t* _v2, int32_t _num_bytes){
  return 0;
}

}