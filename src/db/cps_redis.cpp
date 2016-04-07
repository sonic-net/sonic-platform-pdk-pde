/*
 * cps_redis.cpp
 *
 *  Created on: Apr 5, 2016
 *      Author: cwichmann
 */


#include "cps_api_object.h"
#include "cps_api_key.h"
#include "cps_api_object_key.h"
#include "cps_db.h"

#include <vector>
#include <functional>
#include <mutex>

#include <hiredis/hiredis.h>


static std::mutex _mutex;

bool cps_db::get_sequence(cps_db::connection &conn, std::vector<char> &key, ssize_t &cntr) {
	cps_db::connection::operation_request_t e[2];

	e[0]._str = "INCR";
	e[1]._str = &key[0];
	e[1]._str_len = key.size();
	std::vector<void*> data;
	if (!conn.sync_operation(e,2,data)) {
		return false;
	}

	if (data.size()==1) {
		cps_db::response r((redisReply*)data[0]);
		if (r.IS_INTEGER()) {
			cntr = (r.get_integer());
			return true;
		}
	}
	free_response(data);
	return false;
}

bool cps_db::fetch_all_keys(cps_db::connection &conn, const void *filt, size_t flen,
		const std::function<void(const void *key, size_t klen)> &fun) {

	cps_db::connection::operation_request_t e[2];
	e[0]._str="KEYS";

	e[1]._str=(char*)filt;
	e[1]._str_len = flen;

	if (conn.operation(e,2)!=cps_api_ret_code_OK) {
		return false;
	}

	std::vector<void*> data;

	if (!conn.response(data)) {
		return false;
	}

	for ( auto it : data) {
		redisReply *p = (redisReply*) it;
		cps_db::response resp(p);
		resp.iterator([&fun](size_t ix, int type, const void *data, size_t len){ fun(data,len);});
	}

	return true;
}

void cps_db::free_response(std::vector<void*> &data)  {
	for (auto it : data)  {
		freeReplyObject((redisReply*)it);
	}
}

bool cps_db::multi_start(cps_db::connection &conn) {
	cps_db::connection::operation_request_t e;
	e._str = "MULTI";
	std::vector<void*> data;
	if (!conn.sync_operation(&e,1,data)) {
		return false;
	}
	bool rc = false;
	if (data.size()> 0) {
		cps_db::response r((redisReply*)data[0]);
		rc = (r.get_error_string()!=nullptr && strcmp("OK",r.get_error_string()));
		data.erase(data.begin());
	}
	free_response(data);
	return rc;
}

bool cps_db::multi_end(cps_db::connection &conn, bool commit) {
	cps_db::connection::operation_request_t e;
	e._str = commit  ? "EXEC" : "DISCARD";
	std::vector<void*> data;
	if (!conn.sync_operation(&e,1,data)) {
		return false;
	}
	bool rc = false;

	if (data.size()> 0) {
		rc = true;
		cps_db::response r((redisReply*)data[0]);

		r.iterator([&rc](size_t ix, int type, const void *data, size_t len){
			if (type!=REDIS_REPLY_INTEGER) rc = false;
		});

		data.erase(data.begin());
	}
	free_response(data);
	return rc;

}

bool cps_db::delete_object(cps_db::connection &conn,cps_api_object_t obj) {
	cps_db::connection::operation_request_t e[2];
	e[0]._str = "DEL";
	e[1]._object = obj;
	e[1]._object_key = cps_db::connection::operation_request_t::obj_fields_t::obj_field_INST;

	std::vector<void*> data;
	if (!conn.sync_operation(e,2,data)) {
		return false;
	}
	free_response(data);
	return true;
}


bool cps_db::store_objects(cps_db::connection &conn,cps_api_object_list_t objs) {
    size_t ix = 0;
    size_t mx = cps_api_object_list_size(objs);
    for ( ; ix < mx ; ++ix ) {
    	cps_db::connection::operation_request_t e[2];
    	e[0]._str = "HSET";
    	e[1]._object = cps_api_object_list_get(objs,ix);
    	e[1]._object_key = cps_db::connection::operation_request_t::obj_fields_t::obj_field_OBJ;

    	if (!conn.operation(e,2)==cps_api_ret_code_OK) return false;
    }
    std::vector<void*> data;
	bool rc = (conn.response(data));

	free_response(data);
	return rc;
}

bool cps_db::store_object(cps_db::connection &conn,cps_api_object_t obj) {
	cps_db::connection::operation_request_t e[2];
	e[0]._str = "HSET";
	e[1]._object = obj;
	e[1]._object_key = cps_db::connection::operation_request_t::obj_fields_t::obj_field_OBJ;
	std::vector<void*> data;
	if (!conn.sync_operation(e,2,data)) {
		return false;
	}
	free_response(data);
	return true;
}

bool cps_db::get_object(cps_db::connection &conn, const std::vector<char> &key, cps_api_object_t obj) {
	cps_db::connection::operation_request_t e[3];
	e[0]._str = "HGET";
	e[1]._str = &key[0];
	e[1]._str_len = key.size();
	e[2]._str = "object";
	std::vector<void*> data;
	if (!conn.sync_operation(e,3,data)) {
		return false;
	}
	if (data.size()==0) return false;

	cps_db::response r ((redisReply*)data[0]);

	data.erase(data.begin());

	bool rc = false;

	if (r.IS_STR() && !r.IS_ERROR_STR() && cps_api_array_to_object(r.str(),r.len(),obj)) {
		rc = true;
	}

	free_response(data);
	return rc;
}

bool cps_db::get_object(cps_db::connection &conn, cps_api_object_t obj) {
	std::vector<char> key;
	if (!cps_db::generate_key(key,obj)) return false;
	return get_object(conn,key,obj);
}

#include <iostream>
#include "cps_string_utils.h"

bool cps_db::get_objects(cps_db::connection &conn,std::vector<char> &key,cps_api_object_list_t obj_list) {
	cps_db::append_data(key,"*",1);

	std::vector<std::vector<char>> all_keys;
	bool rc = fetch_all_keys(conn, &key[0],key.size(),[&conn,&all_keys](const void *key, size_t len){
		std::vector<char> c;
		cps_db::append_data(c,key,len);
		all_keys.push_back(std::move(c));
	});

	size_t ix =0;
	size_t mx = all_keys.size();
	for ( ; ix < mx ; ++ix ) {
		cps_api_object_guard og (cps_api_object_create());

		if (get_object(conn,all_keys[ix],og.get())) {
			if (cps_api_object_list_append(obj_list,og.get())) {
				og.release();
			}
		}
	}
	return true;
}

bool cps_db::get_objects(cps_db::connection &conn, cps_api_object_t obj,cps_api_object_list_t obj_list) {
	std::vector<char> k;
	if (!cps_db::generate_key(k,cps_api_object_key(obj))) return false;
	return get_objects(conn,k,obj_list);
}

cps_db::response_vector_cleaner::~response_vector_cleaner() {
	for (auto it: _data) {
		freeReplyObject((redisReply*)it);
	}
}

