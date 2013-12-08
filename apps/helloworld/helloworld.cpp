
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
  dbconfig.hexpected_prop_ = 0;
  dbconfig.vexpected_prop_ = 0;
  dbconfig.hbatch_nanosec_ = 500000;
  dbconfig.vbatch_nanosec_ = 500000; // 500 micro second

  quiltdb::QuiltDB &db = quiltdb::QuiltDB::CreateQuiltDB(dbconfig);

  quiltdb::TableConfig tconfig;
  tconfig.vsize_ = sizeof(int);
  tconfig.vadd_func_ = IntAdd;
  tconfig.vsub_func_ = IntSub;
  tconfig.loop_ = true;
  tconfig.apply_updates_ = true;
  tconfig.user_cbk_ = false;

  quiltdb::Table htable = db.CreateHTable(0, tconfig);
  //quitdb::Table vtable = db.CreateVTable(1, tconfig);
  
  db.Start();
  int ret = db.RegisterThr();
  assert(ret == 0);

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
  
  ret = nanosleep(&req, &rem);

  db.ShutDown();
  db.DeregisterThr();
  assert(ret == 0);
  return 0;
}
