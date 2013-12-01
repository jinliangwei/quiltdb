#ifndef __QUILTDB_RECEIVER_HPP__
#define __QUILTDB_RECEIVER_HPP__

#include <quiltdb/include/common.hpp>
#include <quiltdb/internal_table/internal_table.hpp>

#include <semaphore.h>
#include <zmq.hpp>

namespace quiltdb {

struct ReceiverConfig{
  NodeInfo upstream_;
  zmq::context_t zmq_ctx_;
  std::string update_push_endp_;
  std::string recv_pull_endp_;
};

class Receiver {

  enum ReceiverState{INIT, RUN, TERM_SELF, TERM};

  /*
   * Receiver state transition and termination logic
   *
   * Receiver must process all the queued messages and notify its upstream 
   * propagator to stop sending messages before it terminates. The receiver 
   * thread cannot exit until it gets permission from its upstream propagator.
   * 
   * Propagator State Transition Diagram:
   *
   * INIT --> RUN --> TERM_SELF --> TERM
   *
   * Meaning of different states:
   * INIT: created but not yet started, allows initialization (register tables)
   * RUN: running, accepts updates from upstream propagator and apply them
   *      to shared table
   * TERM_SELF: application threads acknowledge that they do not expect any more
   *            updates
   * TERM: terminated, no more operations
   *
   * Transition triggers:
   * INIT --> RUN: Start()
   * RUN --> TERM_SELF: SignalTerm()
   * TERM_SELF --> TERM: received termination confirmation
   * 
   * Allowed operations:
   *
   * INIT: RegisterTable(), Start()
   * RUN: CommitUpdates(), SignalTerm(), GetErrCode()
   * TERM_SELF: CommitUpdates(), WaitTerm(), GetErrCode()
   * TERM: GetErrCode()
   */

public:
  Receiver();
  int Start(ReceiverConfig &_config, sem_t *sync_sem);
  int RegisterTable(InternalTable *_itable);
  int GetErrCode();
  int SignalTerm();
  int WaitTerm();

private:
  int PropagateUpdates(int32_t _table_id, UpdateBuffer *_updates);

};

}

#endif
