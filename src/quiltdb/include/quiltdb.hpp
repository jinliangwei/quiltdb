#ifndef __QUILTDB_HPP__
#define __QUILTDB_HPP__

#include <boost/unordered_map.hpp>
#include "table.hpp"

#include <quiltdb/comm/propagator.hpp>
#include <quiltdb/comm/receiver.hpp>
#include <string>

namespace quiltdb {

struct DBConfig{
  int32_t my_id_;
  std::string my_ip_;
  std::string hport_prop_;
  std::string vport_prop_;

  NodeInfo hnode_info_; // the horizontal downstream node
  NodeInfo vnode_info_;

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

  
  // ShutDown() blocks until QuiltDB completes proper ShutDown protocol with its
  // neighbors (essentially, its neighbors have called ShutDown too). After it 
  // returns, the process may exit anytime.
  // The application is responsible for determining the termination condition as
  // when ShutDown() is called, the message ring may be broken as soon as all 
  // neighbors call ShutDown().
  int ShutDown();

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
