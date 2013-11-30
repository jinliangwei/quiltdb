#include <iostream>
#include "quiltdb.hpp"

namespace quiltdb {

QuiltDB::QuiltDB(DBConfig &_dbconfig):
  config_(_dbconfig),
  started_(false){}

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
  return db;
}

Table QuiltDB::CreateHTable(int32_t _table_id, const TableConfig &_table_config){
  Table table;
  int ret = CreateTable(_table_id, _table_config, &table);
  
  // TODO: register table with propagator and receiver 
  return table;
}

Table QuiltDB::CreateVTable(int32_t _table_id, const TableConfig &_table_config){
  Table table;
  int ret = CreateTable(_table_id, _table_config, &table);
  
  // TODO: register table with propagator and receiver
  return table;
}

int QuiltDB::Start(){
  started_ = true;
  return 0;
}


int QuiltDB::ShutDown(){
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
