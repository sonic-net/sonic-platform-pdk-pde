/*
 * cps_zmq_comm_utils.h
 *
 *  Created on: Mar 28, 2016
 *      Author: cwichmann
 */

#ifndef INC_CPS_ZMQ_COMM_UTILS_H_
#define INC_CPS_ZMQ_COMM_UTILS_H_


#include <string>
#include <vector>
#include <zmq.hpp>

namespace cps_zmq {

struct zmq_send_param {
	enum class field_type_t : int { field_type_MSG=0, field_type_DATA=1};
	field_type_t type;
	union {
		zmq::message_t *msg;
		struct {
			const void *data;
			size_t len;
		} d;
	} un;
	void set_msg(zmq::message_t *m) { un.msg = m; type = field_type_t::field_type_MSG; }
	void set_data(const void *data, size_t len) { un.d.data = data; un.d.len = len; type = field_type_t::field_type_DATA; }
};

bool send_helper(zmq::socket_t *sock, std::vector<zmq_send_param> &params);
bool recv_helper(zmq::socket_t *sock, std::vector<zmq::message_t> &params);

std::string dump(zmq::message_t &params);

}

#endif /* INC_CPS_ZMQ_COMM_UTILS_H_ */
