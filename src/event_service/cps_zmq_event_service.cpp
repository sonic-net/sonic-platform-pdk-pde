/*
 * cps_zmq_event_service.cpp
 *
 *  Created on: Mar 26, 2016
 *      Author: cwichmann
 */



#include "cps_zmq_event_service.h"

#include "cps_string_utils.h"
#include "cps_comm_utils.h"
#include "std_time_tools.h"
#include "event_log.h"

#include <functional>
#include <unordered_map>
#include <memory>
#include <vector>
#include <iostream>

#include <unistd.h>

#include <zmq.hpp>
#include "../../sonic/private/event_service/cps_zmq_comm_utils.h"



namespace {


enum class socket_type_t: int { SOCKETS_queries=0, SOCKETS_notifier=1, SOCKETS_event_fwd=2};

struct zmq_address {
	std::vector<std::string> __addr;

	zmq_address(const std::string &a) { setup(a); }
	void setup(const std::string &str) {
		__addr = cps_string::split(str,":");
	}

	bool valid_address();

	std::string get(socket_type_t type=socket_type_t::SOCKETS_queries);
};

std::string zmq_address::get(socket_type_t type) {
	bool tcp = __addr[0]=="tcp";
	int port_start = tcp  ? atoi(__addr[2].c_str()) : 0;

	const char *sep = tcp==true ? ":" : "_";

	return cps_string::sprintf("%s:%s%s%d",__addr[0].c_str(),__addr[1].c_str(),
			sep,
			port_start+(int)type);
}

bool zmq_address::valid_address() {
	static const size_t TCP_PIECES=3; //   tcp://xxxx:port
	static const size_t INPROC_PIECES=2; // inproc://xxxxx
	static const std::unordered_map<std::string,size_t> addrs = {
			{ "tcp",TCP_PIECES}, { "inproc",INPROC_PIECES}
	};

	if (__addr.size()<1) {
		return false;
	}

	auto it = addrs.find(__addr[0]);

	if (it==addrs.end()) return false;

	return it->second == __addr.size();
}

}

