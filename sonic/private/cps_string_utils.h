/*
 * cps_string_utils.h
 *
 *  Created on: Mar 26, 2016
 *      Author: cwichmann
 */

#ifndef INC_CPS_STRING_UTILS_H_
#define INC_CPS_STRING_UTILS_H_

#include <vector>
#include <string>

namespace cps_string{
	std::string sprintf(const char *fmt, ...);
	std::vector<std::string> split(const std::string &str, const std::string &reg);

	inline std::vector<std::string> split_ws(const std::string &str) {
		return split(str,"[^\\s]+");
	}

	std::string tostring(const void *data, size_t len);
};


#endif /* INC_CPS_STRING_UTILS_H_ */
