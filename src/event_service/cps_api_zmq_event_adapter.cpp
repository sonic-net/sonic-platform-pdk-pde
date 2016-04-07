/*
 * cps_api_zmq_event_adapter.cpp
 *
 *  Created on: Apr 3, 2016
 *      Author: cwichmann
 */


#include "cps_api_events.h"
#include "cps_api_event_init.h"
#include "cps_api_key.h"

#include "cps_zmq_event_service.h"
#include "cps_zmq_utils.h"

#include "std_time_tools.h"

#include <string>
#include <mutex>
#include <memory>


static cps_zmq::event_service * _es;

const static std::string __get_event_port() {
	return cps_string::sprintf("tcp://127.0.0.1:%d",cps_zmq::EVENT_PORT);
}


struct __zmq_handle {
	constexpr static size_t __MAGIC=0xfeedc11ff;
	size_t _magic = __MAGIC;

	std::mutex _sub_mutex;
	bool _sub_locked = false;	//a simple locking mechanism (saves having to do sync obj)

	bool _enabled = true;
	bool _waiting = false;

	std::mutex _pub_mutex;

	std::unique_ptr<cps_zmq::event_client> _sub;
	std::unique_ptr<cps_zmq::event_publisher> _pub;

};

static inline __zmq_handle * HD(void *c)  {
	assert (((__zmq_handle*)c)->_magic==__zmq_handle::__MAGIC);
	return (__zmq_handle*)c;
}


static cps_api_return_code_t _cps_api_event_service_client_connect(cps_api_event_service_handle_t * handle) {

	std::unique_ptr<__zmq_handle> h;
	h = std::unique_ptr<__zmq_handle>(new __zmq_handle{});

	h->_sub = std::unique_ptr<cps_zmq::event_client>(new cps_zmq::event_client(cps_zmq::GlobalContext()));
	h->_pub = std::unique_ptr<cps_zmq::event_publisher>(new cps_zmq::event_publisher(cps_zmq::GlobalContext()));

	if (!h->_sub->connect(__get_event_port())) {
		return cps_api_ret_code_ERR;
	}
	if (!h->_pub->connect(__get_event_port())) {
		return cps_api_ret_code_ERR;
	}
	*handle = h.get();
	h.release();
    return cps_api_ret_code_OK;
}

//ZMQ handles are not multithread safe.. protect access
static cps_api_return_code_t _cps_api_event_service_client_register(
        cps_api_event_service_handle_t handle,
        cps_api_event_reg_t * req) {

	__zmq_handle *p = HD(handle);

	std::lock_guard<std::mutex> lg(p->_sub_mutex);

	size_t ix = 0;
	for ( ; ix < req->number_of_objects ; ++ix ) {
		size_t _len = cps_api_key_get_len_in_bytes(req->objects+ix);
		void *_addr = cps_api_key_elem_start(req->objects+ix);
		p->_sub->sub(_addr,_len);
	}

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t _cps_api_event_service_publish_msg(cps_api_event_service_handle_t handle,
        cps_api_object_t msg) {

	__zmq_handle *p = HD(handle);

    cps_api_key_t *_okey = cps_api_object_key(msg);

	size_t _len = cps_api_key_get_len_in_bytes(_okey);
	void *_addr = cps_api_key_elem_start(_okey);

	std::lock_guard<std::mutex> lg(p->_pub_mutex);

	if (p->_pub->pub(_addr,_len,cps_api_object_array(msg),cps_api_object_to_array_len(msg))) {
		return cps_api_ret_code_OK;
	}

    return cps_api_ret_code_ERR;
}

static cps_api_return_code_t _cps_api_event_service_client_deregister(cps_api_event_service_handle_t handle) {
	__zmq_handle *p = HD(handle);

	do {
		std::lock_guard<std::mutex> lg(p->_sub_mutex);
		p->_enabled = false;
		if (p->_waiting) continue;

		delete p;
	} while (0);

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t _cps_api_wait_for_event(
        cps_api_event_service_handle_t handle,
        cps_api_object_t msg) {

	__zmq_handle *p = HD(handle);

	{
		std::lock_guard<std::mutex> lg(p->_sub_mutex);

		if (p->_enabled==false) {
			return cps_api_ret_code_ERR;
		}
		if (p->_waiting==true) {
			return cps_api_ret_code_ERR;
		}
		p->_waiting = true;
	}
	//own sub lock now...
	//p->_enabled is readonly so lock safe
	bool has_event = false;

	while (p->_enabled) {
		std::lock_guard<std::mutex> lg(p->_sub_mutex);

		bool rc = p->_sub->event_loop([&msg,&has_event](const void *k, size_t kl, const void *data, size_t len){
			if (cps_api_array_to_object(data,len,msg)) {
				has_event=true;
			}
		},500);

		if (!rc) break;

		if (!has_event) continue;
		break;
	}

	p->_waiting = false;

	if (has_event) return cps_api_ret_code_OK;
	if (!p->_enabled) return cps_api_ret_code_ERR;
	return cps_api_ret_code_TIMEOUT;
}

static cps_api_event_methods_reg_t functions = {
        NULL,
        0,
        _cps_api_event_service_client_connect,
        _cps_api_event_service_client_register,
        _cps_api_event_service_publish_msg,
        _cps_api_event_service_client_deregister,
        _cps_api_wait_for_event
};

extern "C" cps_api_return_code_t cps_api_event_service_init(void) {
    cps_api_event_method_register(&functions);
    return cps_api_ret_code_OK;
}

#include <thread>
#include <iostream>

void dump_client() {
	cps_zmq::event_client ec(cps_zmq::GlobalContext());

	if (!ec.connect(__get_event_port())) {
		return;
	}

	ec.sub(NULL,0);

	bool _found = false;
	while (true) {
		ec.event_loop([&_found](const void *k, unsigned long int kl, const void *d, unsigned long int dl){
			_found = true;
			std::cout << "Key : " << cps_string::tostring(k,kl) << std::endl;
			std::cout << "Data: " << cps_string::tostring(d,dl) << std::endl;
		},1000);
	}
}

void cps_client() {
	cps_api_event_service_handle_t h;
	if (_cps_api_event_service_client_connect(&h)!=cps_api_ret_code_OK) {
		return;
	}

	cps_api_key_t key;
	memset(&key,0,sizeof(key));
	cps_api_event_reg_t _reg;
	memset(&_reg,0,sizeof(_reg));

	_reg.number_of_objects = 1;
	_reg.objects=&key;

	if (_cps_api_event_service_client_register(h,&_reg)!=cps_api_ret_code_OK) {
		return;
	}
	char _print_buff[10000];
	while(true) {
		cps_api_object_t obj = cps_api_object_create();

		if (_cps_api_wait_for_event(h,obj)==cps_api_ret_code_OK) {
			cps_api_object_to_string(obj,_print_buff,sizeof(_print_buff));
			std::cout << _print_buff;
		}
	}
}


extern "C" cps_api_return_code_t cps_api_services_start() {
	_es = new cps_zmq::event_service(cps_zmq::GlobalContext());

	if (!_es->start(__get_event_port())) {
		return cps_api_ret_code_ERR;
	}

	new std::thread(cps_client);

    return cps_api_ret_code_OK;
}

