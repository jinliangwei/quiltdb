#ifndef __QUILTDB_TABLE_HPP__
#define __QUILTDB_TABLE_HPP__

#include <stdint.h>

#include <quiltdb/internal_table/internal_table.hpp>

namespace quiltdb {

class QuiltDB;

// Allow copying.
class Table {
public:

  // Public functions in this class are all concurrent.
  template<typename ValueType>
  ValueType Get(int64_t _key){
    std::cout << "here";
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

  Table(){}

  Table(const Table &_table):
    internal_table_(_table.internal_table_){}

  Table operator=(const Table &_table){
    this->internal_table_ = _table.internal_table_;
  }

private:

  InternalTable *internal_table_;
  friend class QuiltDB;
};

}

#endif
