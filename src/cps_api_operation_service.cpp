/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/*
 * cps_api_operation_service.cpp
 */


#include "cps_ns.h"
#include "cps_db.h"
#include "cps_api_client_utils.h"
#include "cps_api_events.h"

#include "cps_api_operation_stats.h"
#include "cps_api_key_cache.h"
#include "cps_api_object_key.h"
#include "cps_dictionary.h"

#include "cps_api_operation.h"
#include "cps_api_object.h"
#include "std_socket_service.h"
#include "std_rw_lock.h"
#include "std_mutex_lock.h"
#include "event_log.h"
#include "std_file_utils.h"
#include "std_time_tools.h"
#include "std_assert.h"

#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <thread>



cps_db::connection_cache _conn_cache;


using reg_functions_t = std::vector<cps_api_registration_functions_t>;

struct cps_api_operation_data_t {
    std_socket_server_handle_t handle;
    std_socket_server_t service_data;
    std_rw_lock_t db_lock;
    std_mutex_type_t mutex;
    reg_functions_t db_functions;
    cps_api_channel_t ns_handle;
    std::unordered_map<uint64_t,uint64_t> stats;
    std::thread *redis_poller;
    ssize_t _polling_iteration=6;
    cps_api_event_service_handle_t events;

    void insert_functions(cps_api_registration_functions_t*fun);

    uint64_t get_stat(cps_api_obj_stats_type_t stat);
    void set_stat(cps_api_obj_stats_type_t stat,uint64_t val);
    void inc_stat(cps_api_obj_stats_type_t stat,int64_t how_much=1);
    void make_ave(cps_api_obj_stats_type_t stat,cps_api_obj_stats_type_t count,uint64_t val);
};

uint64_t cps_api_operation_data_t::get_stat(cps_api_obj_stats_type_t stat) {
    std_mutex_simple_lock_guard lg(&mutex);
    return stats[stat];
}

void cps_api_operation_data_t::set_stat(cps_api_obj_stats_type_t stat,uint64_t val) {
    std_mutex_simple_lock_guard lg(&mutex);
    stats[stat] = val;
}

void cps_api_operation_data_t::inc_stat(cps_api_obj_stats_type_t stat,int64_t how_much) {
    std_mutex_simple_lock_guard lg(&mutex);
    set_stat(stat,get_stat(stat) + how_much);
}

void cps_api_operation_data_t::make_ave(cps_api_obj_stats_type_t stat,cps_api_obj_stats_type_t count,uint64_t val) {
    std_mutex_simple_lock_guard lg(&mutex);
    inc_stat(count);

    uint64_t cnt = get_stat(count);
    uint64_t tot = get_stat(stat) + val;

    set_stat(stat,(uint64_t)(tot/((double)cnt)));
}

class Function_Timer {
    cps_api_operation_data_t *_p;
    cps_api_obj_stats_type_t _min;
    cps_api_obj_stats_type_t _max;
    cps_api_obj_stats_type_t _ave;
    cps_api_obj_stats_type_t _count;
    bool started=false;
    uint64_t tm=0;
public:
    Function_Timer(cps_api_operation_data_t *p,
            cps_api_obj_stats_type_t min, cps_api_obj_stats_type_t max,
            cps_api_obj_stats_type_t ave, cps_api_obj_stats_type_t count) {
        _p = p;
        _min = min;
        _max = max;
        _ave = ave;
        _count = count;
    }
    void start() {
        tm = std_get_uptime(NULL);
        started = true;
    }
    ~Function_Timer() {
        if (started) {
            uint64_t diff = std_get_uptime(NULL) - tm;
            _p->make_ave(_ave,_count,diff);
            uint64_t min = _p->get_stat(_min);
            if (min==0 || diff < min) {
                _p->set_stat(_min,diff);
            }
            if (diff > _p->get_stat(_max)) {
                _p->set_stat(_max,diff);
            }
        } else {
            _p->inc_stat(_count);
        }
    }
};

