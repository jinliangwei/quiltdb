#ifndef __QUILTDB_MEMSTRUCT_HPP__
#define __QUILTDB_MEMSTRUCT_HPP__

#include <stdint.h>

namespace quiltdb {

class UpdateBuffer{
public:

  static int32_t GetBuffSize(int32_t _update_size,
			     int32_t _update_capacity, 
			     int32_t _node_range_capacity);
  static uint8_t *AllocateBufferMemory(int32_t _buff_size);  
  static UpdateBuffer *CreateUpdateBuffer(int32_t _update_size,
					  int32_t _update_capacity, 
					  int32_t _node_range_capacity);
  static int DestroyUpdateBuffer(UpdateBuffer *_buffer);

  // has to use placement new
  // @param _num_bytes: number of bytes per update.
  UpdateBuffer(int32_t _buff_size, int32_t _update_size,
	       int32_t _update_capacity, int32_t _node_range_capacity);
  ~UpdateBuffer(){}

  int AppendUpdate(int64_t _key, const uint8_t *_update); 
  int StartIteration();
  const uint8_t *NextUpdate(int64_t *key);
  
  int UpdateNodeRange(int32_t _node_id, int64_t _key_st, int64_t _key_end);
  bool GetNodeRange(int32_t _node_id, int64_t *_key_st, int64_t *_key_end);

  int32_t get_update_size();

private:
  int32_t buff_size_;
  int32_t update_size_; //number of bytes
  int32_t update_capacity_; // number of updates that the physical space 
                            // may hold
  int32_t node_range_capacity_;


  int32_t num_updates_occupied_; // the number of update space that are 
                                 // currently ocuupied  
  // node infomation
  int32_t node_range_st_offset_;
  int32_t num_nodes_;

  int32_t update_st_offset_;
  int32_t update_end_offset_; // offset pointing to the next empty update slot
  int32_t update_iter_offset_;
};

}

#endif
