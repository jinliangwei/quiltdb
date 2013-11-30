#ifndef __QUILTDB_PROPAGATOR_HPP__
#define __QUILTDB_PROPAGATOR_HPP__

#include <quiltdb/include/common.hpp>
#include <quiltdb/utils/memstruct.hpp>
#include <quiltdb/receiver/receiver.hpp>
#include <quiltdb/internal_table/internal_table.hpp>

#include <string>
#include <zmq.hpp>
#include <stdint.h>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/tss.hpp>
#include <semaphore.h>
#include <pthread.h>

namespace quiltdb {

struct PropagatorConfig {
  int32_t nanosec_;
  NodeInfo my_info_;
  zmq::context_t *zmq_ctx_;
  std::string update_push_endp_;
};

class Propagator : boost::noncopyable {

  struct PropagatorThrInfo{
    Propagator *propagator_ptr_;
    sem_t *sync_sem_;
    int32_t nanosec_;
    NodeInfo my_info_;
  };

  enum PropagatorState{INIT, RUN, TERM_SELF, TERM_DOWNSTREAM, TERM};

  /* 
   * Propagator state transition and termination logic
   *
   * Terminating propagator is mainly terminating its propagator thread.
   * The propagator thread should be responsible for
   * 1) termnate the timer thread;
   * 2) respond properly to termination message from downstream receiver.
   * 
   * The receiver assumes after it sees a "termination pemission" message from
   * the propagator, the propagator will not send any more message from the 
   * propagator.
   * 
   * The whole logic is to cope with 0MQ to ensure when threads exit, there's no
   * unprocessed message in its sockets so sockets can be shutdown properly.
   *
   * Propagator State Transition Diagram:
   *
   * INIT --> RUN --> TERM_SELF -----------> TERM
   *            |                             ^
   *            '---> TERM_DOWNSTREAM --------'
   *
   * Meaning of different states:
   * INIT: created but not yet started, allows initialization (register tables)
   * RUN: running, accepts updates from applcation threads and propagate them
   * TERM_SELF: application acknowledges that it is prepared to accept 
   *            propagator tear down at anytime -- that is, the propagator may 
   *            turn to TERM state any time and stop propagating updates of 
   *            itself and others'
   * TERM_DOWNSTREAM: downstream process is ready to terminate and is waiting 
   *                  for permission
   * TERM: terminated, no more operation
   *
   * Transition triggers:
   * INIT --> RUN: Start()
   * RUN --> TERM_SELF: SignalTerm()
   * RUN --> TERM_DOWNSTREAM: recieved termination message from downstream 
   *                          receiver
   * TERM_SELF --> TERM: same as RUN --> TERM_DOWNSTREAM
   * TERM_DOWNSTREAM --> TERM: same as RUN --> TERM_SELF
   * When transitting to TERM, send termination ackonwledgement to downstream
   * receiver.
   * 
   * Basically, the propagator can terminate if it receives:
   * 1) SignalTerm() and
   * 2) termination message from its downstream receiver
   *
   * Actions permitted for each state:
   * INIT: RegisterTable(), Start()
   * RUN: Inc(), ApplyUpdates(), SignalTerm(), GetErrCode()
   * TERM_SELF: Inc(), ApplyUpdates(), WaitTerm(), GetErrCode()
   * TERM_DOWNSTREAM: Inc(), ApplyUpdates(), SingalTerm(), GetErrCode()
   * TERM: GetErrCode()
   */

public:
  Propagator();
  ~Propagator();
  int Start(PropagatorConfig &_config, sem_t *_sync_sem);
  int RegisterTable(int32_t _table_id, InternalTable *_itable);
  int Inc(int32_t _table_id, int32_t _key, const uint8_t *_delta, 
	  int32_t _num_bytes);
  int ApplyUpdates(int32_t _table_id, UpdateBuffer *_updates);
  int SignalTerm();
  int WaitTerm();
  int GetErrCode();
  
private:
  static int32_t TimerHandler(void * _propagator, int32_t _rem);
  static void *PropagatorThrMain(void *_argu);

  int32_t TimerTrigger();
  
  pthread_t thr_;

  // write by propagator thread, read by application threads
  volatile PropagatorState state_;
  volatile int errcode_; // 0 -> OK; nonzero -> error
  
  // Intialized by propagator thread, read by timer thread.
  // Timer thread is created after both strings are initialized. Since
  // pthread_create acts as a memory barrier, timer thread should read the
  // proper string value.
  std::string timer_trigger_endp_;
  std::string timer_cmd_endp_;
  
  // Initialized by application threads, shared read access from application and
  // propagator threads.
  zmq::context_t *zmq_ctx_;
  std::string update_pull_endp_;
  
  // access by application threads before propagator thread starts running
  // for RegisterTable()
  boost::unordered_map<int32_t, InternalTable* > table_dir_;
  boost::unordered_map<int32_t, boost::unordered_map<int64_t, uint8_t* > > 
  update_store_;
  
  boost::thread_specific_ptr<zmq::socket_t> timer_push_sock_;

  // PULL sock for timer thread to receive cmd
  boost::thread_specific_ptr<zmq::socket_t> timer_recv_sock_;
};

}

#endif
