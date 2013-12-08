#include <glog/logging.h>

#include "quiltdb.hpp"

namespace quiltdb {

// Inproc end points for zmq:
// shared use between propagator and receiver
const char *KHPROP_UPDATE_PULL_ENDP = "inproc://hprop_update_pull_endp";
// reciever enp
const char *KHINTERNAL_RECV_PULL_ENDP = "inproc://hinternal_recv_pull_endp";
const char *KHINTERNAL_PAIR_RECV_PUSH_ENDP 
= "inproc://hinternal_pair_recv_push_endp";
// propagator push, receiver pull
const char *KHINTERNAL_PAIR_P2R_ENDP = KHINTERNAL_RECV_PULL_ENDP;
// propagator pull, receiver push
const char *KHINTERNAL_PAIR_R2P_ENDP = KHINTERNAL_PAIR_RECV_PUSH_ENDP;

// the same group of end points for vertical direction
const char *KVPROP_UPDATE_PULL_ENDP = "inproc://vprop_update_pull_endp";
// reciever enp
const char *KVINTERNAL_RECV_PULL_ENDP = "inproc://vinternal_recv_pull_endp";
const char *KVINTERNAL_PAIR_RECV_PUSH_ENDP 
= "inproc://vinternal_pair_recv_push_endp";
// propagator push, receiver pull
const char *KVINTERNAL_PAIR_P2R_ENDP = KVINTERNAL_RECV_PULL_ENDP;
// propagator pull, receiver push
const char *KVINTERNAL_PAIR_R2P_ENDP = KVINTERNAL_PAIR_RECV_PUSH_ENDP;

QuiltDB::QuiltDB(DBConfig &_dbconfig):
  config_(_dbconfig),
  started_(false),
  errcode_(0),
  zmq_ctx_(0){}

QuiltDB::~QuiltDB(){

  boost::unordered_map<int32_t, InternalTable*>::iterator table_itr;
  for(table_itr = table_dir_.begin(); table_itr != table_dir_.end(); 
      table_itr++){
    delete table_itr->second;
    table_itr->second = 0;
  }    
  if(zmq_ctx_ != 0) delete zmq_ctx_;
}

QuiltDB &QuiltDB::CreateQuiltDB(DBConfig &_dbconfig){
  static QuiltDB db(_dbconfig);
  try{
    db.zmq_ctx_ = new zmq::context_t(1);
  }catch(...){
    VLOG(0) << "Create zmq_ctx failed";
    db.errcode_ = 1;
  }
  return db;
}

Table QuiltDB::CreateHTable(int32_t _table_id, const TableConfig &_table_config){
  Table table;
  if(errcode_) return table;
  
  int ret = CreateTable(_table_id, _table_config, &table);
  CHECK_EQ(ret, 0) << "CreateTable Failed";

  hpropagator_.RegisterTable(_table_id, table.internal_table_->get_vadd_func(),
			     table.internal_table_->get_vsize(), 
			     _table_config.loop_, _table_config.apply_updates_);
  table.internal_table_->set_propagator(&hpropagator_);

  hreceiver_.RegisterTable(table.internal_table_);
  VLOG(0) << "successfully created htable " << _table_id;
  // TODO: register table with receiver
  return table;
}

Table QuiltDB::CreateVTable(int32_t _table_id, const TableConfig &_table_config){
  Table table;
  if(errcode_) return table;

  int ret = CreateTable(_table_id, _table_config, &table);
  
  vpropagator_.RegisterTable(_table_id, table.internal_table_->get_vadd_func(),
			     table.internal_table_->get_vsize(),
			     _table_config.loop_, _table_config.apply_updates_);
  table.internal_table_->set_propagator(&vpropagator_);
  // TODO: register table with receiver
  return table;
}

int QuiltDB::Start(){
  if(errcode_) return -1;
 
  sem_t sync_sem;
  sem_init(&sync_sem, 0, 0);

  started_ = true;
  PropagatorConfig propagator_config;
  propagator_config.my_id_ = config_.my_id_;
  propagator_config.nanosec_ = config_.hbatch_nanosec_;
  propagator_config.zmq_ctx_ = zmq_ctx_;
  propagator_config.update_pull_endp_ = KHPROP_UPDATE_PULL_ENDP;
  propagator_config.internal_pair_p2r_endp_ = KHINTERNAL_PAIR_P2R_ENDP;
  propagator_config.internal_pair_r2p_endp_ = KHINTERNAL_PAIR_R2P_ENDP;

  hpropagator_.Start(propagator_config, &sync_sem);
  VLOG(0) << "successfully called Start on hpropagator";

  ReceiverConfig receiver_config;
  receiver_config.my_id_ = config_.my_id_;
  receiver_config.num_expected_propagators_ = config_.hexpected_prop_;
  receiver_config.zmq_ctx_ = zmq_ctx_;
  receiver_config.update_push_endp_ = KHPROP_UPDATE_PULL_ENDP;
  receiver_config.internal_recv_pull_endp_ = KHINTERNAL_RECV_PULL_ENDP;
  receiver_config.internal_pair_recv_push_endp_ 
    = KHINTERNAL_PAIR_RECV_PUSH_ENDP;
  
  hreceiver_.Start(receiver_config, &sync_sem);

  //propagator_config.update_pull_endp_ = "inproc://vprop_update_pull_endp";
  //propagator_config.recv_pull_endp_ = "inproc://vrecv_pull_endp";

  //hpropagator_.Start(propagator_config, &sync_sem);
  //VLOG(0) << "successfully called Start on vpropagator";
  
  sem_wait(&sync_sem);
  sem_wait(&sync_sem);
  
  VLOG(0) << "successfully started";
  sem_destroy(&sync_sem);

  return 0;
}

int QuiltDB::RegisterThr(){
  if(hpropagator_.RegisterThr() < 0) return -1;
  //if(vpropagator_.RegisterThr() < 0) return -1;
  return 0;
}

int QuiltDB::DeregisterThr(){
  if(hpropagator_.DeregisterThr() < 0) return -1;
  //if(vpropagator_.DeregisterThr() < 0) return -1;
  return 0;
}


int QuiltDB::ShutDown(){
  int ret = hpropagator_.SignalTerm();
  CHECK_EQ(ret, 0) << "hpropagator_.SignalTerm() failed";
  ret = hreceiver_.SignalTerm();
  CHECK_EQ(ret, 0) << "hreceiver_.SignalTerm() failed";

  //vpropagator_.SignalTerm();
  ret = hpropagator_.WaitTerm();
  CHECK_EQ(ret, 0) << "hpropagator_.WaitTerm() failed";
  ret = hreceiver_.WaitTerm();
  CHECK_EQ(ret, 0) << "hreceiver_.WaitTerm() failed";

  //vpropagator_.WaitTerm();
  return 0;
}

int QuiltDB::CreateTable(int32_t _table_id, const TableConfig &_table_config, 
			 Table *_table){
  boost::unordered_map<int32_t, InternalTable*>::const_iterator table_iter
    = table_dir_.find(_table_id);
  if(table_iter != table_dir_.end())  return -1;
  
  InternalTable *itable = new InternalTable(_table_id, _table_config);
  table_dir_[_table_id] = itable;
  _table->internal_table_ = itable;
  return 0;
}

}
