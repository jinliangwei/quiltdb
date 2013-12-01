
#include <quiltdb/include/quiltdb.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <sys/time.h>

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
  google::InitGoogleLogging(argv[0]);

  quiltdb::DBConfig dbconfig;
  dbconfig.my_id_ = 0;
  dbconfig.hbatch_nanosec_ = 500000;
  dbconfig.vbatch_nanosec_ = 500000; // 500 micro second

  quiltdb::QuiltDB &db = quiltdb::QuiltDB::CreateQuiltDB(dbconfig);

  quiltdb::TableConfig tconfig;
  tconfig.vsize_ = sizeof(int);
  tconfig.vadd_func_ = IntAdd;
  tconfig.vsub_func_ = IntSub;

  quiltdb::Table htable = db.CreateHTable(0, tconfig);
  //quitdb::Table vtable = db.CreateVTable(1, tconfig);
  
  db.Start();

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
  
  int ret = nanosleep(&req, &rem);

  db.ShutDown();
  return 0;
}
