#ifndef __QUILTDB_TABLE_HPP__
#define __QUILTDB_TABLE_HPP__

#include <stdint.h>
#include <boost/shared_ptr.hpp>

#include <quiltdb/internal_table/internal_table.hpp>

namespace quiltdb {

class QuiltDB;

class Table {
public:

  // Public functions in this class are all concurrent.
  
  template<typename ValueType>
  ValueType Get(int64_t _key){
    return internal_table_->Get<ValueType>(_key);
  }
  
  // do vadd_func_(value, _delta);
  template<typename ValueType>
  void Inc(int64_t _key, ValueType _delta){
    internal_table_->Inc<ValueType>(_key, _delta);
  }

  int32_t GetID(){
    return internal_table_->GetID();
  }

private:
  // called by QuiltDB to create Table
  Table(int32_t _table_id, const TableConfig &_config){
    internal_table_.reset(new InternalTable(_table_id, _config));
  }

  boost::shared_ptr<InternalTable> internal_table_;

  friend class QuiltDB;
};

}

#endif
