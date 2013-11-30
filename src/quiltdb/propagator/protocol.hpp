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

enum PropagatorMsgType{EPUpdateLog, EPUpdateBuffer, EPTerminate};

struct EPUpdateLogMsg {
  PropagatorMsgType msgtype_;
  int32_t table_id_;
  int64_t key_;
  UpdateType uptype_;
};

struct EPUpdateBufferMsg {
  PropagatorMsgType msgtype_;
  UpdateBuffer *update_buffer_ptr_;
};

enum PropRecvMsgType{EPRUpdateBuffer, EPRTerminate, EPRTerminateAck};

enum ReceiverMsgType{ERTerminate};

enum TimerMsgType{ETimerTrigger, ETimerCmd};

struct ETimerCmdMsg{
  TimerMsgType msgtype_;
  int32_t nanosec_;
};

}

#endif