static cps_api_return_code_t  cps_api_handle_get(cps_api_operation_data_t *context,cps_api_get_params_t *param, size_t ix) {
    Function_Timer tm(context,cps_api_obj_stat_GET_MIN_TIME,cps_api_obj_stat_GET_MAX_TIME,
            cps_api_obj_stat_GET_AVE_TIME,cps_api_obj_stat_GET_COUNT);

    std_rw_lock_read_guard g(&context->db_lock);

    cps_api_object_t filter = cps_api_object_list_get(param->filters,ix);
    cps_api_key_t *key = cps_api_object_key(filter);

    cps_api_return_code_t rc = cps_api_ret_code_ERR;
    size_t func_ix = 0;
    size_t func_mx = context->db_functions.size();

    for ( ; func_ix < func_mx ; ++func_ix ) {
        cps_api_registration_functions_t *p = &(context->db_functions[func_ix]);
        if ((p->_read_function!=NULL) &&
                (cps_api_key_matches(key,&p->key,false)==0)) {

            tm.start();
            rc = p->_read_function(p->context,param,ix);
            break;
        }
    }
    if (rc!=cps_api_ret_code_OK) {
    	context->inc_stat(cps_api_obj_stat_GET_FAILED);
    }
    return rc;
}

static cps_api_return_code_t _commit_processing(cps_api_operation_data_t *context,cps_api_transaction_params_t *param, size_t ix) {

    Function_Timer tm(context,cps_api_obj_stat_GET_MIN_TIME,cps_api_obj_stat_GET_MAX_TIME,
                cps_api_obj_stat_GET_AVE_TIME,cps_api_obj_stat_GET_COUNT);

    std_rw_lock_read_guard g(&context->db_lock);

    cps_api_return_code_t rc =cps_api_ret_code_OK;

    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_key_t *key = cps_api_object_key(obj);

    size_t func_ix = 0;
    size_t func_mx = context->db_functions.size();

    for ( ; func_ix < func_mx ; ++func_ix ) {
        cps_api_registration_functions_t *p = &(context->db_functions[func_ix]);
        if ((p->_write_function!=NULL) &&
                (cps_api_key_matches(key,&p->key,false)==0)) {
            tm.start();
            rc = p->_write_function(p->context,param,0);
            break;
        }
    }

    if (rc!=cps_api_ret_code_OK) {
        context->inc_stat(cps_api_obj_stat_SET_FAILED);
    }
    return rc;
}

static cps_api_return_code_t cps_api_handle_commit(cps_api_operation_data_t *context,cps_api_transaction_params_t *param, size_t ix) {
	cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
	if (obj==nullptr) return cps_api_ret_code_ERR;


	cps_db::connection_cache::guard g(_conn_cache,_conn_cache.get());
	if (g.get()==nullptr) return cps_api_ret_code_ERR;

	cps_api_object_guard og(cps_api_object_create());
	if (og.get()==nullptr) {
		return cps_api_ret_code_ERR; //already logged
	}

    if (cps_api_obj_has_cached_state(obj)) {
		ssize_t seq = 0;
		cps_api_object_attr_t attr = cps_api_get_key_data(obj,cps_api_qualifier_JOURNAL);
		if (attr!=nullptr) {
			cps_api_qualifier_t qual = cps_db::UPDATE_QUAL(cps_api_object_key(obj),cps_api_qualifier_JOURNAL);
			seq = *(uint64_t*)cps_api_object_attr_data_bin(attr);
			cps_db::delete_object(*g.get(),obj);

			cps_api_object_clone(og.get(),obj);

			if (qual==cps_api_qualifier_JOURNAL) qual = cps_api_qualifier_TARGET;
			(void)cps_db::UPDATE_QUAL(cps_api_object_key(obj),qual);
		}
    }

	cps_api_return_code_t rc = _commit_processing(context,param,ix);

	obj = cps_api_object_list_get(param->change_list,ix);

	if (cps_api_obj_has_cached_state(obj)) {

		cps_api_attr_id_t ids [] = { CPS_API_OBJ_FLAGS, cps_api_object_FLAG_RC };
		cps_api_object_e_add(og.get(),ids,2,cps_api_object_ATTR_T_U32,&rc,sizeof(rc));
		cps_api_event_publish(context->events,og.get());	//publish transaction update

		if (rc==cps_api_ret_code_OK) {
	        if (cps_api_obj_has_cached_state(obj)) {
	        	cps_api_operation_types_t op_type = cps_api_object_type_operation(cps_api_object_key(obj));
	        	if (op_type==cps_api_oper_DELETE) {
	        		cps_db::delete_object(*g.get(),obj);
	        	}
	        	if (op_type==cps_api_oper_SET || op_type==cps_api_oper_CREATE) {
	        		cps_db::store_object(*g.get(),obj);
	        	}
	        }
		}
	}
	return rc;
}

