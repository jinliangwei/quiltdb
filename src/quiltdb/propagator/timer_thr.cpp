#include "timer_thr.hpp"

namespace quiltdb {

NanoTimer::NanoTimer(zmq::context_t *_zmq_ctx){}

int NanoTimer::Start(int32_t _interval, TimerHandler _handler, 
		     void *_handler_argu){
  return 0;
}

int Stop(){
  return 0;
}

}
