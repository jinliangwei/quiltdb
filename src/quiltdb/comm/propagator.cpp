#include <sys/types.h>
#include <sys/syscall.h>
#include <sstream>
#include <glog/logging.h>

#include "propagator.hpp"
#include "protocol.hpp"
#include <quiltdb/utils/timer_thr.hpp>
#include <quiltdb/utils/zmq_util.hpp>

namespace quiltdb {

namespace {

inline int InitThrSockIfHaveNot(boost::thread_specific_ptr<zmq::socket_t> *_sock,
				zmq::context_t *_zmq_ctx, int _type,
				const char *_connect_endp){
  if(_sock->get() == NULL){
    try{
      // zmq::socket_t() may throw error_t
      _sock->reset(new zmq::socket_t(*_zmq_ctx, _type));
      (*_sock)->connect(_connect_endp);	  
    }catch(zmq::error_t &e){
      VLOG(0) << "connect failed, e.what() = " << e.what();
      return -1;
    }catch(...){
      return -1;
    }
  }
  return 0;

}
inline int InitScopedSockIfHaveNot(boost::scoped_ptr<zmq::socket_t> *_sock,
				   zmq::context_t *_zmq_ctx, int _type,
				   const char *_connect_endp){
  if(_sock->get() == NULL){
    try{
      // zmq::socket_t() may throw error_t
      _sock->reset(new zmq::socket_t(*_zmq_ctx, _type));
      (*_sock)->connect(_connect_endp);	  
    }catch(zmq::error_t &e){
      VLOG(0) << "connect failed, e.what() = " << e.what();
      return -1;
    }catch(...){
      return -1;
    }
  }
  return 0;
}

}  // anonymous namespace


Propagator::Propagator():
  state_(INIT),
  errcode_(0),
  have_signaled_term_(false){}

Propagator::~Propagator(){};

int Propagator::Start(PropagatorConfig &_config, sem_t *_sync_sem){
  
  if(state_ != INIT) return -1;

  zmq_ctx_ = _config.zmq_ctx_;
  update_pull_endp_ = _config.update_pull_endp_;
  
  sem_t internal_sync_sem;
  sem_init(&internal_sync_sem, 0, 0);

  thrinfo_.my_id_ = _config.my_id_;
  thrinfo_.propagator_ptr_ = this;
  thrinfo_.sync_sem_ = _sync_sem;
  thrinfo_.nanosec_ = _config.nanosec_;
  thrinfo_.my_info_ = _config.my_info_;
  thrinfo_.internal_pair_endp_ = config.internal_pair_endp_;
  thrinfo_.internal_sync_sem_ = &internal_sync_sem;

  int ret = pthread_create(&thr_, NULL, PropagatorThrMain, &thrinfo_);
  
  sem_destroy(&internal_sync_sem);
  if(ret != 0) return -1;

  return 0;
}

void Propagator::RegisterTable(int32_t _table_id, ValueAddFunc vadd_func,
			       int32_t _update_size, bool _loop,
			       bool _apply_updates){
  table_dir_[_table_id].vadd_func_ = vadd_func;
  table_dir_[_table_id].update_size_ = _update_size;
  table_dir_[_table_id].loop_ = _loop;
  table_dir_[_table_id].apply_updates_ = _apply_updates;
  return;
}

int Propagator::RegisterThr(){
  if(state_ != RUN) return -1;
  if(update_push_sock_.get() != NULL) return -1;
  
  try{
    // zmq::socket_t() may throw error_t
    update_push_sock_.reset(new zmq::socket_t(*zmq_ctx_, ZMQ_PUSH));
    update_push_sock_->connect(update_pull_endp_.c_str());
  }catch(...){
    return -1;
  }

  return 0;
}

int Propagator::DeregisterThr(){
  // can happen in RUN, TERM_PREP, TERM
  if(update_push_sock_.get() == NULL) return -1;
  
  update_push_sock_.reset();
  return 0;

}

int Propagator::Inc(int32_t _table_id, int32_t _key, const uint8_t *_delta, 
		    int32_t _num_bytes){
  if(state_ != RUN) return -1;
  if(update_push_sock_.get() == NULL) return -1;

  PUpdateLogMsg updatelog;
  updatelog.msgtype_ = EPUpdateLog;
  updatelog.table_id_ = _table_id;
  updatelog.key_ = _key;
  updatelog.update_type_ = EInc;

  int ret = SendMsg(*update_push_sock_, (uint8_t *) &updatelog, 
		    sizeof(PUpdateLogMsg), ZMQ_SNDMORE);

  if(ret != sizeof(PUpdateLogMsg)){
    errcode_ = 1;
    return -1;
  }

  ret = SendMsg(*update_push_sock_, _delta, _num_bytes, 0);

  if(ret != _num_bytes){
    errcode_ = 1;
    return -1;
  }

  return 0;
}

int Propagator::SignalTerm(){
  if(state_ != RUN) return -1;

  PropagatorMsgType msgtype = EPInternalTerminate;

  int ret = SendMsg(*update_push_sock_, (uint8_t *) &msgtype, 
		    sizeof(PropagatorMsgType), 0);

  if(ret != sizeof(PropagatorMsgType)){
    errcode_ = 1;
    return -1;
  }
  have_signaled_term_ = true;
  return 0;
}

int Propagator::WaitTerm(){
  if(state_ != RUN && state_ != TERM_PREP && state_ != TERM) return -1;
  if(!have_signaled_term_) return -1;
  
  pthread_join(thr_, NULL);
  return 0;
}

int Propagator::CommitUpdates(int32_t _table_id, UpdateBuffer *_updates){
  
  return 0;
}

int Propagator::StopLocalReceiver(){
  return 0;
}

int32_t Propagator::TimerHandler(void * _propagator, int32_t _rem){
  // TODO: right now, I just igonre _rem

  Propagator *propagator_ptr = reinterpret_cast<Propagator*>(_propagator);

  int ret = InitThrSockIfHaveNot(&(propagator_ptr->timer_push_sock_), 
				 propagator_ptr->zmq_ctx_, ZMQ_PUSH, 
				 (propagator_ptr->timer_trigger_endp_).c_str());
  if(ret < 0){
    VLOG(0) << "failed, creating and initializing timer_push_sock_"
	    << " endp = " << (propagator_ptr->timer_trigger_endp_); 
   propagator_ptr->errcode_ = 1;
    return -1;
  }

  ret = InitThrSockIfHaveNot(&(propagator_ptr->timer_recv_sock_), 
			     propagator_ptr->zmq_ctx_, ZMQ_PULL, 
			     (propagator_ptr->timer_cmd_endp_).c_str());
  if(ret < 0){
    propagator_ptr->errcode_ = 1;
    return -1;
  }

  TimerMsgType msgtype = ETimerTrigger;
  
  ret = SendMsg(*(propagator_ptr->timer_push_sock_), (uint8_t *) &msgtype, 
	  sizeof(TimerMsgType), 0);

  // Timer failed to send trigger propagator thread, Timer will just stop
  if(ret <= 0){
    propagator_ptr->errcode_ = -1;
    return -1;
  }

  boost::shared_array<uint8_t> data;
  ret = RecvMsg(*(propagator_ptr->timer_recv_sock_), data, NULL);
  if(ret != sizeof(TimerCmdMsg)){
    propagator_ptr->errcode_ = -1;
    return -1;
  }
  TimerCmdMsg *msg_ptr = reinterpret_cast<TimerCmdMsg*>(data.get());
  return msg_ptr->nanosec_;
}

void *Propagator::PropagatorThrMain(void *_argu){

  VLOG(0) << "created propagator thread!";
  
  PropagatorThrInfo *thrinfo = reinterpret_cast<PropagatorThrInfo*>(_argu);
  Propagator *propagator_ptr = thrinfo->propagator_ptr_;
  zmq::context_t *zmq_ctx = propagator_ptr->zmq_ctx_;
  int32_t my_id = thrinfo->my_id_;
  std::string internal_pair_endp = thrinfo->internal_pair_endp_;

  // the update store for a particulr table is created on the first update
  // received for that table.
  boost::unordered_map<int32_t, boost::unordered_map<int64_t, uint8_t* > > 
    update_store;
  boost::unordered_map<int32_t, boost::unordered_map<int32_t, UpdateRange> >
    table_peers_update_range;
  boost::unordered_map<int32_t, UpdateRange> my_update_range;
  boost::unordered_map<int32_t, boost::unordered_map<int64_t, uint8_t* > >
    my_update_store;

  bool have_received_ds_term = false;
  bool have_stoped_timer_thr = false;
  bool have_stopped_recv_thr = false; // received ack from recv thread

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
  
  // initialized only when tending to send message to internal receiver
  boost::scoped_ptr<zmq::socket_t> internal_recv_push_sock;
  boost::scoped_ptr<zmq::socket_t> internal_prop_recv_pair_sock;

  // initialize my_update_range
  {
    boost::unordered_map<int32_t, int32_t>::iterator table_iter;
    for(table_iter = propagator_ptr->table_dir_.begin();
	table_iter != propagator_ptr->table_dir_.end();
	table_iter++){

      UpdateRange ur;
      ur.st_ = 0;
      ur.end_ = 0;
      my_update_range[table_iter->first] = ur;
    }
  }

  pid_t tid = syscall(SYS_gettid);
  std::stringstream ss;
  ss << tid;

  propagator_ptr->timer_trigger_endp_ = "inproc://propagator." 
    + ss.str() + ".timer.trigger";

  propagator_ptr->timer_cmd_endp_ = "inproc://propagator." + ss.str() 
    + ".timer.cmd";

  VLOG(0) << "propagator thread start initializing sockets";

  try{
    // inproc sockets
    update_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
    update_pull_sock->bind(propagator_ptr->update_pull_endp_.c_str());    
    
    // TCP sockets
    //prop_push_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
    //recv_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));

    internal_prop_recv_pair_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PAIR));
    internal_prop_recv_pair_sock->bind(internal_pair_endp.c_str());
    
  }catch(zmq::error_t &e){
    VLOG(0) << "Failed to set up sockets, e.what() = " << e.what();
    propagator_ptr->errcode_ = 1;
    sem_post(thrinfo->sync_sem_);
  }catch(...){
    VLOG(0) << "Failed to set up sockets";
    propagator_ptr->errcode_ = 1;
    sem_post(thrinfo->sync_sem_);
  }
  
  sem_post(thrinfo->internal_sync_sem_);
  //TODO: wait for message from internal receiver for connection
  
  VLOG(3) << "propagator thread initiazlied all sockets!";

  // TODO: wait for connection messages from receiver
  // perform handshake with receiver
  propagator_ptr->state_ = RUN;
  sem_post(thrinfo->sync_sem_);
  
  int num_poll_sock = 1; //TODO: add 1 when recv_pull_sock is set up
  NanoTimer timer;
  if(thrinfo->nanosec_ > 0){
    try{
      timer_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
      timer_send_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
      timer_pull_sock->bind(propagator_ptr->timer_trigger_endp_.c_str());
      timer_send_sock->bind(propagator_ptr->timer_cmd_endp_.c_str());
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
      VLOG(3) << "poll get " << num_poll << " messages";
    } catch (zmq::error_t &e) {
      propagator_ptr->errcode_ = 1;
      LOG(FATAL) << "propagator thread pull failed, error = " << e.what() 
		 << "\nPROCESS EXIT";
      // if I fail to poll, I don't know what is being sent to me
      // the client thread might be waiting on condtion variable -- just exit
    }
    
    // give timer trigger the highest priority to simulate accurate timer
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
	// TODO: prepare an update buffer and send it out
	
	TimerCmdMsg tcmd_msg;
	tcmd_msg.msgtype_ = ETimerCmd;
	// Right now, we don't have any logic to dynamically adjust the batch 
	// wait time
	if(propagator_ptr->state_ == RUN){
	  tcmd_msg.nanosec_ = thrinfo->nanosec_;
	}else{
	  tcmd_msg.nanosec_ = 0; // to terminate timer thread
	}
	int ret = SendMsg(*timer_send_sock, 
			  reinterpret_cast<uint8_t*>(&tcmd_msg), 
			  sizeof(TimerCmdMsg), 0);
	if(ret != sizeof(TimerCmdMsg)){
	  VLOG(0) << "SendMsg to timer_send_sock failed";
	  propagator_ptr->errcode_ = 1;
	}
	
	VLOG(2) << "set up update buffer to send out";
	boost::unordered_map<int32_t, 
			     boost::unordered_map<int64_t, uint8_t* > >::iterator
	  table_iter;
	for(table_iter = update_store.begin(); table_iter != update_store.end();
	    table_iter++){
	  int32_t table_id = table_iter->first;
	  int32_t num_updates = table_iter->second.size();
	  if(num_updates == 0) continue;
	  // Note that num_peers is just an upper bound of the number of peers 
	  // to be added to the buffer, as the table may contain range of 0
	  int32_t num_peers = table_peers_update_range[table_id].size();
	  int32_t update_size = (propagator_ptr->table_dir_)[table_id].update_size_;
	  VLOG(2) << "update_size = " << update_size;
	  UpdateBuffer *update_buff = 
	    UpdateBuffer::CreateUpdateBuffer(update_size, num_updates, 
					     num_peers + 1);
	  boost::unordered_map<int64_t, uint8_t*>::const_iterator
	    update_iter;
	  for(update_iter = table_iter->second.begin(); 
	      update_iter != table_iter->second.end();
	      update_iter++){
	    int ret = update_buff->AppendUpdate(update_iter->first, 
						update_iter->second);
	    VLOG(2) << "appended update to buffer, key " << update_iter->first;
	    CHECK(ret == 0) << "Append to update buffer failed";
	    delete[] update_iter->second;
	    table_iter->second.erase(update_iter);
	  }
	  boost::unordered_map<int32_t, UpdateRange>::iterator 
	    update_range_iter;
	  for(update_range_iter = table_peers_update_range[table_id].begin();
	      update_range_iter != table_peers_update_range[table_id].end();
	      update_range_iter++){
	    int64_t st = update_range_iter->second.st_;
	    int64_t end = update_range_iter->second.end_;
	    if(st <= end){
	      update_buff->UpdateNodeRange(update_range_iter->first, 
					  st, end);
	      update_range_iter->second.st_ = end + 1;
	    }
	  }
	  
	  if(my_update_range[table_id].st_ <= my_update_range[table_id].end_){
	    update_buff->UpdateNodeRange(my_id, my_update_range[table_id].st_, 
					 my_update_range[table_id].end_);
	  }

	  //TODO: this is only for debugging, remove it
	  if(update_buff->StartIteration() == 0){
	    int64_t key;
	    const uint8_t *update = update_buff->NextUpdate(&key);
	    while(update != NULL){
	      VLOG(2) << "update, key = " << key
		      << " update = " 
		      << *(reinterpret_cast<const int32_t*>(update));
	      update = update_buff->NextUpdate(&key);
	    }
	  }
	  
	  if(propagator_ptr->table_dir[table_id].loop_){
	    if(my_update_store[table_id].size() > 0){
	      int32_t num_my_updates = my_update_store[table_id].size();
	      UpdateBuffer *my_update_buff = 
		UpdateBuffer::CreateUpdateBuffer(update_size, num_my_updates, 1);
	      boost::unordered_map<int64_t, uint8_t*>::const_iterator
		my_update_iter;
	      for(my_update_iter = my_update_store[table_id].begin(); 
		  my_update_iter != my_update_store[table_id].end();
		  my_update_iter++){
		int ret = my_update_buff->AppendUpdate(my_update_iter->first, 
						       my_update_iter->second);
		CHECK(ret == 0) << "Append to update buffer failed";
		delete[] my_update_iter->second;
		my_update_store[table_id].erase(my_update_iter);	      
	      }
	      if(my_update_range[table_id].st_ 
		 <= my_update_range[table_id].end_){
		my_update_buff->UpdateNodeRange(my_id, 
					     my_update_range[table_id].st_, 
					     my_update_range[table_id].end_);
	      }else{
		LOG(FATAL) << "I have updates, but my update range is empty!";
	      }
	      // TODO: send to internal receiver and wait for its response
	      UpdateBuffer::DestroyUpdateBuffer(my_update_buff);
	    }
	  }
	  
	  my_update_range[table_id].st_ = my_update_range[table_id].end_ + 1;
	  
	  //TODO: send update buffer to downstream receiver!
	  UpdateBuffer::DestroyUpdateBuffer(update_buff);
	  
	}
	if(propagator_ptr->state_ == TERM_PREP){
	  timer.WaitStop();
	  have_stoped_timer_thr = true;
	  VLOG(0) << "timer thread has been stopped";
	  // because right now, I only need to stop timer thread, 
	  // exit when it's stopped
	  // TODO: check if have stopped internal receiver and
	  // acknolwedged downstream receiver
	  propagator_ptr->state_ = TERM;
	  return 0;
	}
	continue;
      }
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

	  if(len != sizeof(PUpdateLogMsg)){
	    propagator_ptr->errcode_ = 1;
	    VLOG(0) << "Malformed UpdateLog message";
	  }
	  
	  PUpdateLogMsg *updatelog 
	    = reinterpret_cast<PUpdateLogMsg*>(data.get());

	  int32_t tid = updatelog->table_id_;
	  int64_t key = updatelog->key_;
	  assert(updatelog->update_type_ == EInc);
	  len = RecvMsg(*update_pull_sock, data);
	  if(len <= 0){
	    propagator_ptr->errcode_ = 1;
	    VLOG(0) << "Malformed UpdateLog message";
	  }
	  
	  VLOG(1) << "received update log"
		  << " table " << tid
		  << " key " << key
		  << " len of update " << len;
	  
	  if(propagator_ptr->state_ != RUN){
	    break;
	  }

	  if(thrinfo->nanosec_ <= 0){
	    // TODO: create an update buffer that contains only one update and 
	    // send it out
	  }else{
	    uint8_t *update_delta = reinterpret_cast<uint8_t*>(data.get());
	    boost::unordered_map<int32_t, TableInfo>::const_iterator itr 
	      = propagator_ptr->table_dir_.find(tid);
	    if(itr == propagator_ptr->table_dir_.end()){
	      propagator_ptr->errcode_ = 1;
	      LOG(FATAL) << "Table " << tid << " does not exist";
	    }
	    ValueAddFunc table_vadd = itr->second.vadd_func_;
	    boost::unordered_map<int64_t, uint8_t*>::iterator update_itr 
	      = update_store[tid].find(key);
	    
	    if(update_itr == update_store[tid].end()){
	      update_store[tid][key] = new uint8_t[len];
	      memset(update_store[tid][key], 0, len);
	    }
	    table_vadd(update_store[tid][key], update_delta, len);
	    
	    if(propagator_ptr->table_dir_[tid].loop_){
	      boost::unordered_map<int64_t, uint8_t*>::iterator my_update_itr
		= my_update_store[tid].find(key);
	      if(my_update_itr == my_update_store[tid].end()){
		my_update_store[tid][key] = new uint8_t[len];
		memset(my_update_store[tid][key], 0, len);
	      }
	      table_vadd(my_update_store[tid][key], update_delta, len);
	    }
	  }
	}
	break;
      case EPUpdateBuffer:
	{
	  PUpdateBufferMsg *update_buffer_msg 
	    = reinterpret_cast<PUpdateBufferMsg*>(data.get());
	    
	  UpdateBuffer *update_buffer_ptr 
	    = update_buffer_msg->update_buffer_ptr_;

	  if(propagator_ptr->state_ == RUN){
	    // TODO: add local updates to buffer and send it out
	  }
	  // TODO: delete update_buffer_ptr
	}
	break;
      case EPInternalTerminate:
	{
	  VLOG(0) << "received termination, message";
	  // TODO: change state to TERM_PREP
	  // If have received termination message from downstream receiver, send
	  // downstream receiver a termination acknowledgement.
	  // After sending, send receiver a termination message, exit only when 
	  // received ack fro mreceiver.
	  // clear update store.
	  // Stop timer when the next timer firing happens
	  // The thread may return from
	  // 1) receiving timer thread msg if it has received termination 
	  // message from downstream receiver earlier
	  // 2) receiving termination ack from local receiver
	  // 3) receiving termination message from ds receiver
	  propagator_ptr->state_ = TERM_PREP;
	  if(have_received_ds_term){
	    // TODO: reply downstream receiver with termination ACK
	  }
	  // TODO: send recv thread a termination message
	  // clear remaing updates
	  boost::unordered_map<int32_t, 
	    boost::unordered_map<int64_t, uint8_t*> >::iterator
	    table_iter;
	  VLOG(0) << "starts clearing remaining updates";
	  for(table_iter = update_store.begin(); table_iter != update_store.end();
	      table_iter++){
	    VLOG(1) << "clear update of "
		    << " table " << table_iter->first;
	    boost::unordered_map<int64_t, uint8_t*>::iterator update_iter;
	    for(update_iter = table_iter->second.begin(); 
		update_iter != table_iter->second.end(); update_iter++){
	      VLOG(1) << "clear update of "
		      << " table " << table_iter->first
		      << " key " << update_iter->first
		      << " delta " << update_iter->second << std::endl;
	      delete[] update_iter->second;
	      table_iter->second.erase(update_iter);
	    }
	  }
	}
	break;
      }
      continue;
    }
    // TODO: check on receiver pull socket
  }
  LOG(FATAL) << "incorrect exit path!";
  return 0;
}
}