static void sync_to_db(cps_api_operation_data_t *context, cps_api_key_t *key) {

	cps_db::connection_cache::guard g(_conn_cache,_conn_cache.get());
	if (g.get()==nullptr) {
    	EV_LOG(ERR,DSAPI,0,"CPS-SYNC-RED","No db... can't proceed");
		return ;
	}

	cps_api_get_params_t param;
    if (cps_api_get_request_init(&param)!=cps_api_ret_code_OK) {
    	EV_LOG(ERR,DSAPI,0,"CPS-SYNC-RED","Failed to initialize handle.. can't proceed");
        return ;
    }
    cps_api_get_request_guard grg(&param);

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(param.filters);
    cps_api_key_copy(cps_api_object_key(obj),key);

    param.key_count = 1;
    param.keys = cps_api_object_key(obj);

    std::unordered_map<std::vector<char>, cps_api_object_t, cps_db::vector_hash<char> > kc;

    cps_api_return_code_t rc = cps_api_handle_get(context,&param,0);

    size_t ix = 0;
    size_t mx = cps_api_object_list_size(param.list);
    for ( ; ix < mx ; ++ix ) {
    	cps_api_object_t o = cps_api_object_list_get(param.list,ix);
    	std::vector<char> key;
    	cps_db::generate_key(key,o);
    	kc[key]=o;
    }

    cps_api_object_list_guard lg(cps_api_object_list_create());
    if (cps_db::get_objects(*g.get(),obj,lg.get())) {
        mx = cps_api_object_list_size(lg.get());
        cps_api_object_t o = nullptr,found=nullptr;

        for ( ix = 0; ix < mx ; ++ix ) {

        	o = cps_api_object_list_get(lg.get(),ix);
        	std::vector<char> key;
        	cps_db::generate_key(key,o);
        	auto it = kc.find(key);

        	if (it!=kc.end()) {
        		cps_db::delete_object(*g.get(),o);
        		cps_api_object_set_type_operation(cps_api_object_key(o),cps_api_oper_DELETE);
        		cps_api_event_publish(context->events,o);
        	}
        }
    }

    if (!cps_db::store_objects(*g.get(),param.list)) {
    	EV_LOG(ERR,DSAPI,0,"CPS-RED-POLL","Failed to update database objects...");
    } else {

    }
}

static void init_cached_state(cps_api_operation_data_t *context_, cps_api_key_t *key_) {
	std::vector<char> key;

	cps_api_qualifier_t qual = cps_db::UPDATE_QUAL(key_,cps_api_qualifier_JOURNAL);
	if (!cps_db::generate_key(key,key_)) {
    	EV_LOG(ERR,DSAPI,0,"CPS-RED-POLL","Failed to generate a key... memory or other issue");
		return;
	}

	(void)cps_db::UPDATE_QUAL(key_,qual);
	cps_api_object_list_guard lg(cps_api_object_list_create());

	{
		cps_db::connection_cache::guard g(_conn_cache,_conn_cache.get());
		if (g.get()==nullptr) {
			EV_LOG(ERR,DSAPI,0,"CPS-SYNC-RED","No db... can't proceed");
			return ;
		}
		cps_db::get_objects(*g.get(),key,lg.get());
	}

	size_t ix = 0;
	size_t mx = cps_api_object_list_size(lg.get());

	while (cps_api_object_list_size(lg.get()) > 0) {
		cps_api_object_t obj = cps_api_object_list_get(lg.get(),0);

	    cps_api_transaction_params_t param;
	    if (cps_api_transaction_init(&param)!=cps_api_ret_code_OK) {
	    	EV_LOG(ERR,DSAPI,0,"CPS-SERVICE","No memory... ");
	        return ;
	    }
	    cps_api_transaction_guard tr(&param);
	    if (cps_api_object_list_append(param.change_list,obj)) {
	    	cps_api_object_list_remove(lg.get(),0);
	    }
	    cps_api_handle_commit(context_,&param,0);
	}
	sync_to_db(context_,key_);
}



