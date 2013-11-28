
#include <quiltdb/include/quiltdb.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>

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
  quiltdb::DBConfig dbconfig;
  dbconfig.my_id_ = 0;

  quiltdb::QuiltDB &db = quiltdb::QuiltDB::CreateQuiltDB(dbconfig);

  quiltdb::TableConfig tconfig;
  tconfig.vsize_ = sizeof(int);
  tconfig.vadd_func_ = IntAdd;
  tconfig.vsub_func_ = IntSub;

  quiltdb::Table htable = db.CreateHTable(0, tconfig);
  //quitdb::Table vtable = db.CreateVTable(1, tconfig);

  int a = htable.Get<int>(10);

  std::cout << "a = " << a << std::endl;

  htable.Inc<int>(10, 2);
  a = htable.Get<int>(10);
  std::cout << "a = " << a << std::endl;
  
  db.ShutDown();
  return 0;
}
