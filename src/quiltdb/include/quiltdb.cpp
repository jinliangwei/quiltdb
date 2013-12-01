#include <glog/logging.h>

#include "quiltdb.hpp"

namespace quiltdb {

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
  
  hpropagator_.RegisterTable(_table_id, table.internal_table_->get_vadd_func());
  table.internal_table_->set_propagator(&hpropagator_);
  VLOG(0) << "successfully created htable " << _table_id;
  // TODO: register table with receiver
  return table;
}

Table QuiltDB::CreateVTable(int32_t _table_id, const TableConfig &_table_config){
  Table table;
  if(errcode_) return table;

  int ret = CreateTable(_table_id, _table_config, &table);
  
  vpropagator_.RegisterTable(_table_id, table.internal_table_->get_vadd_func());
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
  propagator_config.nanosec_ = config_.hbatch_nanosec_;
  propagator_config.zmq_ctx_ = zmq_ctx_;
  propagator_config.update_pull_endp_ = "inproc://hprop_update_pull_endp";
  propagator_config.recv_pull_endp_ = "inproc://hrecv_pull_endp";

  hpropagator_.Start(propagator_config, &sync_sem);
  VLOG(0) << "successfully called Start on hpropagator";

  //propagator_config.update_pull_endp_ = "inproc://vprop_update_pull_endp";
  //propagator_config.recv_pull_endp_ = "inproc://vrecv_pull_endp";

  //hpropagator_.Start(propagator_config, &sync_sem);
  //VLOG(0) << "successfully called Start on vpropagator";
  
  sem_wait(&sync_sem);
  //sem_wait(&sync_sem);
  
  VLOG(0) << "successfully started";
  sem_destroy(&sync_sem);

  return 0;
}


int QuiltDB::ShutDown(){
  int ret = hpropagator_.SignalTerm();
  if(ret < 0){
    VLOG(0) << "hpropagator_.SignalTerm() failed";
  }
  //vpropagator_.SignalTerm();
  ret = hpropagator_.WaitTerm();
  if(ret < 0){
    VLOG(0) << "hpropagator_.WaitTerm() failed";
  }
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
