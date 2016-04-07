/*
 * cps_zmq_utils.cpp
 *
 *  Created on: Mar 26, 2016
 *      Author: cwichmann
 */


#include "cps_zmq_utils.h"

#include "event_log.h"

#include <iostream>
#include <mutex>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iostream>
#include <unistd.h>

namespace cps_zmq {


zmq::socket_t* zmq_endpoint_cache::alloc(int type) {
	std::lock_guard<std::mutex> lg(_mutex);
	auto __type = _sockets.find(type);
	if (__type != _sockets.end()) {
		auto &__cache_list = __type->second;
		auto __cache = __cache_list.begin();
		if (__cache!=__cache_list.end()) {
			zmq::socket_t* p = __cache->second;
			__cache_list.erase(__cache);
			return p;
		}
	}
	return __alloc(type);
}

zmq::socket_t* zmq_endpoint_cache::__alloc(int type) {
	return new zmq::socket_t(_context,type);
}

void zmq_endpoint_cache::free(zmq::socket_t* sock, bool force) {
	force=true;
	if (!force) {
		std::lock_guard<std::mutex> lg(_mutex);
		try {
			int __type = 0;
			size_t len = sizeof(__type);
			sock->getsockopt(ZMQ_TYPE,&__type,&len);
			_sockets[__type][sock] = sock;
			return;
		} catch (...) {
			std::cout << "socket threw exception... not able to clean" << std::endl;
			EV_LOG(ERR,DSAPI,0,"EPC-FREE","Failed to clean up object...");
			//ignore error and delete sock on any failure
		}
	}
	delete sock;
}

bool socket_alloc::alloc(int type, size_t retry_count) {
	free();
	if (_allocator==nullptr) return false;
	static const int MIN_COUNT=1;
	retry_count = std::max(MIN_COUNT,(int)retry_count);

	while (retry_count-- > 1) {
		try {
			_sock = _allocator->alloc(type);
			if (_sock!=nullptr) break;
		} catch (zmq::error_t &ex) {
			if (ex.num()!=ETERM) {
				EV_LOG(ERR,DSAPI,0,"SOCKET-ALLOC-ALLOC","Socket allocation failed %d (err:%d)",retry_count,ex.num());
				std::cout << "Error code " << ex.num() << " when trying to alloc" << std::endl;
				_sock = nullptr;
				static const int _SLEEP_MS=4*1000;
				usleep(_SLEEP_MS);
			} else {
				break;
			}
		}

	}
	return _sock != nullptr;
}

void socket_alloc::free() {
	if (_allocator == nullptr) return;	//not much we can do
	if (_sock!=nullptr) {
		_allocator->free(_sock,_force);
		_sock = nullptr;
	}
}

void socket_base::free() {
	if (_cache.get()!=nullptr) {
		if (_socket.get()!=nullptr) {
			_socket.get()->free();
		}
	}
}

socket_base::socket_base(zmq::context_t &ctx)  {
	_cache = std::unique_ptr<cps_zmq::zmq_endpoint_cache>(new cps_zmq::zmq_endpoint_cache(ctx));
	_socket = std::unique_ptr<cps_zmq::socket_alloc>(new cps_zmq::socket_alloc());
	_cache_ptr = _cache.get();
	_socket->init(_cache_ptr);
}

bool socket_base::connect(const std::string &addr) {
	try {
		_socket->get()->connect(addr.c_str());
	} catch (zmq::error_t &err) {
		return false;
	}
	return true;
}

void zmq_reacter::del_socket(void *sock) {
	auto sit = std::find_if(_sockets.begin(),_sockets.end(),[sock](void *it) ->bool { return sock == it; });
	if (sit!=_sockets.end()) _sockets.erase(sit);

	auto iit = std::find_if(_items.begin(),_items.end(),[sock](zmq_pollitem_t &it) ->bool { return it.socket==sock; });
	if (iit!=_items.end()) _items.erase(iit);
}

void zmq_reacter::add_socket(void *sock, bool pollin, bool pollout) {
	_sockets.push_back(sock);
	zmq_pollitem_t pi;
	memset(&pi,0,sizeof(pi));
	pi.socket = sock;
	pi.events = pollin ? pi.events | ZMQ_POLLIN : pi.events;
	pi.events = pollout ? pi.events | ZMQ_POLLOUT : pi.events;
	pi.events |= ZMQ_POLLERR ;
	_items.push_back(pi);
}

int zmq_reacter::wait(const std::function<void(void *sock, zmq_pollitem_t *item, bool bad)> &fun, size_t timeout) {
	int rc = zmq_poll(&_items[0],_items.size(),timeout);
	if (rc==0) return 0;
	if (rc==-1) {
		for ( auto it : _items) {
			if (zmq_poll(&it,1,timeout)==-1) {
				fun(it.socket,&it,true);
				return -1;
			}
		}
		return -1;
	}
	if (rc > 0) {
		for (auto it : _items ) {
			if (it.revents!=0) {
				fun(it.socket,&it,false);
			}
		}
	}
	return rc;
}


static std::recursive_mutex _mutex;

zmq::context_t &GlobalContext() {
	std::lock_guard<std::recursive_mutex> lg(_mutex);
	static zmq::context_t *context = new zmq::context_t{};
	return *context;
}


}
