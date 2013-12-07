#include "receiver.hpp"
#include "quiltdb/utils/memstruct.hpp"
#include "quiltdb/utils/zmq_util.hpp"

#include <boost/unordered_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/tss.hpp>
#include <boost/shared_array.hpp>
#include <queue>
#include <zmq.hpp>

namespace quiltdb {

Receiver::Receiver(){}

int Receiver::Start(ReceiverConfig &_config, sem_t *sync_sem){


  return 0;
}

int Receiver::RegisterTable(InternalTable *_itable){
  return 0;
}

int Receiver::SignalTerm(){
  return 0;
}

int Receiver::WaitTerm(){
  return 0;
}

void *Receiver::ReceiverThrMain(void *_argu){

  ReceiverThrInfo *thrinfo = reinterpret_cast<ReceiverThrInfo*>(_argu);
  zmq::context_t *zmq_ctx = thrinfo->zmq_ctx_;
  int32_t my_id = thrinfo->my_id_;
  int32_t num_expected_propagators = thrinfo->num_expected_propagators_;
  int32_t internal_pair_endp = thrinfo->internal_pair_endp_;

  boost::unordered_map<int32_t, std::queue<UpdateBuffer*> > my_updates;
  
  // TCP sockets
  // receive updates from propagator
  boost::scoped_ptr<zmq::socket_t> update_pull_sock;
  // PUSH termination message to propagator
  boost::scoped_ptr<zmq::socket_t> term_push_sock;
  
  // inproc sockets
  // for communicating my own updates, also pull termination message
  boost::scoped_ptr<zmq::socket_t> internal_recv_pull_sock;
  boost::scoped_ptr<zmq::socket_t> internal_pair_recv_push_sock;
  // push received updates to propagator
  boost::scoped_ptr<zmq::socket_t> internal_update_push_sock;

  bool have_received_prop_pair_term = false;

  try{
    //update_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
    //term_push_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));

    internal_recv_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
    internal_recv_pull_sock->connect(thrinfo->internal_recv_pull_endp_.c_str());

    internal_pair_recv_push_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
    internal_pair_recv_push_sock->connect(thrinfo->internal_pair_recv_push_endp_.c_str());

    internal_update_push_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
    internal_update_push_sock->connect(thrinfo->update_push_endp_.c_str());

  }catch(zmq::error_t &e){
    LOG(FATAL) << "Failed initializing sockets, error: " << e.what();
  }catch(...){
    LOG(FATAL) << "Failed setting up sockets";
  }

  //TODO: received connections from expected number of propagators
  // Recv from update_pull_sock, one message per propagator
  // Send message on term_push_sock one per propagator
  
  PropagatorMsgtype init_msg = EPRInit;
  int ret = SendMsg(*internal_prop_recv_pair_sock, (uint8_t *) &init_msg, 
		    sizeof(PropagatorMsgType));
  CHECK_EQ(ret, sizeof(PropagatorMsgType)) 
    << "Send on internal_prop_recv_pair_sock failed, ret = " << ret;
  sem_post(thrinfo->sync_sem_);
  
  // TODO: adjust number when TCP sockets are set up
  int num_poll_sock = 1;
  zmq::pollitem_t *pollitems = new zmq::pollitem_t[num_poll_sock];
  pollitems[0].socket = *internal_recv_pull_sock;
  pollitems[0].events = ZMQ_POLLIN;
  //pollitems[1].socket = *update_pull_sock;
  //pollitems[1].events = ZMQ_POLLIN;
  
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
      if(len <= 0){
	LOG(FATAL) << "receiver thread read message type failed, "
		   << "error \nPROCESS EXIT!";
      }
    
      msgtype = *(reinterpret_cast<ReceiverMsgType*>(data.get()));
      switch(msgtype){
      case EMyUpdate:
	{
	  
	  CHECK_EQ(len, sizeof(MyUpdateMsg)) << "malformed MyUpdateMsg";
	  
	  MyUpdatesMsg *myupdate_msg 
	    = reinterpret_cast<MyUpdatesMsg*>(data.get());
	  int32_t table_id = myupdate_msg->table_id_;
	  UpdateBuffer *update_buff_ptr = myupdate_msg->update_buff_ptr_;
	  boost::unordered_map<int32_t, std::queue<UpdateBuffer*> >::iterator
	    table_update_iter = my_updates.find(table_id);
	  if(table_update_iter == my_updates.end()){
	    std::queue<UpdateBuffer*> update_buff_q;
	    my_updates[table_id] = update_buff_q;
	  }
	  
	  PropagatorMsgType myupdates_ack = MyUpdateACK;

	  int ret = SendMsg(*internal_update_push_sock, *myupdates_ack, 
			    sizeof(PropagatorMsgType));
	  CHECK_EQ(ret, sizeof(PropagatorMsgType)) << "send MyUpdateACK failed";

	}
	break;
      case ERInternalTerminate:
	{
	  
	  
	}
	break;
      case EPRInternalTerminate:
	{
	  
	}
	break;
      default:
	LOG(FATAL) << "received unrecognized message type " << msgtype;
      }
      
      continue;
    }
    
    /*
    if(pollitems[1].revents){
      
      continue;
      }*/

  }
  
  return 0;

}

}
