/*
 * fuseppTest.cpp
 *
 *  Created on: 19 Nov 2016
 *      Author: rowan
 */

#include "mount_fixture.h"
#include "fusepp_mocks.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "fuse.hpp"
#include "fusepp/Buffer.h"
#include "fusepp/internal/impl.hpp"

#include "fusepp/internal/using_std.h"

using namespace fusepp;
using namespace fusepp::testing;

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;

class FuseppBindings : public ::testing::Test {
public:
	MockMount mount;
	MockNode node{"dummy"};
	MockFileHandle fileHandle;
	fuse_operations operations;

private:
	shared_ptr<Node> pNode{&node, [](MockNode*){}};

public:

	FuseppBindings() {
		set_test_mount(&mount);
		internal::with_mount1<get_test_mount>::bind(&operations);
	}

	void SetUp() {
		ON_CALL(mount, get_node(_)).WillByDefault(Return(pNode));
	}

	virtual ~FuseppBindings() {
		unset_test_mount();
	}

};

#define EXPECT_NODE_CALL(method) \
	EXPECT_CALL(mount, get_node(_)).Times(1);\
	EXPECT_CALL(node, method)

ACTION_P(SetStatNlink, n) {
	arg0->st_nlink = n;
}

TEST_F(FuseppBindings, forwardsDirectOperationsToNode) {
	struct stat statbuf;

	EXPECT_NODE_CALL(getattr(&statbuf))
			.Times(1)
			.WillOnce(SetStatNlink(17));

	EXPECT_EQ(0, operations.getattr("hello", &statbuf));
	EXPECT_EQ(17, statbuf.st_nlink);
}

TEST_F(FuseppBindings, convertsErrorsToErrorCodes) {
	char link[20];
	size_t size = 20;

	EXPECT_NODE_CALL(readlink(link, size))
			.Times(1)
			.WillOnce(Throw(fuse_error(3)));

	EXPECT_EQ(-3, operations.readlink("hello", link, size));
}

TEST_F(FuseppBindings, forwardsMknodCallsCorrectly) {
	mode_t mode = 0;
	dev_t dev = 67;

	//Check normally forwards to mknod
	EXPECT_NODE_CALL(mknod(mode, dev)).Times(1);
	EXPECT_EQ(0, operations.mknod("hello", mode, dev));

	//Check we forward to mkfifo when appropriate
	mode = S_IFIFO;
	EXPECT_NODE_CALL(mkfifo(mode)).Times(1);
	EXPECT_EQ(0, operations.mknod("hello", mode, dev));

	//Check errors are converted
	mode = 3;
	EXPECT_NODE_CALL(mknod(mode, dev))
			.Times(1)
			.WillOnce(Throw(fuse_error(12)));
	EXPECT_EQ(-12, operations.mknod("hello", mode, dev));
}

TEST_F(FuseppBindings, forwardsLinkOperationsToNode) {
	path_t newName = internal::convert_path("bob");

	EXPECT_NODE_CALL(rename(newName)).Times(1);
	EXPECT_EQ(0, operations.rename("hello", "bob"));

	EXPECT_NODE_CALL(rename(newName)).Times(1)
			.WillOnce(Throw(fuse_error(6)));
	EXPECT_EQ(-6, operations.rename("hello", "bob"));
}

TEST_F(FuseppBindings, openPutsFileHandleInFileInfo) {
	struct fuse_file_info info;

	EXPECT_NODE_CALL(open(info.flags))
			.Times(1)
			.WillOnce(Return(&fileHandle));

	EXPECT_EQ(0, operations.open("Hello", &info));
	EXPECT_EQ(&fileHandle, (void*) info.fh);
}

TEST_F(FuseppBindings, forwardsFHOperationsToFileHandle) {

	struct fuse_file_info info;
	info.fh = (uint64_t) &fileHandle;

	char mem[10];
	shared_ptr<Buffer> buffer = Buffer::create(mem, 10);

	struct fuse_bufvec* bv = nullptr;

	EXPECT_CALL(fileHandle, read(20, 0))
			.Times(1)
			.WillOnce(Return(buffer));

	int res = operations.read_buf("Hello", &bv, 20, 0, &info);
	EXPECT_EQ(0, res);
	ASSERT_NE(nullptr, bv);
	unique_ptr<fuse_bufvec> dispose(bv);
	ASSERT_EQ(1, bv->count);
	EXPECT_EQ(mem, bv->buf->mem);
}
