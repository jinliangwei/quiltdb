#include "comm_util.hpp"

namespace quiltdb {

int InitThrSockIfHaveNot(boost::thread_specific_ptr<zmq::socket_t> *_sock,
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

int InitScopedSockIfHaveNot(boost::scoped_ptr<zmq::socket_t> *_sock,
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


}
