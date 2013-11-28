#ifndef __QUILTDB_PROPAGATOR_HPP__
#define __QUILTDB_PROPAGATOR_HPP__

#include <quiltdb/include/common.hpp>
#include <quiltdb/utils/memstruct.hpp>

#include <zmq.hpp>
#include <stdint.h>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <semaphore.h>

namespace quiltdb {

struct PropagatorConfig {
  int32_t interval_;
  NodeInfo node_info_;
  zmq::context_t *zmq_ctx_;

};

class Propagator : boost::noncopyable {

public:
  Propagator();
  int Start(PropagatorConfig &_config, sem_t *sync_sem);
  int RegisterTable(int32_t _table_id);
  int Inc(int32_t _table_id, int32_t _key, const uint8_t *_delta, 
	  int32_t _num_bytes);
  int ApplyUpdates(int32_t _table_id, UpdateBuffer *_updates);
  int Stop();

private:
  static bool TimerHander(void * _propagator);
  boost::unordered_map<int32_t, boost::unordered_map<int64_t, uint8_t* > > 
  update_store_;
  
  boost::scoped_ptr<zmq::socket_t> prop_push_sock_;
  boost::scoped_ptr<zmq::socket_t> stop_pull_sock_;
  boost::scoped_ptr<zmq::socket_t> timer_pull_sock_;
  
};

}

#endif
