#ifndef __QUILTDB_RECEIVER_HPP__
#define __QUILTDB_RECEIVER_HPP__

#include <quiltdb/include/common.hpp>
#include <quiltdb/internal_table/internal_table.hpp>

#include <semaphore.h>
#include <zmq.hpp>

namespace quiltdb {

struct ReceiverConfig{
  //NodeInfo upstream_;
  zmq::context_t zmq_ctx_;
};

class Receiver {

public:
  Receiver();
  int Start(ReceiverConfig &_config, sem_t *sync_sem);
  int RegisterTable(InternalTable *_itable);
  // These are updates that has been sent out, which should be subtracted from 
  // received updates.
  int CommitUpdates(int32_t _table_id, UpdateBuffer *_updates);

};

}

#endif
