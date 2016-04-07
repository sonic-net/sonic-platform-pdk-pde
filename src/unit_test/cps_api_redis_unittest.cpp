

#include "cps_string_utils.h"
#include "cps_db.h"
#include "cps_redis_utils.h"

#include <gtest/gtest.h>
#include "hiredis/hiredis.h"

TEST(cps_api_redis_ut,objects) {
	cps_db::connection con;
	con.connect();

	cps_api_object_list_guard lg(cps_api_object_list_create());
	cps_api_object_guard og(cps_api_object_create());

	cps_api_object_t obj = og.get();
	cps_api_key_t * key = cps_api_object_key(obj);

	ASSERT_TRUE(key!=nullptr);

    cps_api_key_set(key,0,1);
    cps_api_key_set(key,1,10);
    cps_api_key_set(key,2,0x0d);
    cps_api_key_set_len(key,3);

    ASSERT_TRUE(cps_api_object_attr_add(obj,0,"Cliff",6));
    ASSERT_TRUE(cps_api_object_attr_add_u16(obj,1,16));
    ASSERT_TRUE(cps_api_object_attr_add_u32(obj,2,32));
    ASSERT_TRUE(cps_api_object_attr_add_u64(obj,3,64));

    std::vector<char> k1,k2;

    ASSERT_TRUE(cps_db::generate_key(k1,obj)==cps_db::generate_key(k2,obj));
    std::cout << "First.... " << k1.size() << std::endl;
    std::cout << cps_string::tostring(&k1[0],k1.size()) << std::endl;
    std::cout << cps_string::tostring(&k2[0],k2.size()) << std::endl;
    k1.clear(); k2.clear();

    ASSERT_TRUE(cps_db::generate_key(k1,cps_api_object_key(obj))==cps_db::generate_key(k2,cps_api_object_key(obj)));
    std::cout << "Second.... " << k1.size() << std::endl;
    std::cout << cps_string::tostring(&k1[0],k1.size()) << std::endl;
    std::cout << cps_string::tostring(&k1[0],k2.size()) << std::endl;


	ASSERT_TRUE(cps_db::get_objects(con,obj,lg.get()));

	char buff[3000];
	for ( size_t ix = 0, mx = cps_api_object_list_size(lg.get()); ix < mx ; ++ix ) {
		cps_api_object_t o = cps_api_object_list_get(lg.get(),ix);
		cps_api_object_to_string(o,buff,sizeof(buff));
		std::cout << buff << std::endl;
	}

	ASSERT_TRUE(cps_db::store_object(con,obj));

	cps_api_object_guard ng(cps_api_object_create());

	cps_api_key_copy(cps_api_object_key(ng.get()),cps_api_object_key(og.get()));
	ASSERT_TRUE(cps_db::get_objects(con,ng.get(),lg.get()));

	for ( size_t ix = 0, mx = cps_api_object_list_size(lg.get()); ix < mx ; ++ix ) {
		cps_api_object_t o = cps_api_object_list_get(lg.get(),ix);
		cps_api_object_to_string(o,buff,sizeof(buff));
		std::cout << buff << std::endl;
	}

	ASSERT_TRUE(cps_db::delete_object(con,obj));
}

TEST(cps_api_redis_ut,raw) {

	cps_db::connection con;
	con.connect();

	cps_db::multi_start(con);

	cps_db::connection::operation_request_t e;
	e._object = nullptr;

	std::vector< cps_db::connection::operation_request_t> l;

	e._str="HSET";
	l.push_back(e);
	e._str="cliff_person";
	l.push_back(e);
	e._str="name";
	l.push_back(e);
	e._str="cliff";
	l.push_back(e);

	ASSERT_TRUE(con.operation(&l[0],l.size())==cps_api_ret_code_OK);
	//ASSERT_TRUE(con.operation(&l[0],l.size())==cps_api_ret_code_OK);

	std::vector<void*> data;

	ASSERT_TRUE(con.response(data));
	for ( auto it : data) {
		redisReply *p = (redisReply*) it;
		ASSERT_TRUE(p!=nullptr);

		cps_db::response resp(p);
		resp.dump();
	}
	ASSERT_TRUE(cps_db::multi_end(con));

	ASSERT_TRUE(con.operation(&l[0],l.size())==cps_api_ret_code_OK);

	std::cout << "Next..." << std::endl;
	data.clear();
	ASSERT_TRUE(con.response(data));
	for ( auto it : data) {
		redisReply *p = (redisReply*) it;
		ASSERT_TRUE(p!=nullptr);

		cps_db::response resp(p);
		resp.dump();
		ASSERT_TRUE(resp.is_ok_return());
	}

	cps_db::fetch_all_keys(con,"*",1,[](const void *data,size_t len){
		std::cout << "Key..." << cps_string::tostring(data,len) << std::endl;
	});
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

