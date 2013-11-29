
#include "quiltdb.hpp"

namespace quiltdb {

QuiltDB::QuiltDB(DBConfig &_dbconfig):
  config_(_dbconfig),
  started_(false){}

QuiltDB &QuiltDB::CreateQuiltDB(DBConfig &_dbconfig){
  static QuiltDB db(_dbconfig);
  return db;
}

Table QuiltDB::CreateHTable(int32_t _table_id, const TableConfig &_table_config){
  Table table(_table_id, _table_config);
  return table;
}

Table QuiltDB::CreateVTable(int32_t _table_id, const TableConfig &_table_config){
  Table table(_table_id, _table_config);
  return table;
}

int QuiltDB::Start(){
  started_ = true;
  return 0;
}

int QuiltDB::ShutDown(){
  return 0;
}

}
