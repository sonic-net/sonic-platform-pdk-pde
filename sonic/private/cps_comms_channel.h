/*
 * cps_comms_channel.h
 *
 *  Created on: Mar 15, 2016
 *      Author: cwichmann
 */

#ifndef SONIC_OBJECT_LIBRARY_SONIC_PRIVATE_CPS_COMMS_CHANNEL_H_
#define SONIC_OBJECT_LIBRARY_SONIC_PRIVATE_CPS_COMMS_CHANNEL_H_

#include <string>
#include <vector>

class CPSInternalComms {
public:
	virtual bool init(const std::string &init_params) = 0;
	virtual bool bind(const std::string &address)=0;
	virtual bool connect(const std::string &address)=0;

	virtual bool create_address(std::string &address, std::vector<std::string> &lst)=0;

	//send a message.  Set the more flag to false to indicate last message
	//means that this should be single message accessed (eg.. one thread accessed at a time)
	//to avoid partial messages
	virtual int send(const void * data, size_t len, bool more)=0;

	//preallocated message buffer
	virtual int recv(std::vector<char> &data)=0;

	virtual ~CPSInternalComms()=0;
};


#endif /* SONIC_OBJECT_LIBRARY_SONIC_PRIVATE_CPS_COMMS_CHANNEL_H_ */
