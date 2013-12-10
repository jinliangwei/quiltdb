#include "memstruct.hpp"

#include <cstddef>
#include <string.h>
#include <new> // for operator new
#include <glog/logging.h>
namespace quiltdb {

int32_t UpdateBuffer::GetBuffSize(int32_t _update_size, 
				  int32_t _update_capacity,
				  int32_t _node_range_capacity){
  
  // for each update, need 1 key + 1 delta
  // for each node range info, need 1 node id, 1 range st, 1 range end
  
  int32_t node_range_mem_size =  _node_range_capacity*(sizeof(int32_t) 
						       + sizeof(int64_t) 
						       + sizeof(int64_t));
  int32_t update_mem_size = _update_capacity*(sizeof(int64_t) + _update_size);
  
  return node_range_mem_size + update_mem_size + sizeof(UpdateBuffer);
}

uint8_t *UpdateBuffer::AllocateBufferMemory(int32_t _buff_size){

  uint8_t *mem = new uint8_t[_buff_size];
  
  return mem;
}

UpdateBuffer *UpdateBuffer::CreateUpdateBuffer(int32_t _update_size,
					       int32_t _update_capacity, 
					       int32_t _node_range_capacity){
  int32_t buff_size = GetBuffSize(_update_size, _update_capacity, 
				  _node_range_capacity);

  uint8_t *mem = AllocateBufferMemory(buff_size);

  UpdateBuffer *buffer = new (mem) UpdateBuffer(buff_size, _update_size,
						_update_capacity, 
						_node_range_capacity);
  return buffer;
}

int UpdateBuffer::DestroyUpdateBuffer(UpdateBuffer *_buffer){

  uint8_t *mem = reinterpret_cast<uint8_t*>(_buffer);
  delete[] mem;

}


UpdateBuffer::UpdateBuffer(int32_t _buff_size,
			   int32_t _update_size,
			   int32_t _update_capacity,
			   int32_t _node_range_capacity):
  buff_size_(_buff_size),
  update_size_(_update_size),
  update_capacity_(_update_capacity),
  node_range_capacity_(_node_range_capacity),
  num_updates_occupied_(0),
  node_range_st_offset_(sizeof(UpdateBuffer)),
  num_nodes_(0){
  
  VLOG(0) << "Create UpdateBuffer, node_range_capacity = "
	  << node_range_capacity_
	  << " update szie = " << update_size_;
  
  // initialize node range information
  uint8_t *node_range_ptr = reinterpret_cast<uint8_t*>(this) 
    + node_range_st_offset_;
  
  int32_t node_range_idx;
  for(node_range_idx = 0; node_range_idx < node_range_capacity_; 
      ++node_range_idx){

    int32_t *node_id_ptr = reinterpret_cast<int32_t*>(node_range_ptr);
    int64_t *node_range_st_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							    + sizeof(int32_t));
    int64_t *node_range_end_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							     + sizeof(int32_t)
							     + sizeof(int64_t));
    *node_id_ptr = -1;
    *node_range_st_ptr = -1;
    *node_range_end_ptr = -1;

    node_range_ptr += (sizeof(int32_t) + sizeof(int64_t) + sizeof(int64_t));
  }

  update_st_offset_ = sizeof(UpdateBuffer) 
    + node_range_capacity_*(sizeof(int32_t) 
			    + sizeof(int64_t) 
			    + sizeof(int64_t));
  
  update_end_offset_ = update_st_offset_;
  update_iter_offset_ = update_st_offset_;

}

int UpdateBuffer::AppendUpdate(int64_t _key, const uint8_t *_update){
  if(num_updates_occupied_ == update_capacity_) return -1;

  uint8_t *update_end_ptr = reinterpret_cast<uint8_t*>(this) 
    + update_end_offset_;

  VLOG(0) << "update_end_offset_ (before) = " << update_end_offset_;

  int64_t *key_ptr = reinterpret_cast<int64_t*>(update_end_ptr);
  *key_ptr = _key;
  
  memcpy(update_end_ptr + sizeof(int64_t), _update, update_size_);
  
  update_end_offset_ += (sizeof(int64_t) + update_size_);
  ++num_updates_occupied_;

  return 0;
}

int UpdateBuffer::StartIteration(){
  if(num_updates_occupied_ == 0) return -1;

  update_iter_offset_ = update_st_offset_;
  return 0;
}

uint8_t *UpdateBuffer::NextUpdate(int64_t *key){
  
  if(update_iter_offset_ >= update_end_offset_) return NULL;

  VLOG(0) << "update_iter_offset_ (before) = " << update_iter_offset_;

  uint8_t *update_iter_ptr = reinterpret_cast<uint8_t*>(this) 
    + update_iter_offset_;
  
  int64_t *key_ptr = reinterpret_cast<int64_t*>(update_iter_ptr);
  
  *key = *key_ptr;

  update_iter_offset_ += (sizeof(int64_t) + update_size_);
  VLOG(0) << "update_size_ = " << update_size_;
  VLOG(0) << "update_iter_offset_ (after) = " << update_iter_offset_;
  
  return (update_iter_ptr + sizeof(int64_t));
}

uint8_t *UpdateBuffer::GetUpdate(int64_t _key){
  uint8_t *update_ptr = reinterpret_cast<uint8_t*>(this) 
    + update_st_offset_;

  int32_t num_updates;

  for(num_updates = 0; num_updates < num_updates_occupied_; ++num_updates){
    int64_t *key_ptr = reinterpret_cast<int64_t*>(update_ptr);
    int64_t key = *key_ptr;
    if(key == _key){
      return (update_ptr + sizeof(int64_t));
    }
    update_ptr += (sizeof(int64_t) + update_size_);
  }
  return NULL;

}

