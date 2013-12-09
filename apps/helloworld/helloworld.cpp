
#include <quiltdb/include/quiltdb.hpp>

#include <gflags/gflags.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <sys/time.h>

DEFINE_string(config_file, "", "configuration file");
DEFINE_int32(myhid, 0, "my h id");
DEFINE_int32(myvid, 1, "my v id");

int IntAdd(uint8_t *_v, uint8_t *_delta, int32_t _vsize){
  if(_vsize != sizeof(int)) return -1;

  int &v = *(reinterpret_cast<int *>(_v));
  int delta = *(reinterpret_cast<int *>(_delta));

  v += delta;
  return 0;
}

int IntSub(uint8_t *_v, uint8_t *_delta, int32_t _vsize){
  if(_vsize != sizeof(int)) return -1;

  int &v = *(reinterpret_cast<int *>(_v));
  int delta = *(reinterpret_cast<int *>(_delta));

  v -= delta;
  return 0;
}

int main(int argc, char *argv[]){
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  quiltdb::ConfigParser config_parser;
  config_parser.LoadConfigFile(FLAGS_config_file);

  int32_t myhid = FLAGS_myhid;
  int32_t myvid = FLAGS_myvid;
  // Get H node configuration
  quiltdb::NodeConfig my_hconfig = config_parser.GetNodeInfo(myhid);
  CHECK_EQ(my_hconfig.node_info_.node_id_, myhid) 
    << "Failed to find id for myself id = " << myhid;

  int32_t h_downstream_id = my_hconfig.downstream_recv_;
  quiltdb::NodeConfig h_downstream_config 
    = config_parser.GetNodeInfo(h_downstream_id);
  
  CHECK_EQ(h_downstream_config.node_info_.node_id_, h_downstream_id) 
    << "Failed to find id for h downstream config id = " << h_downstream_id;

  // Get V node configuration
  quiltdb::NodeConfig my_vconfig = config_parser.GetNodeInfo(myvid);
  CHECK_EQ(my_vconfig.node_info_.node_id_, myvid) 
    << "Failed to find id for myself id = " << myvid;

  int32_t v_downstream_id = my_vconfig.downstream_recv_;
  quiltdb::NodeConfig v_downstream_config 
    = config_parser.GetNodeInfo(v_downstream_id);
  
  CHECK_EQ(v_downstream_config.node_info_.node_id_, v_downstream_id) 
    << "Failed to find id for h downstream config id = " << v_downstream_id;

  quiltdb::DBConfig dbconfig;
  dbconfig.my_hid_ = myhid;
  dbconfig.my_hrecv_info_ = my_hconfig.node_info_;
  dbconfig.hnode_prop_downstream_ = h_downstream_config.node_info_;
  dbconfig.hexpected_prop_ = my_hconfig.num_expected_props_;

  dbconfig.my_vid_ = myvid;
  dbconfig.my_vrecv_info_ = my_vconfig.node_info_;
  dbconfig.vnode_prop_downstream_ = v_downstream_config.node_info_;
  dbconfig.vexpected_prop_ = my_vconfig.num_expected_props_;

  dbconfig.hbatch_nanosec_ = 500000;
  dbconfig.vbatch_nanosec_ = 500000; // 500 micro second

  quiltdb::QuiltDB &db = quiltdb::QuiltDB::CreateQuiltDB(dbconfig);

  quiltdb::TableConfig tconfig;
  tconfig.vsize_ = sizeof(int);
  tconfig.vadd_func_ = IntAdd;
  tconfig.vsub_func_ = IntSub;
  tconfig.loop_ = true;
  tconfig.apply_updates_ = true;
  tconfig.user_cbk_ = false;

  quiltdb::Table htable = db.CreateHTable(0, tconfig);
  //quitdb::Table vtable = db.CreateVTable(1, tconfig);

  db.Start();
  int ret = db.RegisterThr();
  assert(ret == 0);

  int a = htable.Get<int>(10);

  std::cout << "a = " << a << std::endl;

  htable.Inc<int>(10, 2);
  a = htable.Get<int>(10);
  std::cout << "a = " << a << std::endl;
  htable.Inc<int>(8, 20);
  htable.Inc<int>(32, 12);


  timespec req;
  req.tv_sec = 0;
  req.tv_nsec = 5000000;
  timespec rem;
  
  ret = nanosleep(&req, &rem);

  db.ShutDown();
  db.DeregisterThr();
  assert(ret == 0);
  return 0;
}
