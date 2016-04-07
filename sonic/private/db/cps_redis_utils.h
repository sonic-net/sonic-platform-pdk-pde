/*
 * cps_redis_utils.h
 *
 *  Created on: Apr 4, 2016
 *      Author: cwichmann
 */

#ifndef SONIC_OBJECT_LIBRARY_SONIC_PRIVATE_CPS_REDIS_UTILS_H_
#define SONIC_OBJECT_LIBRARY_SONIC_PRIVATE_CPS_REDIS_UTILS_H_


#include "cps_api_errors.h"
#include "cps_api_object.h"
#include "cps_api_operation.h"

#include <functional>
#include <vector>
#include <string>
#include <mutex>
#include <memory>

struct redisContext ;
struct redisReply;

namespace cps_db {

//Note.. this is not a multthread safe class - not expected to at this point.
class connection {
	redisContext *_ctx=nullptr;
	size_t _pending = 0;

	static const std::string REDIS_SERVER_ADDR;
public:
	~connection();
	bool connect(const std::string &em=REDIS_SERVER_ADDR);

	struct operation_request_t {
		const char *_str=nullptr;
		ssize_t _str_len = -1;

		cps_api_object_t _object=nullptr;
		enum class obj_fields_t: int { obj_field_NULL,obj_field_OBJ,obj_field_INST,obj_field_CLASS };
		obj_fields_t _object_key=obj_fields_t::obj_field_NULL;
	};

	cps_api_return_code_t operation(operation_request_t * lst,size_t len);

	bool response(std::vector<void*> &data);

	bool sync_operation(operation_request_t * lst,size_t len, std::vector<void*> &data);
};

class response {
	redisReply * _reply;
public:
	response(redisReply *p) { _reply = p ; }

	int len() ;
	const char *str() ;


	bool is_ok_return() ;
	bool is_error_string();

	bool OK() { return is_ok_return(); }
	bool IS_ERROR_STR() { return is_error_string(); }
	bool IS_STR();
	bool IS_INTEGER();

	const char *get_error_string();
	int get_integer();

	bool use_iterator();
	void iterator(const std::function<void(size_t ix, int type, const void *data, size_t len)> &fun);

	void dump();
	~response();
};

class connection_cache {
	std::mutex _mutex;
	std::vector<std::unique_ptr<connection>> _pool;
public:
	connection * get();
	void put(connection* conn);

	/**
	 * This function will force the deletion of the db connection
	 * @param conn the connection to clean
	 */
	void remove(connection *conn);

	class guard {
		connection *_conn;
		connection_cache &_cache;
	public:
		connection *get() { return _conn; }
		void reset() { _conn = nullptr; }
		guard(connection_cache &cache, connection *conn) : _cache(cache), _conn(conn) {}
		~guard() { if (_conn!=nullptr)_cache.put(_conn); }
	};
};

class response_vector_cleaner {
	std::vector<void*> &_data;
public:
	response_vector_cleaner(std::vector<void*> &d) : _data(d){}
	~response_vector_cleaner();
};


}


#endif /* SONIC_OBJECT_LIBRARY_SONIC_PRIVATE_CPS_REDIS_UTILS_H_ */
