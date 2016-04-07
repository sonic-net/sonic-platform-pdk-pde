/*
 * cps_zmq_utils_dump_data.cpp
 *
 *  Created on: Mar 27, 2016
 *      Author: cwichmann
 */


#include "cps_zmq_comm_utils.h"

#include "cps_string_utils.h"

#include <sstream>
#include <iostream>
#include <string>

#include <zmq.hpp>

namespace cps_zmq {

std::string dump(zmq::message_t &params) {
	void *p = params.data();
	bool more = params.more();
	size_t len = params.size();
	std::string s =cps_string::sprintf("Message - Len(%d) More(%d)\n",(int)len,more);
	s+= cps_string::tostring(p,len);
	return s;
}

bool recv_helper(zmq::socket_t *sock, std::vector<zmq::message_t> &msg) {
    //  Read all frames off the wire, reject if bogus
	const size_t MAX_FRAMES=20;
	const size_t MIN_LIKELY_FRAMES=5;

	msg.resize(MIN_LIKELY_FRAMES);

	bool rc = false;
    for (size_t frame_nbr = 0; frame_nbr < MAX_FRAMES; frame_nbr++) {
    	if (msg.size() > frame_nbr) {
    		msg.resize(msg.size()+1);
    	}

    	try {
    		bool rc = sock->recv(&msg[frame_nbr],0);
    		if (!rc) break;
    	} catch (zmq::error_t &err) {
    		break;
    	}

    	if (!msg[frame_nbr].more()) {
    		if (msg.size()!=(frame_nbr+1)) {
    			msg.resize(frame_nbr+1);
    			rc = true;
    		}
    		break;
    	}
    }
    return rc;
}

bool send_helper(zmq::socket_t *sock, std::vector<zmq_send_param> &params) {
	auto it = params.begin();
	auto end = params.end();

	bool rc = false;

	for ( ; it != end ; ++it ) {
		int flags = (it + 1) == end ? 0 : ZMQ_SNDMORE;
		assert(it->type==zmq_send_param::field_type_t::field_type_DATA || it->type ==zmq_send_param::field_type_t::field_type_MSG);
		try {
			if (it->type==zmq_send_param::field_type_t::field_type_DATA) {
				rc = sock->send(it->un.d.data,it->un.d.len,flags);
			} else {
				rc = sock->send(it->un.msg,flags);
			}

		} catch (zmq::error_t &err) {
			rc = false;
		}
		if (!rc) break;
	}

	return rc;
}

}
