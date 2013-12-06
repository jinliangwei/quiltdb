#include "receiver.hpp"
#include "quiltdb/utils/memstruct.hpp"

#include <boost/unordered_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/tss.hpp>
#include <boost/shared_array.hpp>
#include <vector>
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

int Receiver::PropagateUpdates(int32_t _table_id, UpdateBuffer *_updates){
  return 0;
}

void *Receiver::ReceiverThrMain(void *_argu){

  ReceiverThrInfo *thrinfo = reinterpret_cast<ReceiverThrInfo*>(_argu);
  zmq::context_t *zmq_ctx = thrinfo->zmq_ctx_;
  int32_t my_id = thrinfo->my_id_;
  int32_t num_expected_propagators = thrinfo->num_expected_propagators_;
  int32_t internal_pair_endp = thrinfo->internal_pair_endp_;

  boost::unordered_map<int32_t, std::vector<UpdateBuffer*> > my_updates;
  
  // TCP sockets
  // receive updates from propagator
  boost::scoped_ptr<zmq::socket_t> recv_pull_sock;
  // PUSH termination message to propagator
  boost::scoped_ptr<zmq::socket_t> recv_push_sock;
  
  // inproc sockets
  // for communicating my own updates
  boost::scoped_ptr<zmq::socket_t> internal_prop_recv_pair_sock;
  // push received updates to propagator
  boost::scoped_ptr<zmq::socket_t> internal_recv_push_sock;

  bool have_received_prop_pair_term = false;

  try{
    //recv_pull_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PULL));
    //recv_push_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));

    internal_prop_recv_pair_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PAIR));
    internal_prop_recv_pair_sock->connect(thrinfo->internal_pair_endp_.c_str());

    internal_recv_push_sock.reset(new zmq::socket_t(*zmq_ctx, ZMQ_PUSH));
    internal_recv_push_sock->bind(thrinfo->update_push_endp_.c_str());

  }catch(zmq::error_t &e){
    LOG(FATAL) << "Failed initializing sockets, error: " << e.what();
  }catch(...){
    LOG(FATAL) << "Failed setting up sockets";
  }

  //TODO: received connections from expected number of propagators
  boost::shared_array<uint8_t> data;
  PropagatorMsgtype init_msg = EPRInit;
  SendMsg(*internal_prop_recv_pair_sock, (uint8_t*) &init_msg, 
	  sizeof(PropagatorMsgType));
  
  sem_post(thrinfo->
  
  return 0;

}

}
