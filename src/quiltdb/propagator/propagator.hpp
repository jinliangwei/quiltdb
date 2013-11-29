#ifndef __QUILTDB_PROPAGATOR_HPP__
#define __QUILTDB_PROPAGATOR_HPP__

#include <quiltdb/include/common.hpp>
#include <quiltdb/utils/memstruct.hpp>

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

};

class Propagator : boost::noncopyable {

  struct PropagatorThrInfo{
    Propagator *propagator_ptr_;
    sem_t *sync_sem_;
  };

public:
  Propagator();
  ~Propagator();
  int Start(PropagatorConfig &_config, sem_t *_sync_sem);
  int RegisterTable(int32_t _table_id);
  int Inc(int32_t _table_id, int32_t _key, const uint8_t *_delta, 
	  int32_t _num_bytes);
  int ApplyUpdates(int32_t _table_id, UpdateBuffer *_updates);
  int Stop();
  int GetErrCode();
  
private:
  static int32_t TimerHander(void * _propagator, int32_t _rem);
  static void *PropagatorThrMain(void *_argu);

  int32_t TimerTrigger();
  
  bool started_;
  pthread_t thr_;

  int errcode_;
  
  // after initialization, only accessed by propagator thread
  int32_t nanosec_;
  NodeInfo my_info_;
  zmq::context_t *zmq_ctx_;
  
  // initialized by propagator thread, used by timer thread
  std::string timer_trigger_endp_;
  std::string timer_cmd_endp_;
  // only accessed by propagator thread
  boost::unordered_map<int32_t, boost::unordered_map<int64_t, uint8_t* > > 
  update_store;
  
  boost::thread_specific_ptr<zmq::socket_t> timer_push_sock_;

  // PULL sock for timer thread to receive cmd
  boost::thread_specific_ptr<zmq::socket_t> timer_recv_sock_;
};

}

#endif
