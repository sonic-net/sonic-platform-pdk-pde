/*
 * cps_zmq_utils.cpp
 *
 *  Created on: Apr 4, 2016
 *      Author: cwichmann
 */



#include "cps_redis_utils.h"

#include "cps_api_object_key.h"

#include "cps_string_utils.h"
#include "cps_api_object.h"
#include "event_log.h"

#include "std_time_tools.h"

#include <stdlib.h>
#include <hiredis/hiredis.h>

#include <mutex>
#include <iostream>


const std::string cps_db::connection::REDIS_SERVER_ADDR{"127.0.0.1:6379"};

static std::mutex _singleton_lock;



bool cps_db::connection::connect(const std::string &s_) {
	auto lst = cps_string::split(s_,":");
	if (lst.size()!=2) {
		EV_LOG(ERR,DSAPI,0,"RED-CON","Failed to connect to server... bad address (%s)",s_.c_str());
		return false;
	}
	size_t port = std::atoi(lst[1].c_str());

	_ctx = redisConnect(lst[0].c_str(), port);

	return _ctx !=nullptr;
}

cps_api_return_code_t cps_db::connection::operation(operation_request_t * lst_,size_t len_) {

	static const size_t MAX_EXP_CMD{10};

	const char *cmds[MAX_EXP_CMD];
	size_t cmds_lens[MAX_EXP_CMD];

	operation_request_t * cur = lst_;

	const char **cmds_ptr = cmds;
	size_t *cmds_lens_ptr = cmds_lens;

	size_t iter = 0;

	std::vector<std::vector<char>> key;

	for ( ; iter < len_; ++iter , ++cur, ++cmds_ptr, ++cmds_lens_ptr ) {
		if (cur->_str!=nullptr) {
			*cmds_ptr = cur->_str;
			if (cur->_str_len==-1) *cmds_lens_ptr = strlen(cur->_str);
			else *cmds_lens_ptr = cur->_str_len;
		} else {
			assert(cur->_object!=nullptr);

			size_t needed = cur->_object_key == cps_db::connection::operation_request_t::obj_fields_t::obj_field_OBJ ?
					3 : 1;

			if ((cmds_ptr+needed) >= (cmds + MAX_EXP_CMD)) {
				EV_LOG(ERR,DSAPI,0,"CPS-RED-CON-OP","not enough space... args too "
						"long each object needs 2 slots of 10 possible (%d)",len_);
				return cps_api_ret_code_ERR;
			}

			size_t kpos = key.size();
			key.resize(kpos+1);

			std::vector<char> t;
			//get class or instance based on fields.. default to instance
			if (cur->_object_key==cps_db::connection::operation_request_t::obj_fields_t::obj_field_CLASS) {
				if (!cps_db::generate_key(key[kpos],cps_api_object_key(cur->_object))) return false;
			} else {
				if (!cps_db::generate_key(key[kpos],cur->_object)) return false;
			}

			*(cmds_ptr) = &key[kpos][0];
			*(cmds_lens_ptr) = key[kpos].size();

			if (cur->_object_key!=cps_db::connection::operation_request_t::obj_fields_t::obj_field_OBJ)
				continue;

			++cmds_ptr;
			++cmds_lens_ptr;

			*(cmds_ptr++) = "object";
			*(cmds_lens_ptr++) = strlen("object");

			size_t dl = cps_api_object_to_array_len(cur->_object);
			const char *d = (const char*)(const void *)cps_api_object_array(cur->_object);

			*(cmds_ptr) = d;
			*(cmds_lens_ptr) = dl;
		}
	}

	size_t MAX_RETRY=3;
	do {
		if (redisAppendCommandArgv(_ctx,cmds_ptr - cmds,cmds,cmds_lens)==REDIS_OK) {
			break;
		}
		EV_LOG(ERR,DSAPI,0,"CPS-RED-CON-OP","Seems to be an issue with the REDIS request - (first entry: %s)",cmds[0]);
		_pending = 0;
		redisFree(_ctx);
		connect();
	} while (MAX_RETRY-->0);

	++_pending;

	return cps_api_ret_code_OK;
}
cps_db::connection::~connection() {
	redisFree(_ctx);
}

bool cps_db::connection::response(std::vector<void*> &data_) {
	std::lock_guard<std::mutex> lg (_singleton_lock);
	if (_pending == 0) {
		return false;
	}

	while (_pending-- > 0) {
		void *reply;
		int rc = redisGetReply(_ctx,&reply);
		if (rc!=REDIS_OK) {
			std::cout << "Error : " << rc << std::endl;
			return false;
		}
		data_.push_back(reply);
	}
	_pending = 0;
	return true;
}

bool cps_db::connection::sync_operation(operation_request_t * lst,size_t len, std::vector<void*> &data) {
	if (operation(lst,len)!=cps_api_ret_code_OK) {
		return false;
	}
	return response(data);
}

