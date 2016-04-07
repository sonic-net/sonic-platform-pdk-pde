/*
 * cps_zmq_event_service.h
 *
 *  Created on: Mar 26, 2016
 *      Author: cwichmann
 */

#ifndef INC_CPS_ZMQ_EVENT_SERVICE_H_
#define INC_CPS_ZMQ_EVENT_SERVICE_H_

#include <zmq.hpp>

#include "cps_service_interface.h"
#include "cps_string_utils.h"

#include <memory>
#include <thread>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include "cps_zmq_utils.h"

namespace cps_zmq {

class event_service: public cps_comms::comm_base_api_event_service {

	std::unique_ptr<cps_zmq::zmq_endpoint_cache> _cache;
	std::vector<cps_zmq::socket_alloc> _sockets;

	std::unique_ptr<std::thread> _thread;
	std::recursive_mutex _lock;

	cps_zmq::zmq_reacter _reactor;

	std::unordered_map<int,std::function<void(std::vector<zmq::message_t>&)>, std::hash<int> > _ops;
protected:

	std::string _identifier_prefix;
	std::string _address;

	bool _stop=false;

	const static int MAX_SOCKETS=3;
	const static size_t _socket_ix[3];
	const static int _socket_types[3];

	//methods
	virtual void socket_setup();
	virtual void run_helper();

	ssize_t socket_ix_from_void(void *sock) ;
	void incomming_event(void *sock, zmq_pollitem_t *item, bool bad);

	//notification callbacks
	virtual void event_operation(std::vector<zmq::message_t> &msgs);
	virtual void notifier_operation(std::vector<zmq::message_t> &msgs);
	virtual void puller_operation(std::vector<zmq::message_t> &msgs);

public:
	event_service(zmq::context_t &context);

	static inline std::string tcp_address(const std::string &ip, size_t port) {
		return cps_string::sprintf("tcp://%s:%d",ip.c_str(),port);
	}

	bool valid_address(const std::string &addr);

	bool start(const std::string &addr);
	virtual bool stop();
};

class event_publisher : public cps_comms::comm_base_api_pub, public socket_base {
public:
	event_publisher(zmq::context_t &context);
	virtual bool connect(const std::string &addrs);
	virtual bool pub(const void *key, size_t klen, const void *data, size_t len);
	virtual ~event_publisher() {}
};

class event_client : public cps_comms::comm_base_api_sub, public socket_base {
protected:
	std::vector<std::vector<char>> _keys;
	void reset();
public:
	event_client(zmq::context_t &context) ;

	virtual bool connect(const std::string &addrs);
	virtual bool event_loop(const std::function<void(const void *key, size_t klen, const void *data, size_t len)> &event_loop_cb,
			ssize_t timeout_in_ms=-1);
	virtual bool sub(const void *key, size_t klen);

	virtual ~event_client() {}
};


}


#endif /* INC_CPS_ZMQ_EVENT_SERVICE_H_ */
