/*
 * mount_fixture.h
 *
 *  Created on: 2 Jan 2017
 *      Author: rowan
 */

#ifndef MOUNT_FIXTURE_H_
#define MOUNT_FIXTURE_H_

#include "fuse.hpp"

namespace fusepp {
namespace testing {

static thread_local Mount * testMount;

Mount * get_test_mount() {
	return testMount;
}

void set_test_mount(Mount * mount) {
	testMount = mount;
}

void unset_test_mount() {
	testMount = nullptr;
}

}
}

#endif /* MOUNT_FIXTURE_H_ */
