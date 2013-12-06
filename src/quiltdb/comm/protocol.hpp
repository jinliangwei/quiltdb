#ifndef __QUILTDB_PROTOCOL_HPP__
#define __QUILTDB_PROTOCOL_HPP__

#include <quiltdb/include/common.hpp>
#include <quiltdb/utils/memstruct.hpp>

namespace quiltdb {

/**
 * General Message format
 *
 * 1. EMsgType
 * 2. Type specific information
 *
 * This format relies on 0mq's multi-part message.
 */

// messages to be received by propagator via inter-thread sock
enum PropagatorMsgType{EPUpdateLog, EPUpdateBuffer, EPInternalTerminate, 
		       EPRecvInternalTerminateAck, EPRInit};

struct PUpdateLogMsg {
  PropagatorMsgType msgtype_;
  int32_t table_id_;
  int64_t key_;
  UpdateType update_type_;
};

struct PUpdateBufferMsg {
  PropagatorMsgType msgtype_;
  UpdateBuffer *update_buffer_ptr_;
};

// messages to be received by propagator or receiver via TCP sock
enum PropRecvMsgType{EPRUpdateBuffer, EPRTerminate, EPRTerminateAck};

// messages to be received by receiver via inter-thread sock
enum ReceiverMsgType{ERInternalTerminate, EPRInternalTerminate, EPRInitAck};

enum TimerMsgType{ETimerTrigger, ETimerCmd};

struct TimerCmdMsg{
  TimerMsgType msgtype_;
  int32_t nanosec_;
};

}

#endif
