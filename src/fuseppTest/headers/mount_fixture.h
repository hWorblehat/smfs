/*
 * mount_fixture.h
 *
 *  Created on: 2 Jan 2017
 *      Author: rowan
 */

#ifndef MOUNT_FIXTURE_H_
#define MOUNT_FIXTURE_H_

#include "fuse.hpp"
#include <iostream>

namespace fusepp {
namespace testing {

static thread_local mount * testMount;

mount * get_test_mount() {
	return testMount;
}

void set_test_mount(mount * mount) {
	testMount = mount;
}

void unset_test_mount() {
	std::cerr << "Setting testMount to null";
	testMount = nullptr;
}

}
}

#endif /* MOUNT_FIXTURE_H_ */
