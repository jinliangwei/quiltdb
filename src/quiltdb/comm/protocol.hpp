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
		       EPRecvInternalTerminateACK, EPRInit, EMyUpdatesACK};

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
enum PropRecvMsgType{PropInit, PropInitACK, PropInitACKACK, PropStart,
		     EPRUpdateBuffer, EPRTerminate, EPRTerminateACK};

struct PropInitMsg{
  PropRecvMsgType msgtype_;
  int32_t node_id_;
};

struct PropInitAckAckMsg {
  PropRecvMsgType msgtype_;
  int32_t node_id_;
};

struct EPRUpdateBufferMsg {
  PropRecvMsgType msgtype_;
  int32_t table_id_;
};

// messages to be received by receiver via inter-thread sock
// ERInternalTerminate: application signals terminate to receiver
// EPRInternalTerminate: paired propagator signals terminate
// EMyUpdates: my owned updates
enum ReceiverMsgType{ERInternalTerminate, EPRInternalTerminate, EMyUpdates};

struct MyUpdatesMsg{
  ReceiverMsgType msgtype_;
  int32_t table_id_;
  UpdateBuffer *update_buffer_ptr_;
};

enum TimerMsgType{ETimerTrigger, ETimerCmd};

struct TimerCmdMsg{
  TimerMsgType msgtype_;
  int32_t nanosec_;
};

}

#endif
