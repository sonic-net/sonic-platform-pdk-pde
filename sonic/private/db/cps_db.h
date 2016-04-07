/*
 * cps_redis.h
 *
 *  Created on: Apr 5, 2016
 *      Author: cwichmann
 */

#ifndef SONIC_OBJECT_LIBRARY_SONIC_PRIVATE_DB_CPS_DB_H_
#define SONIC_OBJECT_LIBRARY_SONIC_PRIVATE_DB_CPS_DB_H_

#include "cps_redis_utils.h"
#include "cps_api_operation.h"

#include <algorithm>
#include <functional>
#include <stddef.h>


namespace cps_db {

bool get_sequence(cps_db::connection &conn, std::vector<char> &key, ssize_t &cntr);

bool fetch_all_keys(cps_db::connection &conn, const void *filt, size_t flen,
		const std::function<void(const void *key, size_t klen)> &fun);

bool multi_start(cps_db::connection &conn);
bool multi_end(cps_db::connection &conn, bool commit=true);

bool delete_object(cps_db::connection &conn,cps_api_object_t obj);
bool store_object(cps_db::connection &conn,cps_api_object_t obj);
bool store_objects(cps_db::connection &conn,cps_api_object_list_t objs);


bool get_object(cps_db::connection &conn, const std::vector<char> &key, cps_api_object_t obj);
bool get_object(cps_db::connection &conn, cps_api_object_t obj);

bool get_objects(cps_db::connection &conn,std::vector<char> &key,cps_api_object_list_t obj_list) ;
bool get_objects(cps_db::connection &conn, cps_api_object_t obj,cps_api_object_list_t obj_list);

void free_response(std::vector<void*> &data) ;


static inline cps_api_qualifier_t QUAL(cps_api_key_t *key) { return cps_api_key_get_qual(key); }

static inline cps_api_qualifier_t UPDATE_QUAL(cps_api_key_t *key, cps_api_qualifier_t qual) {
	cps_api_qualifier_t _cur = cps_api_key_get_qual(key);
	cps_api_key_set(key,0,qual);
	return _cur;
}

bool generate_key(std::vector<char> &lst,const cps_api_key_t *key);
bool generate_key(std::vector<char> &lst,cps_api_object_t obj);

bool append_data(std::vector<char> &lst,const void *data, size_t len);


cps_db::connection &GetStaticDBConn();

template <typename T>
struct vector_hash {
	std::size_t operator() (const std::vector<T> &c) const {
		std::size_t rc;
		for ( auto it : c ) {
			rc = (it) ^ (rc << 3);
		}
		return rc;
	}
};


}


#endif /* SONIC_OBJECT_LIBRARY_SONIC_PRIVATE_DB_CPS_DB_H_ */
