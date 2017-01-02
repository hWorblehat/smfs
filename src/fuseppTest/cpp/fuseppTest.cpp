/*
 * fuseppTest.cpp
 *
 *  Created on: 19 Nov 2016
 *      Author: rowan
 */

#include "../headers/mount_fixture.h"
#include "../headers/MountMock.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "fuse.hpp"
#include "fusepp/internal/impl.hpp"


using namespace fusepp;
using namespace fusepp::testing;

using ::testing::_;
using ::testing::Return;

TEST(fusepp, forwardsOperationsToMount) {
	MockMount mount;
	set_test_mount(&mount);

	fuse_operations operations;
	internal::with_mount<get_test_mount>::bind(&operations);

	MockNode node("hello");

	std::cerr << "huuuuu";

	EXPECT_CALL(mount, get_node("yo"))
//			.Times(1)
			.WillOnce(Return(std::shared_ptr<fusepp::node>(&node)));

	struct stat *statbuf;

	EXPECT_CALL(node, getattr(statbuf)).Times(1);

	int rc = operations.getattr("yo", statbuf);
	std::cerr << "Result: " << rc;
	//EXPECT_EQ(0, rc);

	unset_test_mount();
}
