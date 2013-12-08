#ifndef __QUILTDB_RECEIVER_HPP__
#define __QUILTDB_RECEIVER_HPP__

#include <quiltdb/include/common.hpp>
#include <quiltdb/internal_table/internal_table.hpp>

#include <semaphore.h>
#include <zmq.hpp>
#include <pthread.h>
#include <boost/unordered_map.hpp>
#include <boost/thread/tss.hpp>

namespace quiltdb {

struct ReceiverConfig{
  int32_t my_id_;
  NodeInfo my_info_;
  int32_t num_expected_propagators_;

  zmq::context_t *zmq_ctx_;
  std::string update_push_endp_; // receive my own updates from internal 
                                 // propagator pair
  std::string internal_recv_pull_endp_;
  std::string internal_pair_recv_push_endp_;

};

class Receiver {

  enum ReceiverState{INIT, RUN, TERM_SELF, TERM};

  struct ReceiverThrInfo {
    int32_t my_id_;
    NodeInfo my_info_;
    int32_t num_expected_propagators_;

    zmq::context_t *zmq_ctx_;
    std::string update_push_endp_;
    std::string internal_recv_pull_endp_;
    std::string internal_pair_recv_push_endp_;

    sem_t *sync_sem_;

    Receiver *receiver_ptr_;
  };

  struct PeerPropagatorInfo {
    bool has_started_;
    bool has_replied_termination_;
    
    PeerPropagatorInfo():
      has_started_(false),
      has_replied_termination_(false){}
  };

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

  /*
   * Receiver functionality:
   * 1. Get connections from expected number of propagators;
   * 2. When the paired propagator sends out a update buffer, it must notify the
   * paired receiver of the internal updates that's included. The internal 
   * update buffer will be saved to remove my own updates.
   * 3. When receives an update buffer, check if the corresponding table is 
   * loop. If it is, remove my own updates and destroy the corresponding update 
   * buffer.
   * 4. The received upates triggers user callback if corresponding table's 
   * callback is true.
   * 5. When receiving an update buffer, need to make sure updates from one peer
   * are ordered -- this is to ensure a contiguous range of received updates 
   * from one client. This relies on the ordering of messages of 0MQ (both TCP 
   * conection and inproc pipe).
   *
   */

public:
  Receiver();
  int Start(ReceiverConfig &_config, sem_t *sync_sem);
  int RegisterTable(InternalTable *_itable);
  int GetErrCode();
  int SignalTerm();
  int WaitTerm();

private:
  static bool HasAllPeersAckedTerm(boost::unordered_map<int32_t, 
							PeerPropagatorInfo> 
				   &_peer_info);
  static void *ReceiverThrMain(void *_argu);
  
  volatile ReceiverState state_;
  ReceiverThrInfo thrinfo_;
  zmq::context_t *zmq_ctx_;
  pthread_t recv_thr_;
  std::string internal_recv_pull_endp_;
  bool have_signaled_term_;
  
  boost::unordered_map<int32_t, InternalTable*> table_dir_;
  boost::thread_specific_ptr<zmq::socket_t> term_push_sock_;
  
};

}

#endif
