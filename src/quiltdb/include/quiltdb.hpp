#ifndef __QUILTDB_HPP__
#define __QUILTDB_HPP__

#include <boost/unordered_map.hpp>
#include "table.hpp"
#include <quiltdb/utils/config_parser.hpp>
#include <quiltdb/utils/memstruct.hpp>

#include <quiltdb/comm/propagator.hpp>
#include <quiltdb/comm/receiver.hpp>
#include <string>

namespace quiltdb {

struct DBConfig{
  int32_t my_hid_;
  NodeInfo my_hrecv_info_;
  NodeInfo hnode_prop_downstream_;
  int32_t hexpected_prop_;
  int32_t my_vid_;
  NodeInfo my_vrecv_info_;
  NodeInfo vnode_prop_downstream_;
  int32_t vexpected_prop_;

  // How long should horizontal and vertical propagation thread should 
  // wait between propagation.
  // <= 0 means no waiting.
  int32_t hbatch_nanosec_;
  int32_t vbatch_nanosec_;
  
};

class QuiltDB {

  // Public functions are not concurrent.

public:
  // make QuiltDB a singleton
  static QuiltDB &CreateQuiltDB(DBConfig &_dbconfig);
  
  Table CreateHTable(int32_t _table_id, const TableConfig &_table_config);
  Table CreateVTable(int32_t _table_id, const TableConfig &_table_config);

  int Start();
  
  // needed for threads that accesses table API (actually just Inc) and ShutDown
  int RegisterThr();
  
  // ShutDown() blocks until QuiltDB completes proper ShutDown protocol with its
  // neighbors (essentially, its neighbors have called ShutDown too). After it 
  // returns, the process may exit anytime.
  // The application is responsible for determining the termination condition as
  // when ShutDown() is called, the message ring may be broken as soon as all 
  // neighbors call ShutDown().
  // must be called when that thread is registered
  int ShutDown();

  // Deregister must be called by threads that have registered
  int DeregisterThr();

private:

  int CreateTable(int32_t _table_id, const TableConfig &_table_config, 
		    Table *_table);

  QuiltDB(DBConfig &_dbconfig);
  ~QuiltDB();
  
  Propagator hpropagator_;
  Propagator vpropagator_;
  Receiver hreceiver_;
  Receiver vreceiver_;

  DBConfig config_;
  bool started_;
  int errcode_;
  boost::unordered_map<int32_t, InternalTable*> table_dir_;
  zmq::context_t *zmq_ctx_;
};

}

#endif
