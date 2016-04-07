/*
 * cps_service_interface.h
 *
 *  Created on: Mar 26, 2016
 *      Author: cwichmann
 */

#ifndef INC_CPS_SERVICE_INTERFACE_H_
#define INC_CPS_SERVICE_INTERFACE_H_

#include <string>
#include <algorithm>
#include <functional>

#include <stddef.h>

namespace cps_comms {

class comm_base_api_event_service {
public:
	virtual bool start(const std::string &addr)=0;
	virtual bool stop()=0;
	virtual ~comm_base_api_event_service(){}
};

class comm_base_api_pub {
public:
	virtual bool connect(const std::string &addr)=0;
	virtual bool pub(const void *key, size_t klen, const void *data, size_t len)=0;
	virtual ~comm_base_api_pub(){}
};

class comm_base_api_sub {
public:
	virtual bool connect(const std::string &publisher)=0;
	virtual bool sub(const void *key, size_t klen)=0;
	virtual bool event_loop(const std::function<void(const void *key, size_t klen, const void *data, size_t len)> &event_loop_cb,
			ssize_t timeout_in_ms=-1)=0;
	virtual ~comm_base_api_sub(){}
};

class comm_ns_api {
public:
	virtual bool connect(const std::string &nameserver) = 0;
	virtual bool find(void *key, size_t len, std::string &address)=0;

	virtual bool reg(void *key, size_t len);
	virtual ~comm_ns_api(){}
};

class comm_ops_api {
public:
	virtual bool connect(const std::string &nameserver) = 0;

	virtual bool find(void *key, size_t len, std::string &address)=0;

	virtual ~comm_ops_api(){}
};



class comm_endpoint_api {
public:
	virtual bool update_settings(const std::vector<std::string> &init_params) = 0;

	//listen to one or more addresses
	virtual bool bind(const std::string &address)=0;
	virtual bool connect(const std::string &address)=0;

	virtual bool connected(const std::string &address);
	virtual bool disconnected(const std::string &address);

	virtual bool create_address(std::string &address, std::vector<std::string> &pieces)=0;

	//send a message.  Set the more flag to false to indicate last message
	//means that this should be single message accessed (eg.. one thread accessed at a time)
	//to avoid partial messages
	virtual int send(const void * data, size_t len, bool more)=0;

	//preallocated message buffer
	virtual int recv(std::vector<char> &data, bool enlarge=true)=0;

	//preallocated message buffer
	virtual int recv(const char *data, size_t len)=0;

	virtual ~comm_endpoint_api()=0;
};

}



#endif /* INC_CPS_SERVICE_INTERFACE_H_ */
