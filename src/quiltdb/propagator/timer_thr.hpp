#ifndef __QUILTDB_TIMER_THR_HPP__
#define __QUILTDB_TIMER_THR_HPP__

#include <stdint.h>
#include <time.h>
#include <zmq.hpp>

namespace quiltdb {

class NanoTimer{

  // true if time continue, false if timer stop
typedef bool (*TimerHandler)(void *);

public:
  NanoTimer(zmq::context_t *_zmq_ctx);

  int Start(int32_t _interval, TimerHandler _handler, void *_handler_argu);

  int Stop();

};

}

#endif
