#include "receiver.hpp"
#include "quiltdb/utils/memstruct.hpp"

#include <boost/unordered_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/tss.hpp>
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

  try{
    
  }catch(zmq::error_t &e){

  }catch(...){

  }

  return 0;

}

}
