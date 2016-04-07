/*
 * cps_zmq_utils.h
 *
 *  Created on: Mar 26, 2016
 *      Author: cwichmann
 */

#ifndef INC_CPS_ZMQ_UTILS_H_
#define INC_CPS_ZMQ_UTILS_H_

#include <zmq.hpp>

#include <unordered_map>
#include <mutex>
#include <memory>

namespace cps_zmq {

/**
 * ZMQ Constants...
 */

static const size_t EVENT_PORT=10040;
static const size_t NS_PORT=10030;

zmq::context_t &GlobalContext();

class zmq_endpoint_cache  {
	std::mutex _mutex;
	std::unordered_map<int,std::unordered_map<zmq::socket_t*,zmq::socket_t*>> _sockets;
	zmq::context_t &_context;
public:
	zmq_endpoint_cache(zmq::context_t &ctx) : _context(ctx) {}

	zmq::context_t & get_context() { return _context; }

	zmq::socket_t* alloc(int type) ;
	void free(zmq::socket_t* sock, bool force=false);
protected:
	zmq::socket_t* __alloc(int type) ;
};

class socket_alloc {
	bool _force=false;
	zmq_endpoint_cache *_allocator=nullptr;
	zmq::socket_t *_sock=nullptr;
public:
	static const int SOCK_ALLOC_RETRY=100;
	/**
	 * Release and re-allocate the socket (release if a existing socket exists
	 * @return true if successfully allocated a socket
	 */

	bool alloc(int type, size_t retry_count=SOCK_ALLOC_RETRY) ;

	void init(zmq_endpoint_cache *epc) { free(); _allocator=epc; }

	socket_alloc() : _allocator(nullptr) {}

	socket_alloc(zmq_endpoint_cache *epc, zmq::socket_t *s) :
		_allocator(epc),_sock(s) {}

	void release(){ _sock = nullptr; }
	zmq::socket_t *get() { return _sock; }

	void good_fsm() { _force=false ; }
	void bad_fsm() { _force = true; }

	void free() ;
	~socket_alloc() { free(); }
};

class socket_base {
	cps_zmq::zmq_endpoint_cache *_cache_ptr=nullptr;
protected:
	std::unique_ptr<cps_zmq::zmq_endpoint_cache> _cache;
	std::unique_ptr<cps_zmq::socket_alloc> _socket;

	void init_alt_cache(cps_zmq::zmq_endpoint_cache * alt) { _cache_ptr = alt; }

	void free();

	socket_base(zmq::context_t &ctx=GlobalContext());
	virtual ~socket_base() { free() ; }
public:
	virtual bool connect(const std::string &addr);
};

class zmq_reacter {
	std::vector<void *> _sockets;
	std::vector<zmq_pollitem_t> _items;
public:
	zmq_reacter(){}

	void add_socket(void *sock, bool pollin=false, bool pollout=false);
	void del_socket(void *sock);
	void clear_sockets() { _sockets.clear(); _items.clear(); }
	int wait(const std::function<void(void *sock, zmq_pollitem_t *item, bool bad)> &fun, size_t timeout=-1);
};


}

#endif /* INC_CPS_ZMQ_UTILS_H_ */
