#include <sys/types.h>
#include <sys/syscall.h>
#include <sstream>

#include "propagator.hpp"
#include <quiltdb/utils/timer_thr.hpp>


namespace quiltdb {

Propagator::Propagator():
  started_(false),
  errcode_(0){}

Propagator::~Propagator(){};

int Propagator::Start(PropagatorConfig &_config, sem_t *_sync_sem){
  
  if(started_) return -1;

  nanosec_ = _config.nanosec_;
  PropagatorThrInfo thrinfo;
  thrinfo.propagator_ptr_ = this;
  thrinfo.sync_sem_ = _sync_sem;

  int ret = pthread_create(&thr_, NULL, PropagatorThrMain, &thrinfo);
  if(ret != 0) return -1;

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

int32_t Propagator::TimerHander(void * _propagator, int32_t _rem){
  return 0;
}

void *Propagator::PropagatorThrMain(void *_argu){
  
  PropagatorThrInfo *thrinfo = reinterpret_cast<PropagatorThrInfo*>(_argu);
  Propagator *propagator_ptr = thrinfo->propagator_ptr_;
  zmq::context_t *zmq_ctx = propagator_ptr->zmq_ctx_;

  boost::scoped_ptr<zmq::socket_t> prop_push_sock;
  boost::scoped_ptr<zmq::socket_t> stop_pull_sock;
  boost::scoped_ptr<zmq::socket_t> timer_pull_sock;
  // PUSH sock to send cmd to timer thread
  boost::scoped_ptr<zmq::socket_t> timer_send_sock;

  pid_t tid = syscall(SYS_gettid);
  std::stringstream ss;
  ss << tid;

  propagator_ptr->timer_trigger_endp_ = "inproc://propagator." 
    + ss.str() 
    + ".timer.trigger";

  propagator_ptr->timer_cmd_endp_ = "inproc://propagator." + ss.str() 
    + ".timer.trigger";

  try{
    timer_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
    timer_send_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
  }catch(...){
    //TODO: wake up semaphore
    return 0;
  }

  return 0;
}
}