cps_db::response::~response() {
	freeReplyObject(_reply);
}

int cps_db::response::get_integer() {
	return _reply->integer;
}

bool cps_db::response::IS_STR() {
	return _reply->type == REDIS_REPLY_STRING || _reply->type==REDIS_REPLY_ERROR;
}

const char *cps_db::response::get_error_string() {
	return _reply->str;
}
int cps_db::response::len()  {
	return _reply->len;
}
const char *cps_db::response::str()  {
	return _reply->str;
}

bool cps_db::response::use_iterator() {
	return _reply->type == REDIS_REPLY_ARRAY;
}

bool cps_db::response::IS_INTEGER() {
	return _reply->type == REDIS_REPLY_INTEGER;
}

bool cps_db::response::is_ok_return() {
	return _reply->type == REDIS_REPLY_INTEGER ?
			_reply->integer == 0 : false;
}

bool cps_db::response::is_error_string() {
	return _reply->type == REDIS_REPLY_ERROR ;
}

void cps_db::response::iterator(const std::function<void(size_t ix, int type, const void *data, size_t len)> &fun) {
	if (_reply->type != REDIS_REPLY_ARRAY) {
		return;
	}
	for (size_t ix = 0; ix < _reply->elements ; ++ix ) {
		if (_reply->element[ix]->type==REDIS_REPLY_INTEGER) {
			fun(ix,_reply->element[ix]->type,&_reply->element[ix]->integer,sizeof(_reply->element[ix]->integer));
		}
		if (_reply->element[ix]->type==REDIS_REPLY_STRING) {
			fun(ix,_reply->element[ix]->type,_reply->element[ix]->str,_reply->element[ix]->len);
		}
	}
}

void cps_db::response::dump() {
	std::cout << "Resp type is : " << _reply->type << std::endl;

	if(_reply->type == REDIS_REPLY_INTEGER) {
		std::cout << "Int : " << _reply->integer << std::endl;
	}
	if (is_ok_return()) std::cout << "OK Resp..." << std::endl;

	if (is_error_string()) {
		std::cout << get_error_string() << std::endl;
	}
	if (use_iterator()) {
		iterator([](size_t ix, int type, const void *data, size_t len) {
			std::cout << "IX:" << ix << " Type:" << type << " " << cps_string::tostring(data,len) << std::endl;
		});
	}
}

cps_db::connection * cps_db::connection_cache::get() {
	std::lock_guard<std::mutex> lg(_mutex);
	if (_pool.size()>0) {
		std::unique_ptr<connection> conn(_pool[_pool.size()-1].release());
		_pool.pop_back();
		return conn.release();
	}
	connection *con = new connection{};
	con->connect();
	return con;
}

void cps_db::connection_cache::put(cps_db::connection *c_) {
	std::lock_guard<std::mutex> lg(_mutex);
	std::unique_ptr<connection> conn(c_);
	_pool.push_back(std::move(conn));
}

void cps_db::connection_cache::remove(cps_db::connection *c) {
	delete c;
}

namespace {

bool append(std::vector<char> &lst, const void *data, size_t len) {
	size_t pos = lst.size();
	try {
		lst.resize(pos+len);
	} catch (...) { return false; }
	memcpy(&lst[pos],data,len);
	return true;
}

}

bool cps_db::append_data(std::vector<char> &lst,const void *data, size_t len) {
	return append(lst,data,len);
}

bool cps_db::generate_key(std::vector<char> &lst,const cps_api_key_t *key) {
	const cps_api_key_element_t *p = cps_api_key_elem_start_const(key);
	size_t len = cps_api_key_get_len_in_bytes((cps_api_key_t*)key);
	return append(lst,p,len);
}

bool cps_db::generate_key(std::vector<char> &lst,cps_api_object_t obj) {
	if (!generate_key(lst,cps_api_object_key(obj))) return false;

	cps_api_key_element_t *p = cps_api_key_elem_start(cps_api_object_key(obj));
	size_t len = cps_api_key_get_len(cps_api_object_key(obj));

	for ( size_t ix = 0; ix < len ; ++ix ) {
		cps_api_object_attr_t attr = cps_api_get_key_data(obj,p[ix]);
		if (attr==nullptr) continue;
		if (!append(lst,cps_api_object_attr_data_bin(attr),cps_api_object_attr_len(attr))) return false;
	}
	return true;
}

cps_db::connection &cps_db::GetStaticDBConn() {
	std::lock_guard<std::mutex> lg(_singleton_lock);
	static cps_db::connection *h = nullptr;
	if (h==nullptr) {
		h = new cps_db::connection{};
		h->connect();	//will have to deal with failures in get/set situations since connections may drop
	}
	return *h;
}

