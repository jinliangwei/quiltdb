#ifndef __QUILTDB_RECEIVER_HPP__
#define __QUILTDB_RECEIVER_HPP__

#include <quiltdb/include/commom.hpp>
#include <quiltdb/internal_table/internal_table.hpp>

namespace quiltdb {

struct ReceiverConfig{
  NodeInfo upstream_;
  zmq::context_t zmq_ctx_;
};

class Receiver {

public:
  Receiver();
  int Init(ReceiverConfig &_config);
  int RegisterTable(InternalTable *_itable);
  int StoreUpdates(int32_t _table_id, UpdateBuffer *_updates);

};

}

#endif
