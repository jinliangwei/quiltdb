#include "memstruct.hpp"

#include <cstddef>

namespace quiltdb {

UpdateBuffer::UpdateBuffer(int32_t _num_bytes, int32_t _num_updates){}

int UpdateBuffer::AppendUpdate(int32_t _seq_num, const uint8_t *_update){
  return 0;
}

const uint8_t *UpdateBuffer::NextUpdate(){
  return 0;
}

int32_t UpdateBuffer::GetUpdateSize(){
  return 0;
}

int32_t UpdateBuffer::GetBuffSize(int32_t _num_bytes, int32_t _num_updates){
  return 0;
}

}
