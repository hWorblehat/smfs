/*
 * util.hpp
 *
 *  Created on: 25 Jul 2016
 *      Author: rowan
 */

#ifndef FUSEPP_UTIL_HPP_
#define FUSEPP_UTIL_HPP_

#include <fusepp/common.hpp>

namespace fusepp {

/**
 * @brief Checks whether the given return value indicates an error.
 *
 * The given return value, expected to be a returned value from a function,
 * is checked for negativity. If it is negative, then a fuse_error is thrown
 * using the current value of errno (and the value of errno is reset to zero).
 *
 * @param retval The return value to check.
 * @return retval (never negative).
 * @throws fuse_error if retval is negative.
 */
inline unsigned int check_ret(int retval) {
	if(retval<0) {
		throw fuse_error::from_errno();
	}
	return retval;
}

}

#endif /* FUSEPP_UTIL_HPP_ */