//Communications aspects...
static bool  cps_api_handle_get(cps_api_operation_data_t *op, int fd,size_t len) {

    cps_api_get_params_t param;
    if (cps_api_get_request_init(&param)!=cps_api_ret_code_OK) {
        return false;
    }
    cps_api_get_request_guard grg(&param);

    cps_api_object_t filter = cps_api_receive_object(fd,len);
    if (filter==NULL) {
        return false;
    }

    if (!cps_api_object_list_append(param.filters,filter)) {
        cps_api_object_delete(filter);
        return false;
    }

    cps_api_return_code_t rc = cps_api_handle_get(op,&param,0);

    if (rc!=cps_api_ret_code_OK) {
        return cps_api_send_return_code(fd,cps_api_msg_o_RETURN_CODE,rc);
    } else {
        //send all objects in the list
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(param.list);
        for ( ; (ix < mx) ; ++ix ) {
            cps_api_object_t obj = cps_api_object_list_get(param.list,ix);
            if (obj==NULL) continue;
            if (!cps_api_send_one_object(fd,cps_api_msg_o_GET_RESP,obj)) return false;
        }
        if (!cps_api_send_header(fd,cps_api_msg_o_GET_DONE,0)) return false;
    }

    return true;
}

static bool cps_api_handle_commit(cps_api_operation_data_t *op, int fd, size_t len) {
    std_rw_lock_read_guard g(&op->db_lock);
    cps_api_return_code_t rc =cps_api_ret_code_OK;

    //receive changed object
    cps_api_object_guard o(cps_api_receive_object(fd,len));
    if (!o.valid()) {
        return false;
    }

    //initialize transaction
    cps_api_transaction_params_t param;
    if (cps_api_transaction_init(&param)!=cps_api_ret_code_OK) {
        return false;
    }

    cps_api_transaction_guard tr(&param);

    //add request to change list
    if (!cps_api_object_list_append(param.change_list,o.get())) return false;
    cps_api_object_t object_to_edit = o.get();

    o.release();


    //check the previous data sent by client
    uint32_t act;
    if (!cps_api_receive_header(fd,act,len)) {
        return false;
    }
    if(act!=cps_api_msg_o_COMMIT_PREV) {
        return false;
    }

    //read and add to list the previous object if passed
    if (len > 0) {
        o.set(cps_api_receive_object(fd,len));
        if (!o.valid()) {
            return false;
        }
        if (!cps_api_object_list_append(param.prev,o.get())) return false;
        o.release();
    }

    rc = cps_api_handle_commit(op,&param,0);

    if (rc!=cps_api_ret_code_OK) {
        return cps_api_send_return_code(fd,cps_api_msg_o_RETURN_CODE,rc);
    } else {
        cps_api_object_t cur = cps_api_object_list_get(param.change_list,0);

        if (cps_api_object_list_size(param.prev)==0) {
            cps_api_object_list_create_obj_and_append(param.prev);
        }
        cps_api_object_t prev = cps_api_object_list_get(param.prev,0);

        if (cur==NULL || prev==NULL) {
            EV_LOG(ERR,DSAPI,0,"COMMIT-REQ","response to request was invalid - cur=%d,prev=%d.",
                    (cur!=NULL),(prev!=NULL));
            return false;
        }

        if (!cps_api_send_one_object(fd,cps_api_msg_o_COMMIT_OBJECT,cur) ||
                !cps_api_send_one_object(fd,cps_api_msg_o_COMMIT_OBJECT,prev)) {
            EV_LOG(ERR,DSAPI,0,"COMMIT-REQ","Failed to send response to commit... ");
            return false;
        }
    }

    return true;
}

