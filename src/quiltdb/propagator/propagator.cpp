#include "propagator.hpp"


namespace quiltdb {

Propagator::Propagator(){}

int Propagator::Start(PropagatorConfig &_config, sem_t *sync_sem){
  return 0;
}

int Propagator::RegisterTable(int32_t _table_id){
  return 0;
}

int Propagator::Inc(int32_t _table_id, int32_t _key, const uint8_t *_delta, 
		    int32_t _num_bytes){
  return 0;
}

int Propagator::ApplyUpdates(int32_t _table_id, UpdateBuffer *_updates){
  return 0;
}

int Propagator::Stop(){
  return 0;
}


}
