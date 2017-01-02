/*
 * common.hpp
 *
 *  Created on: 25 Jul 2016
 *      Author: rowan
 */

#ifndef FUSEPP_COMMON_HPP_
#define FUSEPP_COMMON_HPP_

#include <stdexcept>
#include <cerrno>
#include <cstring>

namespace fusepp {

/**
 * Thrown when a fuse operation fails.
 */
struct fuse_error : std::runtime_error {

	/**
	 * The error code that caused this error to be raised.
	 */
	int const error;

	/**
	 * Constructor for fuse_error.
	 * @param error The error code that caused this error to be raised.
	 */
	fuse_error(int error) : std::runtime_error(strerror(error)), error(error) {}

	/**
	 * Creates a new fuse_error using the current value of errno as its error.
	 *
	 * This function also resets the value of errno to zero.
	 * @return The newly-created fuse_error.
	 */
	static fuse_error from_errno() {
		int err = errno;
		errno = 0;
		return fuse_error(err);
	}

};

} //namespace fuse

#endif /* FUSEPP_COMMON_HPP_ */
