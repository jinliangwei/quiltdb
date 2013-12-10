#include "receiver.hpp"
#include "comm_util.hpp"
#include "protocol.hpp"
#include "quiltdb/utils/memstruct.hpp"
#include "quiltdb/utils/zmq_util.hpp"

#include <iostream>
#include <glog/logging.h>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/tss.hpp>
#include <boost/shared_array.hpp>
#include <queue>
#include <zmq.hpp>

namespace quiltdb {

Receiver::Receiver():
  state_(INIT){}

int Receiver::Start(ReceiverConfig &_config, sem_t *_sync_sem){
  if(state_ != INIT) return -1;

  zmq_ctx_ = _config.zmq_ctx_;
  internal_recv_pull_endp_ = _config.internal_recv_pull_endp_;

  thrinfo_.my_id_ = _config.my_id_;
  thrinfo_.my_info_ = _config.my_info_;
  thrinfo_.num_expected_propagators_ = _config.num_expected_propagators_;
  thrinfo_.zmq_ctx_ = _config.zmq_ctx_;
  thrinfo_.update_push_endp_ = _config.update_push_endp_;
  thrinfo_.internal_recv_pull_endp_ = _config.internal_recv_pull_endp_;
  thrinfo_.internal_pair_recv_push_endp_ 
    = _config.internal_pair_recv_push_endp_;

  thrinfo_.sync_sem_ = _sync_sem;
  thrinfo_.receiver_ptr_ = this;

  int ret = pthread_create(&recv_thr_, NULL, ReceiverThrMain, &thrinfo_);
  
  CHECK_EQ(ret, 0) << "Create receiver thread failed";

  return 0;
}

int Receiver::RegisterTable(InternalTable *_itable){
  
  if(state_ != INIT) return -1;
  int32_t table_id = _itable->GetID();
  table_dir_[table_id] = _itable;
  return 0;
}

int Receiver::SignalTerm(){
  
  if(state_ != RUN) return -1;
  int ret = InitThrSockIfHaveNot(&term_push_sock_, zmq_ctx_, ZMQ_PUSH,
				 internal_recv_pull_endp_.c_str());
  CHECK_EQ(ret, 0) << "Failed setting term sock";
  
  ReceiverMsgType term_msg = ERInternalTerminate;

  ret = SendMsg(*term_push_sock_, (uint8_t*) &term_msg, sizeof(ReceiverMsgType),
		0);
  CHECK_EQ(ret, sizeof(ReceiverMsgType)) 
    << "Failed sending termination message";
  have_signaled_term_ = true;

  term_push_sock_.reset(0);

  return 0;
}

int Receiver::WaitTerm(){
  if(state_ == INIT) return -1;
  if(!have_signaled_term_) return -1;
 
  pthread_join(recv_thr_, NULL);
  return 0;
}

bool Receiver::HasAllPeersAckedTerm(boost::unordered_map<int32_t,
							PeerPropagatorInfo> 
				   &_peer_info){

  boost::unordered_map<int32_t, PeerPropagatorInfo>::iterator
    peer_info_iter;

  for(peer_info_iter = _peer_info.begin(); peer_info_iter != _peer_info.end();
      peer_info_iter++){
    
    if(!peer_info_iter->second.has_replied_termination_){
      return false;
    }
  }

  return true;
}

void *Receiver::ReceiverThrMain(void *_argu){

  ReceiverThrInfo *thrinfo = reinterpret_cast<ReceiverThrInfo*>(_argu);
  zmq::context_t *zmq_ctx = thrinfo->zmq_ctx_;
  int32_t my_id = thrinfo->my_id_;
  int32_t num_expected_propagators = thrinfo->num_expected_propagators_;
  Receiver *receiver_ptr = thrinfo->receiver_ptr_;

  boost::unordered_map<int32_t, std::queue<UpdateBuffer*> > my_updates;
  boost::unordered_map<int32_t, PeerPropagatorInfo> peer_prop_info;

  // TCP sockets
  // receive updates from propagator
  boost::scoped_ptr<zmq::socket_t> update_pull_sock;
  // PUSH termination message to propagator
  boost::scoped_ptr<zmq::socket_t> term_pub_sock;
  
  // inproc sockets
  // for communicating my own updates, also pull termination message
  boost::scoped_ptr<zmq::socket_t> internal_recv_pull_sock;
  boost::scoped_ptr<zmq::socket_t> internal_pair_recv_push_sock;
  // push received updates to propagator
  boost::scoped_ptr<zmq::socket_t> internal_update_push_sock;

  bool have_received_prop_pair_term = false;
  bool have_replied_prop_pair_term = false;
  
  try{
    if(thrinfo->num_expected_propagators_ > 0){
     
      update_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
      std::string tcp_update_pull_endp = "tcp://" 
	+ thrinfo->my_info_.recv_pull_ip_ + ":"
	+ thrinfo->my_info_.recv_pull_port_;
      update_pull_sock->bind(tcp_update_pull_endp.c_str());
      
      term_pub_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUB));
      std::string tcp_term_push_endp = "tcp://"
	+ thrinfo->my_info_.recv_push_ip_ + ":"
	+ thrinfo->my_info_.recv_push_port_;
      term_pub_sock->bind(tcp_term_push_endp.c_str());
    }

