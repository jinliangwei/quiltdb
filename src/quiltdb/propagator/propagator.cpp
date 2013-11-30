#include <sys/types.h>
#include <sys/syscall.h>
#include <sstream>
#include <glog/logging.h>

#include "propagator.hpp"
#include "protocol.hpp"
#include <quiltdb/utils/timer_thr.hpp>
#include <quiltdb/utils/zmq_util.hpp>

namespace quiltdb {

Propagator::Propagator():
  state_(INIT),
  errcode_(0){}

Propagator::~Propagator(){};

int Propagator::Start(PropagatorConfig &_config, sem_t *_sync_sem){
  
  if(state_ != INIT) return -1;

  PropagatorThrInfo thrinfo;
  thrinfo.propagator_ptr_ = this;
  thrinfo.sync_sem_ = _sync_sem;
  thrinfo.nanosec_ = _config.nanosec_;
  thrinfo.my_info_ = _config.my_info_;

  int ret = pthread_create(&thr_, NULL, PropagatorThrMain, &thrinfo);
  if(ret != 0) return -1;

  return 0;
}

int Propagator::RegisterTable(int32_t _table_id, InternalTable *_itable){
  
  return 0;
}

int Propagator::Inc(int32_t _table_id, int32_t _key, const uint8_t *_delta, 
		    int32_t _num_bytes){
  return 0;
}

int Propagator::ApplyUpdates(int32_t _table_id, UpdateBuffer *_updates){
  return 0;
}

int Propagator::SignalTerm(){
  return 0;
}

int Propagator::WaitTerm(){
  return 0;
}

int32_t Propagator::TimerHandler(void * _propagator, int32_t _rem){
  return 0;
}

void *Propagator::PropagatorThrMain(void *_argu){
  
  PropagatorThrInfo *thrinfo = reinterpret_cast<PropagatorThrInfo*>(_argu);
  Propagator *propagator_ptr = thrinfo->propagator_ptr_;
  zmq::context_t *zmq_ctx = propagator_ptr->zmq_ctx_;

  // TCP sockets
  boost::scoped_ptr<zmq::socket_t> prop_push_sock;
  // PULL sock to receive messages (basically, termination message) from
  // receiver
  boost::scoped_ptr<zmq::socket_t> recv_pull_sock;

  // inproc sockets
  boost::scoped_ptr<zmq::socket_t> update_pull_sock;
  boost::scoped_ptr<zmq::socket_t> timer_pull_sock;  
  // PUSH sock to send cmd to timer thread
  boost::scoped_ptr<zmq::socket_t> timer_send_sock;

  pid_t tid = syscall(SYS_gettid);
  std::stringstream ss;
  ss << tid;

  propagator_ptr->timer_trigger_endp_ = "inproc://propagator." 
    + ss.str() + ".timer.trigger";

  propagator_ptr->timer_cmd_endp_ = "inproc://propagator." + ss.str() 
    + ".timer.trigger";

  try{
    // inproc sockets
    update_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
    
    // TCP sockets
    //prop_push_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
    //recv_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));

    timer_pull_sock->bind(propagator_ptr->timer_trigger_endp_.c_str());
    timer_send_sock->bind(propagator_ptr->timer_cmd_endp_.c_str());
  }catch(...){
    VLOG(0) << "Failed to set up sockets";
    propagator_ptr->errcode_ = 1;
    sem_post(thrinfo->sync_sem_);
  }

  // TODO: wait for connection messages from receiver
  sem_post(thrinfo->sync_sem_);
  int num_poll_sock = 1; //TODO: add 1 when recv_pull_sock is set up
  NanoTimer timer;
  if(thrinfo->nanosec_ > 0){
    try{
      timer_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
      timer_send_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
    }catch(...){
      VLOG(0) << "Failed to set up sockets";
      propagator_ptr->errcode_ = 1;
    }
    ++num_poll_sock;
    int ret = timer.Start(thrinfo->nanosec_, TimerHandler,
			  thrinfo->propagator_ptr_);
  }
  
  zmq::pollitem_t *pollitems = new zmq::pollitem_t[num_poll_sock];
  pollitems[0].socket = *(update_pull_sock.get());
  pollitems[0].events = ZMQ_POLLIN;

  if(thrinfo->nanosec_ > 0){
    pollitems[1].socket = *(timer_pull_sock.get());
    pollitems[1].events = ZMQ_POLLIN;
  }

  while(true){
    try { 
      int num_poll;
      num_poll = zmq::poll(pollitems, num_poll_sock);
      VLOG(2) << "poll get " << num_poll << " messages";
    } catch (zmq::error_t &e) {
      propagator_ptr->errcode_ = 1;
      LOG(FATAL) << "propagator thread pull failed, error = " << e.what() 
		 << "\nPROCESS EXIT";
      // if I fail to poll, I don't know what is being sent to me
      // the client thread might be waiting on condtion variable -- just exit
    }

    // received update
    if(pollitems[0].revents){
      boost::shared_array<uint8_t> data;
      int len;
      PropagatorMsgType msgtype;
      len = RecvMsg(*update_pull_sock, data);
      if(len <= 0){
	propagator_ptr->errcode_ = 1;
	LOG(FATAL) << "propagator thread read message type failed, "
		   << "error \nPROCESS EXIT!";
      }
      msgtype = *(reinterpret_cast<PropagatorMsgType*>(data.get()));
      switch(msgtype){
      case EPUpdateLog:
	{
	  if(len != sizeof(EPUpdateLogMsg)){
	    propagator_ptr->errcode_ = 1;
	    VLOG(0) << "Malformed UpdateLog message";
	  }
	  
	  EPUpdateLogMsg *updatelog 
	    = reinterpret_cast<EPUpdateLogMsg*>(data.get());

	  int32_t tid = updatelog->table_id_;
	  int64_t key = updatelog->key_;
	  assert(updatelog->uptype_ == EInc);

	  len = RecvMsg(*update_pull_sock, data);
	  if(len <= 0){
	    propagator_ptr->errcode_ = 1;
	    VLOG(0) << "Malformed UpdateLog message";
	  }
	  if(thrinfo->nanosec_ <= 0){
	    // TODO: create an update buffer that contains only one update and 
	    // send it out
	  }else{
	    uint8_t *update_delta = reinterpret_cast<uint8_t*>(data.get());
	    boost::unordered_map<int32_t, InternalTable* >::const_iterator itr 
	      = propagator_ptr->table_dir_.find(tid);
	    if(itr == propagator_ptr->table_dir_.end()){
	      propagator_ptr->errcode_ = 1;
	      VLOG(0) << "Table " << tid << " does not exist";
	    }
	    InternalTable *table = itr->second;
	    boost::unordered_map<int64_t, uint8_t*>::iterator update_itr 
	      = propagator_ptr->update_store_[tid].find(key);
	    
	    if(update_itr == propagator_ptr->update_store_[tid].end()){
	      propagator_ptr->update_store_[tid][key] = new uint8_t[len];
	      memset(propagator_ptr->update_store_[tid][key], 0, len);
	    }
	    table->Add(propagator_ptr->update_store_[tid][key], update_delta, len);
	  }
	}
	break;
      case EPUpdateBuffer:
	{
	  EPUpdateBufferMsg *update_buffer_msg 
	    = reinterpret_cast<EPUpdateBufferMsg*>(data.get());
	    
	  UpdateBuffer *update_buffer_ptr 
	    = update_buffer_msg->update_buffer_ptr_;
	  // TODO: delete update_buffer_ptr
	}
	break;
      case EPTerminate:
	if(propagator_ptr->state_ == RUN){
	  propagator_ptr->state_ == TERM_SELF;
	}else{
	  // TODO: send downstream receiver a termination acknowledgement
	  propagator_ptr->state_ = TERM;
	  return 0;
	}
	break;
      }
      continue;
    }
    
    if(thrinfo->nanosec_ > 0){
      if(pollitems[1].revents){
	boost::shared_array<uint8_t> data;
	int len;
	TimerMsgType msgtype;
	len = RecvMsg(*timer_pull_sock, data);
	if(len <= 0){
	  propagator_ptr->errcode_ = 1;
	  LOG(FATAL) << "propagator thread read message type failed, "
		     << "error \nPROCESS EXIT!";
	}
	msgtype = *(reinterpret_cast<TimerMsgType*>(data.get()));
	CHECK_EQ(msgtype, ETimerTrigger);
	// TODO: 
      }
    }
  }

  return 0;
}
}
