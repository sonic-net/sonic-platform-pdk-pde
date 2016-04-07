/*
 * cps_string_utils.cpp
 *
 *  Created on: Mar 26, 2016
 *      Author: cwichmann
 */

#include "cps_string_utils.h"

#include <cstdio>
#include <cstdarg>
#include <regex>

namespace cps_string{

std::string sprintf(const char *fmt, ...) {
	std::va_list args_len;
	va_start(args_len,fmt);
	std::va_list args_real;
	va_copy(args_real,args_len);

	size_t len = std::vsnprintf(nullptr,0,fmt,args_len)+1;
	va_end(args_len);

	std::unique_ptr<char[]> buf( new char[ len ] );
	std::vsnprintf(buf.get(),len,fmt,args_real);
	va_end(args_real);

	return std::string(buf.get());
}

std::vector<std::string> split(const std::string &str, const std::string &reg) {
	std::regex re(reg);
	std::sregex_token_iterator first{str.begin(),str.end(),re,-1},last;
	return {first,last};
}

std::string tostring(const void *data, size_t len) {
	std::string s;
	for (size_t ix =0; ix<len ; ++ix ) {
		if ((ix%16==0) && (ix!=0)) s+="\n";
		s+=cps_string::sprintf("%02x ",((unsigned char*)data)[ix]);
	}
	s+="\n";
	return s;
}

}
