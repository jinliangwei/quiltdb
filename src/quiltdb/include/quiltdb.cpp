
#include "quiltdb.hpp"

namespace quiltdb {

QuiltDB::QuiltDB(DBConfig &_dbconfig):
  config_(_dbconfig){}

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


int QuiltDB::ShutDown(){
  return 0;
}

}
