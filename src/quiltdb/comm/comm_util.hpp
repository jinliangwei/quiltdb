#ifndef __QUILTDB_COMM_UTIL_HPP__
#define __QUILTDB_COMM_UTIL_HPP__

#include <zmq.hpp>
#include <glog/logging.h>
#include <boost/thread/tss.hpp>
#include <boost/scoped_ptr.hpp>

namespace quiltdb {

int InitThrSockIfHaveNot(boost::thread_specific_ptr<zmq::socket_t> *_sock,
				zmq::context_t *_zmq_ctx, int _type,
				const char *_connect_endp);

int InitScopedSockIfHaveNot(boost::scoped_ptr<zmq::socket_t> *_sock,
			    zmq::context_t *_zmq_ctx, int _type,
			    const char *_connect_endp);

}


#endif
