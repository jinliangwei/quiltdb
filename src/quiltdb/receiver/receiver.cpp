#include "receiver.hpp"

namespace quiltdb {

Receiver::Receiver(){}

int Receiver::Start(ReceiverConfig &_config, sem_t *sync_sem){
  return 0;
}

int Receiver::RegisterTable(InternalTable *_itable){
  return 0;
}

int Receiver::CommitUpdates(int32_t _table_id, UpdateBuffer *_updates){
  return 0;
}

int Receiver::SignalTerm(){
  return 0;
}

int Receiver::WaitTerm(){
  return 0;
}
}
