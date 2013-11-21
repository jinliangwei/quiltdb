#ifndef __QUILTDB_MEMSTRUCT_HPP__
#define __QUILTDB_MEMSTRUCT_HPP__

#include <stdint.h>

namespace quiltdb {

class UpdateBuffer{
public:
  // has to use placement new
  // @param _num_bytes: number of bytes per update.
  UpdateBuffer(int32_t _num_bytes, int32_t _num_updates);
  ~UpdateBuffer();

  int AppendUpdate(int32_t _seq_num, const uint8_t *_update); 
  const uint8_t *NextUpdate();
  int32_t GetUpdateSize();

  static int32_t GetBuffSize(int32_t _num_bytes, int32_t _num_updates);

private:
  int32_t buff_size_;
  int32_t num_updates_; // number of updates in the buffer
  int32_t update_st_offset_;
  int32_t update_end_offset_;
  int32_t iter_offset_;

  int32_t update_size_; //number of bytes
  int32_t update_total_size_;
};

}

#endif