namespace cps_zmq {

constexpr const size_t event_service::_socket_ix[3] { 0,1,2 };
constexpr const int event_service::_socket_types[3] { ZMQ_ROUTER,ZMQ_PUB,ZMQ_PULL };

event_service::event_service(zmq::context_t &context) {
	_cache=std::unique_ptr<cps_zmq::zmq_endpoint_cache> (new zmq_endpoint_cache(context));
	_sockets.clear();
	_sockets.resize(MAX_SOCKETS);

	_ops = std::unordered_map<int,std::function<void(std::vector<zmq::message_t>&)>, std::hash<int> > {
		{(int) socket_type_t::SOCKETS_queries, std::bind(&cps_zmq::event_service::event_operation,this, std::placeholders::_1) },
		{(int) socket_type_t::SOCKETS_notifier, std::bind(&cps_zmq::event_service::notifier_operation,this, std::placeholders::_1) },
		{(int) socket_type_t::SOCKETS_event_fwd, std::bind(&cps_zmq::event_service::puller_operation,this, std::placeholders::_1) },
	};

	for ( auto ix : _socket_ix ) {
		_sockets[ix].init(_cache.get());
	}
}

bool event_service::stop() {
	_stop =true;
	_thread->join();
	return true;
}

bool event_service::start(const std::string &addr) {
	zmq_address _addr(addr);

	if (!_addr.valid_address()) {
		EV_LOG(ERR,DSAPI,0,"EV-SERV-ST","Failed to start event service.. invalid address %s",addr.c_str());
		return false;
	}

	_address = addr;

	try {
		_thread = std::unique_ptr<std::thread>(new std::thread([this](){run_helper();}));
	} catch (...) {
		EV_LOG(ERR,DSAPI,0,"EV-SERV-ST","Failed to launch the service (%s)",addr.c_str());
		return false;
	}
	return true;
}

ssize_t event_service::socket_ix_from_void(void *sock)  {
	for ( auto ix : _socket_ix ) {
		if (_sockets[ix].get()->operator void *() == sock) {
			return ix;
		}
	}
	return -1;
}

void event_service::incomming_event(void *sock, zmq_pollitem_t *item, bool bad) {

	ssize_t ix = socket_ix_from_void(sock);

	if (ix==-1) return ;	//invalid socket

	do {
		if (bad) break;

		std::vector<zmq::message_t> msgs;

		if (!cps_zmq::recv_helper(_sockets[ix].get(),msgs)) {
			EV_LOG(ERR,DSAPI,0,"EV-SERV-INC","Yikes... socket state machine issue... need a reset (%d)",ix);
			bad = true;
			break;
		}

		if (event_log_is_enabled(ev_log_t_DSAPI,EV_T_LOG_DEBUG,0)) {
			EV_LOG(TRACE,DSAPI,0,"EV-SERV-INC","INC... message from (%d)",ix);
			for (auto &it : msgs ) {
				std::string s = cps_zmq::dump(it).c_str();
				EV_LOG(TRACE,DSAPI,0,"EV-SERV-INC","INC... message data (%s)",
						s.c_str());
			}
		}
		_ops.at(ix)(msgs);
	} while (false);

	if (bad) { _reactor.del_socket(sock); return ; }
}

void event_service::socket_setup() {
	zmq_address _addr(_address);

	static const std::vector<std::string> sock_types={"router","pub","pull"};

	if (_identifier_prefix.size()==0) {
		_identifier_prefix = cps_comms::cps_generate_unique_process_key();
	}

	std::string key = _identifier_prefix;

	for (auto ix : _socket_ix) {
		if (!_sockets[ix].alloc(_socket_types[ix])) {
			EV_LOG(ERR,DSAPI,0,"EV-SERV-SETUP","Not much we can do.. socket alloc fails..");
			std::terminate();
		}
	}

	for ( auto ix : _socket_ix ) {
		std::string addr = key + sock_types[ix];
		_sockets[ix].get()->setsockopt(ZMQ_IDENTITY,addr.c_str(),addr.size()+1);
	}

	for ( auto ix : _socket_ix ) {
		std::string addr = _addr.get(socket_type_t(ix));

		try {
			_sockets[ix].get()->bind(addr.c_str());
		} catch (zmq::error_t &err) {
			EV_LOG(ERR,DSAPI,0,"EV-SERV-SETUP","Port binding failed... (%d) (%s)",err.num(),addr.c_str());
			std::terminate();
		}
	}
	for (auto ix : _socket_ix ) {
		_reactor.add_socket(_sockets[ix].get()->operator void *(),true,false);
	}
}

void event_service::run_helper() {
	while (_stop==false) {
		socket_setup();
		while (_stop==false) {

			int rc = _reactor.wait( 	[&](void *sock, zmq_pollitem_t *item, bool bad) {
								this->incomming_event(sock,item,bad);
							},1000);
			if (rc==-1) break; //re-init all sockets... at the same addresses of course..
		}
		//reset all connections...
		for ( auto ix : _socket_ix ) {
			_sockets[ix].bad_fsm();
		}
	}
}

void event_service::event_operation(std::vector<zmq::message_t> &msgs) {
	//future use
}

void event_service::notifier_operation(std::vector<zmq::message_t> &msgs) {
	for ( auto &it : msgs ) {
		cps_zmq::dump(it);
	}
}

void event_service::puller_operation(std::vector<zmq::message_t> &msgs) {
	size_t ix = 0;
	size_t mx = msgs.size();
	size_t retry_count = 0;
	const static size_t MAX_RETRY=10;

	zmq::socket_t *sock = _sockets[(int)socket_type_t::SOCKETS_notifier].get();

	bool bad = false;
	for ( ; ix < mx ; ++ix ) {
		bad = false;
		try {
			bool rc = sock->send(msgs[ix],(ix+1)==mx ? 0 : ZMQ_SNDMORE);
			bad = !rc;
		} catch (...) {
			bad = true;
		}

		if (bad) {
			_sockets[(int)socket_type_t::SOCKETS_notifier].bad_fsm();
			_sockets[(int)socket_type_t::SOCKETS_notifier].alloc((int)socket_type_t::SOCKETS_notifier);
			sock = _sockets[(int)socket_type_t::SOCKETS_notifier].get();

			ix = 0;	//reset and resend entire message
			if (++retry_count > MAX_RETRY) { return; }
		}

	}
}

cps_zmq::event_client::event_client(zmq::context_t &context) : socket_base(context) {
	_socket->  alloc(ZMQ_SUB);
}

bool cps_zmq::event_client::connect(const std::string &addrs) {
	zmq_address _addr(addrs);
	return socket_base::connect(_addr.get(socket_type_t::SOCKETS_notifier)) ;
}

void cps_zmq::event_client::reset() {
	ssize_t RESET_ATTEMPTS=10;

	while (--RESET_ATTEMPTS > 0) {
		try {
			_socket.get()->alloc(ZMQ_SUB);
			for ( auto it : _keys ) {
				_socket->get()->setsockopt(ZMQ_SUBSCRIBE,&it[0],it.size());
			}
		} catch (zmq::error_t &err) {
			EV_LOG(ERR,DSAPI,0,"EV-CLI-RESET","Attempting to reset connection failed... (%d)",err.num());
			break;
		}
	}
}

bool cps_zmq::event_client::event_loop(const std::function<void(const void *key,
		size_t klen, const void *data, size_t len)> &event_loop_cb,	ssize_t timeout_in_ms) {

	while (true) {
		zmq_pollitem_t pi;
		pi.socket = _socket->get()->operator void *();
		pi.events = ZMQ_POLLIN;
		pi.fd = 0;
		pi.revents =0;
		int rc = 0;
		try {
			rc = zmq::poll(&pi,1,timeout_in_ms);
		} catch (zmq::error_t &err) {
			static const int _SLEEP_MS=4*1000;
			std_usleep(_SLEEP_MS);
			if (err.num()==ETERM) return false; //shutting down
			EV_LOG(ERR,DSAPI,0,"EV-CLI-EVENT_LOOP","Poll failed - socket will be regenerated... (%d)",err.num());
			//reset();
			continue;
		}
		if (rc==0) return true;	//timed out but there is nothing wrong with the operation.. (the user asked for timeout)
		 std::vector<zmq::message_t> msg;

		if (!cps_zmq::recv_helper(_socket->get(), msg)) {
			EV_LOG(ERR,DSAPI,0,"EV-CLI-EVENT_LOOP","Failed to receive message..."); //!@TODO Need to have a better log
			break;
		}

		void *key=nullptr, *data=nullptr;
		size_t klen=0,len=0;

		const static int KEY_DATA=2;
		static const int KEY_SEQ_DATA=3;

		if (msg.size()==KEY_DATA) {
			key = msg[0].data();
			klen = msg[0].size();
			data = msg[1].data();
			len = msg[1].size();
		} else if (msg.size()==KEY_SEQ_DATA) {
			key = msg[0].data();
			klen = msg[0].size();
			data = msg[2].data();
			len = msg[2].size();
		} else {
			break;
		}
		//if not one or two.. then something not so good.
		//call callback
		event_loop_cb(key,klen,data,len);
		return true;
	}
	return false;
}

bool cps_zmq::event_client::sub(const void *key,size_t klen) {
	try {
		std::vector<char> v;
		v.resize(klen);
		memcpy(&v[0],key,klen);
		_socket->get()->setsockopt(ZMQ_SUBSCRIBE,key,klen);
		_keys.push_back(v);
	} catch(zmq::error_t &err) {
		EV_LOG(ERR,DSAPI,0,"EV-CLI-SUB","Subscribe failed... (%d)",err.num());
		return false;
	}
	return true;
}

bool cps_zmq::event_publisher::connect(const std::string &addrs) {
	zmq_address _addr(addrs);
	return socket_base::connect(_addr.get(socket_type_t::SOCKETS_event_fwd)) ;
}

cps_zmq::event_publisher::event_publisher(zmq::context_t &context) : socket_base(context) {
	//always free sockets
	_socket->alloc(ZMQ_PUSH);
}

bool cps_zmq::event_publisher::pub(const void *key, size_t klen, const void *data, size_t len) {
	std::vector<zmq_send_param> param;
	param.resize(2);
	param[0].set_data(key,klen);
	param[1].set_data(data,len);
	try {
		if (send_helper(_socket->get(),param)) {
			return true;
		}
	} catch(zmq::error_t &err) {
		EV_LOG(ERR,DSAPI,0,"EV-PUB-PUB","Publish failed... (%d)",err.num());
	}
	return false;
}

}