int UpdateBuffer::UpdateNodeRange(int32_t _node_id, int64_t _key_st, 
			       int64_t _key_end){
  uint8_t *node_range_ptr = reinterpret_cast<uint8_t*>(this) 
    + node_range_st_offset_;

  int32_t node_range_idx;

  bool updated = false;
  
  for(node_range_idx = 0; node_range_idx < node_range_capacity_; 
      ++node_range_idx){

    int32_t *node_id_ptr = reinterpret_cast<int32_t*>(node_range_ptr);
    int64_t *node_range_st_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							    + sizeof(int32_t));
    int64_t *node_range_end_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							     + sizeof(int32_t)
							     + sizeof(int64_t));
    if(*node_id_ptr == _node_id){
      *node_range_st_ptr = _key_st;
      *node_range_end_ptr = _key_end;
      updated = true;
      break;
    }

    node_range_ptr += (sizeof(int32_t) + sizeof(int64_t) + sizeof(int64_t));
  }

  if(updated) return 0;

  node_range_ptr = reinterpret_cast<uint8_t*>(this) 
    + node_range_st_offset_;

  for(node_range_idx = 0; node_range_idx < node_range_capacity_; 
      ++node_range_idx){
        
    int32_t *node_id_ptr = reinterpret_cast<int32_t*>(node_range_ptr);
    int64_t *node_range_st_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							    + sizeof(int32_t));
    int64_t *node_range_end_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							     + sizeof(int32_t)
							     + sizeof(int64_t));
    if(*node_id_ptr == -1){
      *node_id_ptr = _node_id;
      *node_range_st_ptr = _key_st;
      *node_range_end_ptr = _key_end;
      updated = true;
      break;
    }
    node_range_ptr += (sizeof(int32_t) + sizeof(int64_t) + sizeof(int64_t));
  }
 
  if(!updated) return -1;
  return 0;
}

int UpdateBuffer::DeleteNodeRange(int32_t _node_id){

  uint8_t *node_range_ptr = reinterpret_cast<uint8_t*>(this) 
    + node_range_st_offset_;

  int32_t node_range_idx;

  bool found = false;
  
  for(node_range_idx = 0; node_range_idx < node_range_capacity_; 
      ++node_range_idx){

    int32_t *node_id_ptr = reinterpret_cast<int32_t*>(node_range_ptr);
    int64_t *node_range_st_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							    + sizeof(int32_t));
    int64_t *node_range_end_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							     + sizeof(int32_t)
							     + sizeof(int64_t));
    if(*node_id_ptr < 0) break;
    if(*node_id_ptr == _node_id){
      *node_id_ptr = -1;
      return 0;
    }

    node_range_ptr += (sizeof(int32_t) + sizeof(int64_t) + sizeof(int64_t));
  }
  return -1;
}

bool UpdateBuffer::GetNodeRange(int32_t _node_id, int64_t *_key_st, 
			       int64_t *_key_end){
  uint8_t *node_range_ptr = reinterpret_cast<uint8_t*>(this) 
    + node_range_st_offset_;

  int32_t node_range_idx;

  bool found = false;
  
  for(node_range_idx = 0; node_range_idx < node_range_capacity_; 
      ++node_range_idx){
    VLOG(0) << "GetNodeRange, node_range_idx = " << node_range_idx;
    int32_t *node_id_ptr = reinterpret_cast<int32_t*>(node_range_ptr);
    int64_t *node_range_st_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							    + sizeof(int32_t));
    int64_t *node_range_end_ptr = reinterpret_cast<int64_t*>(node_range_ptr 
							     + sizeof(int32_t)
							     + sizeof(int64_t));
    if(*node_id_ptr == _node_id){
      VLOG(0) << "Found node id = " << *node_id_ptr
	      << " looking for _node_id = " << _node_id;
      *_key_st = *node_range_st_ptr;
      *_key_end = *node_range_end_ptr;
      found = true;
      break;
    }

    node_range_ptr += (sizeof(int32_t) + sizeof(int64_t) + sizeof(int64_t));
  }
  return found;
}

int UpdateBuffer::StartNodeRangeIteration(){
  node_range_iter_offset_ = node_range_st_offset_;
  return 0;
}

int32_t UpdateBuffer::NextNodeRange(int64_t *_key_st, int64_t *_key_end){

  if(node_range_iter_offset_ >= update_st_offset_) return -1;

  uint8_t *node_range_iter_ptr = reinterpret_cast<uint8_t*>(this) 
    + node_range_iter_offset_;
  
  int32_t *peer_id_ptr = reinterpret_cast<int32_t*>(node_range_iter_ptr);
  
  while((*peer_id_ptr < 0) && (node_range_iter_offset_ >= update_st_offset_)){
    node_range_iter_offset_ += (sizeof(int32_t) + sizeof(int64_t)*2);
    
    node_range_iter_ptr = reinterpret_cast<uint8_t*>(this) 
      + node_range_iter_offset_;
    
    peer_id_ptr = reinterpret_cast<int32_t*>(node_range_iter_ptr);
  }
  
  if(*peer_id_ptr < 0) return -1;
  
  uint8_t *key_st_ptr = node_range_iter_ptr + sizeof(int32_t);
  uint8_t *key_end_ptr = key_st_ptr + sizeof(int64_t);

  *_key_st = *(reinterpret_cast<int64_t*>(key_st_ptr));
  *_key_end = *(reinterpret_cast<int64_t*>(key_end_ptr));

  node_range_iter_offset_ += (sizeof(int32_t) + sizeof(int64_t)*2);

  return *peer_id_ptr;
}


int32_t UpdateBuffer::get_update_size(){
  return  update_size_;
}

int32_t UpdateBuffer::get_buff_size(){
  return buff_size_;
}
}