static bool cps_api_handle_revert(cps_api_operation_data_t *op, int fd, size_t len) {
    std_rw_lock_read_guard g(&op->db_lock);

    cps_api_return_code_t rc =cps_api_ret_code_OK;

    cps_api_object_guard o(cps_api_receive_object(fd,len));
    if (!o.valid()) return false;

    cps_api_transaction_params_t param;
    if (cps_api_transaction_init(&param)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_guard tr(&param);

    if (!cps_api_object_list_append(param.prev,o.get())) return false;

    cps_api_object_t l = o.release();

    size_t func_ix = 0;
    size_t func_mx = op->db_functions.size();
    for ( ; func_ix < func_mx ; ++func_ix ) {
        cps_api_registration_functions_t *p = &(op->db_functions[func_ix]);
        if ((p->_rollback_function!=NULL) &&
                (cps_api_key_matches(cps_api_object_key(l),&p->key,false)==0)) {
            rc = p->_rollback_function(p->context,&param,0);
            break;
        }
    }

    return cps_api_send_return_code(fd,cps_api_msg_o_RETURN_CODE,rc);
}

static bool cps_api_handle_stats(cps_api_operation_data_t *op, int fd, size_t len) {
    std_rw_lock_read_guard g(&op->db_lock);

    cps_api_object_guard og(cps_api_object_create());
    if (!og.valid()) return false;

    cps_api_key_init(cps_api_object_key(og.get()),cps_api_qualifier_OBSERVED,
            cps_api_obj_stat_e_OPERATIONS,0,0);

    cps_api_obj_stats_type_t cur = cps_api_obj_stat_BEGIN;
    for ( ; cur < cps_api_obj_stat_MAX ; cur= (cps_api_obj_stats_type_t)(cur+1) ) {
        cps_api_object_attr_add_u64(og.get(),cur,op->get_stat(cur));
    }

    if (!cps_api_send_one_object(fd,cps_api_msg_o_STATS,og.get())) return false;

    return true;
}

static bool  _some_data_( void *context, int fd ) {
    cps_api_operation_data_t *p = (cps_api_operation_data_t *)context;

    uint32_t op;
    size_t len;

    if(!cps_api_receive_header(fd,op,len)) return false;

    if (op==cps_api_msg_o_GET) return cps_api_handle_get(p,fd,len);
    if (op==cps_api_msg_o_COMMIT_CHANGE) return cps_api_handle_commit(p,fd,len);
    if (op==cps_api_msg_o_REVERT) return cps_api_handle_revert(p,fd,len);
    if (op==cps_api_msg_o_STATS) return cps_api_handle_stats(p,fd,len);
    return true;
}

static bool register_one_key(cps_api_operation_data_t *p, cps_api_key_t &key) {

    //initial sync to db once application registers
	init_cached_state(p,&key);

	cps_api_object_owner_reg_t r;
    r.addr = p->service_data.address;
    memcpy(&r.key,&key,sizeof(r.key));
    if (!cps_api_ns_register(p->ns_handle,r)) {
        close(p->ns_handle);
        return false;
    }
    return true;
}

static bool reconnect_with_ns(cps_api_operation_data_t *data) {
    if (!cps_api_ns_create_handle(&data->ns_handle)) {
        data->ns_handle = STD_INVALID_FD;
        return false;
    }
    if (std_socket_service_client_add(data->handle,data->ns_handle)!=STD_ERR_OK) {
        close(data->ns_handle);
        return false;
    }
    size_t ix = 0;
    size_t mx = data->db_functions.size();
    for ( ; ix < mx ; ++ix ) {
        if(!register_one_key(data,data->db_functions[ix].key)) {
            close(data->ns_handle); data->ns_handle=STD_INVALID_FD;
            return false;
        }
    }
    data->inc_stat(cps_api_obj_stat_NS_CONNECTS);
    return true;
}

void cps_api_operation_data_t::insert_functions(cps_api_registration_functions_t*fun) {
    auto it = db_functions.begin();
    auto end = db_functions.end();

    size_t key_size = cps_api_key_get_len(&fun->key);

    for ( ; it != end ; ++it ) {
        if (cps_api_key_matches(&fun->key,&it->key,false)==0) {
            size_t target_len = cps_api_key_get_len(&it->key);
            if (key_size < target_len) continue;
            char buffA[CPS_API_KEY_STR_MAX];
            char buffB[CPS_API_KEY_STR_MAX];
            EV_LOG(INFO,DSAPI,0,"NS","Inserting %s after %s",
                    cps_api_key_print(&it->key,buffA,sizeof(buffA)-1),
                    cps_api_key_print(&fun->key,buffB,sizeof(buffB)-1)
                    );
            db_functions.insert(it,*fun);
            return ;
        }
    }
    db_functions.push_back(*fun);
}

cps_api_return_code_t cps_api_register(cps_api_registration_functions_t * reg) {
    STD_ASSERT(reg->handle!=NULL);
    cps_api_operation_data_t *p = (cps_api_operation_data_t *)reg->handle;
    std_rw_lock_write_guard g(&p->db_lock);

    if (p->ns_handle==STD_INVALID_FD) {
        reconnect_with_ns(p);
    }
    p->insert_functions(reg);

    if (p->ns_handle!=STD_INVALID_FD) {
        if (!register_one_key(p,reg->key)) {
            close(p->ns_handle);
            p->ns_handle = STD_INVALID_FD;
        }
    }
    return cps_api_ret_code_OK;
}


static void _timedout(void * context) {
    cps_api_operation_data_t *p = (cps_api_operation_data_t *)context;
    STD_ASSERT(p!=nullptr);
    if (p->ns_handle==STD_INVALID_FD) {
        std_rw_lock_write_guard g(&p->db_lock);
        reconnect_with_ns(p);
    }
}

static bool _del_client(void * context, int fd) {
    cps_api_operation_data_t *p = (cps_api_operation_data_t *)context;
    STD_ASSERT(p!=nullptr);
    if (p->ns_handle==fd) {
        std_rw_lock_write_guard g(&p->db_lock);
        p->inc_stat(cps_api_obj_stat_NS_DISCONNECTS);
        p->ns_handle = STD_INVALID_FD;
    }
    return true;
}


void redis_poll_cycle(cps_api_operation_data_t *p) {
	//read from the handle to determine characteristics... for now just periodically poll
	std_rw_lock_read_guard rg(&p->db_lock);

    size_t func_ix = 0;
    size_t func_mx = p->db_functions.size();

    for ( ; func_ix < func_mx ; ++func_ix ) {
        cps_api_registration_functions_t *rf = &(p->db_functions[func_ix]);

        if (rf->_read_function==nullptr) continue;

        sync_to_db(p,&rf->key);
    }
}

//general purpose polling is not really that important...
void redis_support_loop(cps_api_operation_data_t *p) {
	while (true) {
		std_usleep(1000*1000*p->_polling_iteration);
		redis_poll_cycle(p);
	}
}

cps_api_return_code_t cps_api_operation_subsystem_init(
        cps_api_operation_handle_t *handle, size_t number_of_threads) {

    cps_api_operation_data_t *p = new cps_api_operation_data_t;
    if (p==NULL) {
    	return cps_api_ret_code_ERR;
    }

    p->handle = NULL;
    memset(&p->service_data,0,sizeof(p->service_data));
    std_mutex_lock_init_recursive(&p->mutex);

    std_rw_lock_create_default(&p->db_lock);

    for ( size_t ix = cps_api_obj_stat_BEGIN, mx =cps_api_obj_stat_MAX ; ix < mx ; ++ix ) {
        p->stats[ix] = 0;
    }
    p->service_data.name = "CPS_API_instance";
    cps_api_create_process_address(&p->service_data.address);
    p->service_data.thread_pool_size = number_of_threads;
    p->service_data.some_data = _some_data_;
    p->service_data.timeout = _timedout;
    p->service_data.del_client = _del_client;
    p->service_data.context = p;

    p->redis_poller = new std::thread([&p]() { redis_support_loop(p); } );

    if (cps_api_event_client_connect(&p->events)!=cps_api_ret_code_OK) {
    	delete p;
    	return cps_api_ret_code_ERR;
    }

    if (std_socket_service_init(&p->handle,&p->service_data)!=STD_ERR_OK) {
        delete p;
        return cps_api_ret_code_ERR;
    }

    //@TODO move this when cleaning up sockets/connection should be handled by the connections init function (eg.. unix, zmq, )
    if (p->service_data.address.type==e_std_sock_UNIX) {
        cps_api_set_cps_file_perms(p->service_data.address.address.str);
    }

    if (std_socket_service_run(p->handle)!=STD_ERR_OK) {
        std_socket_service_destroy(p->handle);
        delete p;
        return cps_api_ret_code_ERR;
    }

    {
        std_rw_lock_write_guard g(&p->db_lock);
        reconnect_with_ns(p);
    }
    *handle = p;

    return cps_api_ret_code_OK;
}