    internal_recv_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
    internal_recv_pull_sock->bind(thrinfo->internal_recv_pull_endp_.c_str());
    VLOG(0) << "internal_recv_pull_sock binds to " 
	    << thrinfo->internal_recv_pull_endp_;

    internal_pair_recv_push_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
    internal_pair_recv_push_sock->connect(thrinfo->internal_pair_recv_push_endp_.c_str());
    
    VLOG(0) << "internal_pair_recv_push_sock connects to "
	    << thrinfo->internal_pair_recv_push_endp_;

    internal_update_push_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
    internal_update_push_sock->connect(thrinfo->update_push_endp_.c_str());
    VLOG(0) << "internal_update_push_sock connects to "
	    << thrinfo->update_push_endp_;

  }catch(zmq::error_t &e){
    LOG(FATAL) << "Failed initializing sockets, error: " << e.what();
  }catch(...){
    LOG(FATAL) << "Failed setting up sockets";
  }

  //TODO: received connections from expected number of propagators
  // Recv from update_pull_sock, one message per propagator
  // Send message on term_push_sock one per propagator

  // Handshake with internal propagator
  VLOG(2) << "Successfully initialized sockets";
  PropagatorMsgType init_msg = EPRInit;
  int ret = SendMsg(*internal_pair_recv_push_sock, (uint8_t *) &init_msg, 
		    sizeof(PropagatorMsgType), 0);
  CHECK_EQ(ret, sizeof(PropagatorMsgType)) 
    << "Send on internal_prop_recv_pair_sock failed, ret = " << ret;

  // Handshake iwth external propagator

  if(thrinfo->num_expected_propagators_ > 0){
  // Step 1: Propagator -> Receiver: PropInit
    boost::shared_array<uint8_t> data;
    int32_t num_props;
    for(num_props = 0; num_props < thrinfo->num_expected_propagators_; 
	++num_props){
      ret = RecvMsg(*update_pull_sock, data);
      CHECK_EQ(ret, sizeof(PropInitMsg)) << "RecvMsg failed ret = " << ret ;
      PropInitMsg *msg = reinterpret_cast<PropInitMsg*>(data.get());
      VLOG(2) << "Received PropInitMsg from " << msg->node_id_;
      peer_prop_info[msg->node_id_].has_started_ = true;
      peer_prop_info[msg->node_id_].has_replied_termination_ = false;
    }

    // Step 2: Receiver -> Propagator: PropInitACK
    int32_t gid = 1;
    
    PropRecvMsgType initack_msg = PropInitACK;
    num_props = 0;
    while(num_props < thrinfo->num_expected_propagators_){
      ret = SendMsg(*term_pub_sock, gid, (uint8_t*) &initack_msg,
		    sizeof(PropRecvMsgType), 0);
      VLOG(0) << "Send PropInitAck!";
      CHECK_EQ(ret, sizeof(PropRecvMsgType)) << "Send InitAck failed, ret = "
					     << ret;
      timespec sleep_time, sleep_rem;
      sleep_time.tv_sec = 0;
      sleep_time.tv_nsec = 50000000;
      nanosleep(&sleep_time, &sleep_rem); // wait to allow the message to be propagated
      
      // Step 3: Propagator -> Receiver: PropInitACKACK
      do{
	ret = RecvMsgAsync(*update_pull_sock, data);
	CHECK(ret >=0) << "RecvMsgAsync failed";
	if(ret > 0){
	  
	  CHECK_EQ(ret, sizeof(PropInitAckAckMsg))
	    << "Received malformed message";
	
	  PropInitAckAckMsg *msg_ptr 
	    = reinterpret_cast<PropInitAckAckMsg*>(data.get());
	  
	  CHECK_EQ(msg_ptr->msgtype_, PropInitACKACK) 
	    << "Received malformed message"
	    << " msgtype = " << msg_ptr->msgtype_
	    << " expected = " << PropInitACKACK;
	  
	  VLOG(2) << "Received PropInitAckAckMsg from " 
		  << msg_ptr->node_id_;
	  ++num_props;
	  if(num_props == thrinfo->num_expected_propagators_)
	    break;
	}
      }while(ret > 0); 
    }
    
    // Step 4: Receiver -> Propagator: PropStart
    PropRecvMsgType prop_start_msg = PropStart;
    ret = SendMsg(*term_pub_sock, gid, (uint8_t*) &prop_start_msg,
		  sizeof(PropRecvMsgType), 0);
    CHECK_EQ(ret, sizeof(PropRecvMsgType)) << "Send PropStart failed, ret = " 
					   << ret;
  }  
  // Handshake with external propagator done
  
  receiver_ptr->state_ = RUN;
  sem_post(thrinfo->sync_sem_);
  
  // TODO: adjust number when TCP sockets are set up
  int num_poll_sock = 1;
  if(thrinfo->num_expected_propagators_ > 0){
    ++num_poll_sock;
  }
  zmq::pollitem_t *pollitems = new zmq::pollitem_t[num_poll_sock];
  pollitems[0].socket = *internal_recv_pull_sock;
  pollitems[0].events = ZMQ_POLLIN;
  
  if(thrinfo->num_expected_propagators_ > 0){
    pollitems[1].socket = *update_pull_sock;
    pollitems[1].events = ZMQ_POLLIN;
  }

  while(true){
    try{
      int num_poll;
      num_poll = zmq::poll(pollitems, num_poll_sock);
      VLOG(3) << "poll get " << num_poll << " messages";
    }catch(zmq::error_t &e){
      LOG(FATAL) << "receiver thread pull failed, error = " << e.what() 
		 << "\nPROCESS EXIT";
    }

    if(pollitems[0].revents){
      boost::shared_array<uint8_t> data;
      int len;
      ReceiverMsgType msgtype;
      len = RecvMsg(*internal_recv_pull_sock, data);

      CHECK(len > 0) << "receiver thread read message type failed, "
		     << "error \nPROCESS EXIT!";
    
      msgtype = *(reinterpret_cast<ReceiverMsgType*>(data.get()));
      switch(msgtype){
      case EMyUpdates:
	{
	  VLOG(2) << "Received EMyUpdates";

	  CHECK_EQ(len, sizeof(MyUpdatesMsg)) << "malformed MyUpdateMsg";
	  
	  MyUpdatesMsg *myupdate_msg 
	    = reinterpret_cast<MyUpdatesMsg*>(data.get());
	  int32_t table_id = myupdate_msg->table_id_;
	  UpdateBuffer *update_buff_ptr = myupdate_msg->update_buffer_ptr_;
	  
	  if(receiver_ptr->state_ == RUN){
	    boost::unordered_map<int32_t, std::queue<UpdateBuffer*> >::iterator
	      table_update_iter = my_updates.find(table_id);
	    if(table_update_iter == my_updates.end()){
	      my_updates[table_id].push(update_buff_ptr);
	    }
	  }else{
	    UpdateBuffer::DestroyUpdateBuffer(update_buff_ptr);
	  }
	  
	  PropagatorMsgType myupdates_ack = EMyUpdatesACK;

	  int ret = SendMsg(*internal_pair_recv_push_sock, 
			    (uint8_t*) &myupdates_ack, 
			    sizeof(PropagatorMsgType), 0);
	  CHECK_EQ(ret, sizeof(PropagatorMsgType)) << "send MyUpdateACK failed";

	}
	break;
      case ERInternalTerminate:
	{
	  VLOG(0) << "Received ERInternalTerminate";
	  receiver_ptr->state_= TERM_SELF;
	  // Once I'm signaled to terminate, I'm not going to process any 
	  // received update buffers (not forwarding them to propagator or
	  // execute application-defined callback, so I may simply discard 
	  // all my own update buffers

	  boost::unordered_map<int32_t, 
			       std::queue<UpdateBuffer*> >::iterator
	    update_buff_iter;
	  for(update_buff_iter = my_updates.begin();
	      update_buff_iter != my_updates.end();
	      update_buff_iter++){
	    UpdateBuffer *update_buff_ptr;
	    VLOG(0) << "clearing up update buffer of table "
		    << update_buff_iter->first;
	    while(!(update_buff_iter->second).empty()){
	      VLOG(0) << "Destroying buffer " << update_buff_ptr;
	      update_buff_ptr = update_buff_iter->second.front();
	      VLOG(0) << "Fetched buff ptr " << update_buff_ptr;
	      UpdateBuffer::DestroyUpdateBuffer(update_buff_ptr);
	      update_buff_iter->second.pop();
	    }
	  }
	  
	  if(have_received_prop_pair_term){
	    VLOG(2) << "have_received_prop_pair_term = true";
	    PropagatorMsgType term_ack_msg = EPRecvInternalTerminateACK;
	    int ret = SendMsg(*internal_update_push_sock, 
			      (uint8_t*) &term_ack_msg,
			      sizeof(PropagatorMsgType), 0);
	    have_replied_prop_pair_term = true;
	    
	    // TODO: remove this return after setting TCP sockets
	    //receiver_ptr->state_ = TERM;
	    //VLOG(0) << "Receiver exiting!";
	    //return 0;
	  }
	  
	  PropRecvMsgType term_msg = EPRTerminate;
	  int32_t gid = 1;
	  int ret = SendMsg(*term_pub_sock, gid, (uint8_t*) &term_msg, 
			    sizeof(PropRecvMsgType), 0);
	  CHECK_EQ(ret, sizeof(PropRecvMsgType));
	}
	break;
      case EPRInternalTerminate:
	{
	  VLOG(0) << "Received EPRInternalTerminate";
	  if(receiver_ptr->state_ == RUN){
	    VLOG(0) << "At running state, just turn on the flag";
	    have_received_prop_pair_term = true;
	  }else if(receiver_ptr->state_ == TERM_SELF){
	    PropagatorMsgType term_ack_msg = EPRecvInternalTerminateACK;
	    int ret = SendMsg(*internal_update_push_sock, 
			      (uint8_t*) &term_ack_msg,
			      sizeof(PropagatorMsgType), 0);
	    CHECK_EQ(ret, sizeof(PropagatorMsgType)) 
	      << "Send EPRecvInternalTerminateACK failed";
	    have_replied_prop_pair_term = true;
	    VLOG(0) << "Send EPRecvInternalTerminateACK to propagator";
	    if(HasAllPeersAckedTerm(peer_prop_info) 
	       && have_replied_prop_pair_term){
	      
	      receiver_ptr->state_ = TERM;
	      delete[] pollitems;
	      VLOG(0) 
		<< "**********Receiver exiting from EPRInternalTerminate!";
	      return NULL;
	    }
	  }else{
	    LOG(FATAL) << "I should not be in this state " 
		       << receiver_ptr->state_;
	  }
	}
	break;
      default:
	LOG(FATAL) << "received unrecognized message type " << msgtype;
      }
      
      continue;
    }
    
   
    if(thrinfo->num_expected_propagators_ > 0){
      if(pollitems[1].revents){
	 boost::shared_array<uint8_t> data;
	 int len;
	 PropRecvMsgType msgtype;
	 len = RecvMsg(*update_pull_sock, data);
	 msgtype = *(reinterpret_cast<PropRecvMsgType*>(data.get()));
	 switch(msgtype){
	 case EPRUpdateBuffer:
	   {
	     VLOG(0) << " node " << my_id
		     << " received EPRUpdateBuffer";
	     
	     EPRUpdateBufferMsg *update_buff_msg 
	       = reinterpret_cast<EPRUpdateBufferMsg*>(data.get());
	     int32_t table_id = update_buff_msg->table_id_;
	     
	     len = RecvMsg(*update_pull_sock, data);
	     CHECK(len > 0) << "Received UpdateBuffer size not positive";
	 
	     if(receiver_ptr->state_ != RUN) break;
	     
	     UpdateBuffer *recv_update_buff 
	       = reinterpret_cast<UpdateBuffer*>(data.get());

	     recv_update_buff->StartNodeRangeIteration();
	     int64_t test_st, test_end;
	     int32_t test_node_id = recv_update_buff->NextNodeRange(&test_st, 
								    &test_end);
	     if(test_node_id == my_id){
	       test_node_id = recv_update_buff->NextNodeRange(&test_st, 
							      &test_end);
	     }
	     if(test_node_id < 0){
	       break; // case ended
	     }

	     UpdateRange recv_my_update_range;
	     boost::unordered_map<int32_t, InternalTable*>::iterator
	       table_iter = receiver_ptr->table_dir_.find(table_id);
	     CHECK(table_iter != receiver_ptr->table_dir_.end());
	     bool do_user_cbk = table_iter->second->get_do_user_cbk();
	     bool forward_updates = table_iter->second->get_forward_updates();
	     bool apply_updates = table_iter->second->get_apply_updates();
	     UpdateBufferCbk ucbk = table_iter->second->get_update_buff_cbk();
	     bool recv_found = recv_update_buff->GetNodeRange(my_id, 
					     &(recv_my_update_range.st_),
					     &(recv_my_update_range.end_));
	     InternalTable *itable = table_iter->second;
	     if(recv_found){
	       VLOG(0) << "node " << my_id
		       << " need to remove duplicated updates";
	       
	       ValueSubFunc vsub_func = table_iter->second->get_vsub_func();
	       boost::unordered_map<int32_t, 
				    std::queue<UpdateBuffer*> >::iterator
		 my_updates_iter = my_updates.find(table_id);
	       
	       CHECK(my_updates_iter != my_updates.end()) 
		 << "Cannot find my updates to cancel duplicated updates";
	       int64_t seq_num = recv_my_update_range.st_;
	       std::queue<UpdateBuffer*> &my_updates_queue = my_updates[table_id];
	       
	       do{
		 
		 UpdateBuffer *my_update_buff = my_updates_queue.front();
		 UpdateRange my_update_range;
		 bool found = my_update_buff->GetNodeRange(my_id, 
							   &(my_update_range.st_),
							  &(my_update_range.end_));
		 CHECK(found) 
		   << "Cannot find my update range from my update buffer";
		 CHECK(recv_my_update_range.Contains(my_update_range))
		   << "My update range is not contained";
		 CHECK_EQ(my_update_range.st_, seq_num);
		 seq_num = my_update_range.end_;
		 my_updates_queue.pop();
		 
		 int ret = my_update_buff->StartIteration();
		 CHECK_EQ(ret, 0) << "start iteration failed";
		 int64_t key;
		 uint8_t *my_delta;
		 uint8_t *recv_delta;
		 my_delta = my_update_buff->NextUpdate(&key);
		 
		 while(my_delta != NULL){
		   recv_delta = recv_update_buff->GetUpdate(key);
		   CHECK(recv_delta != NULL) 
		     << "Cannot find update in received buffer"; 
		   vsub_func(recv_delta, my_delta, 
			     table_iter->second->get_vsize());
		 }
		 UpdateBuffer::DestroyUpdateBuffer(my_update_buff);
	       }while(seq_num < recv_my_update_range.end_);
	       recv_update_buff->DeleteNodeRange(my_id);
	     }

	     if(forward_updates){
	       uint8_t *recv_update_buff_mem;
	       UpdateBuffer *forward_recv_update_buff;
	       
	       recv_update_buff_mem = new uint8_t[len];
	       memcpy(recv_update_buff_mem, data.get(), len);
	       
	       forward_recv_update_buff 
		 = reinterpret_cast<UpdateBuffer*>(recv_update_buff_mem);
		 
	       PUpdateBufferMsg internal_update_buff_msg;
	       internal_update_buff_msg.msgtype_ = EPUpdateBuffer;
	       internal_update_buff_msg.table_id_ = table_id;
	       internal_update_buff_msg.update_buffer_ptr_ 
		 = forward_recv_update_buff;
	       
	       ret = SendMsg(*internal_update_push_sock, 
			     (uint8_t*) &internal_update_buff_msg, 
			     sizeof(PUpdateBufferMsg), 0);
	       CHECK_EQ(ret, sizeof(PUpdateBufferMsg)) 
		 << "Send UpdateBuffer to internal propagator failed";
	       
	       VLOG(0) << "Sent UpdateBuffer to internal propagator";
	     }	       

	     if(apply_updates){
	       recv_update_buff->StartIteration();
	       int64_t key;
	       uint8_t *delta;
	       delta = recv_update_buff->NextUpdate(&key);
	       while(delta != NULL){
		 itable->IncRaw(key, delta);
		 delta = recv_update_buff->NextUpdate(&key);
	       }
	     }
	     
	     if(do_user_cbk){
	       ucbk(table_id, recv_update_buff);
	     }
	   }
	 break;
	 case EPRTerminateACK:
	   {
	     PRTerminateAckMsg *msg_ptr 
	       = reinterpret_cast<PRTerminateAckMsg*>(data.get());
	     int32_t node_id = msg_ptr->node_id_;
	     peer_prop_info[node_id].has_replied_termination_ = true;
	     if(HasAllPeersAckedTerm(peer_prop_info) 
		&& have_replied_prop_pair_term){
	       receiver_ptr->state_ = TERM;
	       delete[] pollitems;
	       VLOG(0) << "**********Receiver exiting from EPRTerminateACK!";
	       return NULL;
	     }
	   }
	   break;
	 }
	 continue;
      }
    }
  }
  LOG(FATAL) << "Error!! Receiver exiting!";
  return 0;

}

}
