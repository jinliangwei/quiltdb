#ifndef __QUILTDB_TABLE_HPP__
#define __QUILTDB_TABLE_HPP__

#include <stdint.h>
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
  void Inc(ValueType _delta){
    internal_table_->Inc<ValueType>(_delta);
  }

  int32_t GetID(){
    return internal_table->GetID();
  }

private:
  // called by QuiltDB to create Table
  Table(InternalTable *_internal_table):
    internal_table_(_internal_table);

  InternalTable *internal_table_;

  friend class QuiltDB;
};

}

#endif
