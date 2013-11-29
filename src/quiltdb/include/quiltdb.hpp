#ifndef __QUILTDB_HPP__
#define __QUILTDB_HPP__

#include <boost/unordered_map.hpp>
#include "table.hpp"

#include <quiltdb/propagator/propagator.hpp>
#include <quiltdb/receiver/receiver.hpp>
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
  int32_t hbatch_nano_sec_;
  int32_t vbatch_nano_sec_;

};

class QuiltDB {

  // Public functions are not concurrent.

public:
  // make QuiltDB a singleton
  static QuiltDB &CreateQuiltDB(DBConfig &_dbconfig);
  
  Table CreateHTable(int32_t _table_id, const TableConfig &_table_config);
  Table CreateVTable(int32_t _table_id, const TableConfig &_table_config);

  int Start();
  int ShutDown();

private:

  QuiltDB(DBConfig &_dbconfig);

  Propagator hpropagator_;
  Propagator vpropagator_;
  Receiver hreceiver_;
  Receiver vreceiver_;

  DBConfig config_;
  bool started_;
  
  // When Quite

};

}

#endif
