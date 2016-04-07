/*
 * cps_buffered_ipc.cpp
 *
 *  Created on: Apr 5, 2016
 *      Author: cwichmann
 */



#include "cps_api_client_utils.h"


#include "std_file_utils.h"
#include "event_log.h"

class client_response {
	std::vector<char> _buff;
	size_t _buffered;
	int _fd=-1;
	void resize_buffer(ssize_t new_max);
public:
	client_response(size_t def_size=20000) { resize_buffer(def_size); }
	void init(int fd);

	bool write(const void *data, size_t len);
	bool flush();
};

void client_response::resize_buffer(ssize_t new_max) {
	_buff.resize(_buff.size()+new_max);
}

void client_response::init(int fd) {
	if (_buffered > 0 && _fd>=0) {
		if (!flush()) {
			EV_LOG(ERR,DSAPI,0,"CPS-IPC","Failed to flush buffer...");
		}
	}
	_buffered = 0;
	_fd = fd;
}

bool client_response::write(const void *data, size_t len) {
	while (len) {
		size_t cp = std::min(len,_buff.size()-_buffered);

		if (cp==0) {
			if (!flush()) return false;
			continue;
		}

		memcpy(&_buff[_buffered],data,cp);

		data = ((char*)data)+cp;
		len -= cp;
		_buffered+=cp;
	}
	return true;
}

bool client_response::flush() {
	if (_buffered>0) {
	    t_std_error rc = STD_ERR_OK;
	    int by = std_write(_fd,&_buff[0],_buffered,true,&rc);
	    if (by!=(_buffered)) return false;
	}
	_buffered=0;
	return true;
}

