/*
 * fuse.cpp
 *
 *  Created on: 20 Jul 2016
 *      Author: rowan
 */

// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26
#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "fuse.hpp"
#include "fusepp/internal/impl.hpp"

namespace fusepp {

/**
 * @return The pointer to a @ref mount object held in the current fuse context.
 */
inline mount* get_mount_real() {
	return (mount *) fuse_get_context()->private_data;
}

int main(int argc, char *argv[], mount *mount) {
	struct fuse_operations operations;
	internal::with_mount<&fusepp::get_mount_real>::bind(&operations);
	return fuse_main(argc, argv, &operations, mount);
}

} // namespace fuse
