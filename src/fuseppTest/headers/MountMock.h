/*
 * MountMock.h
 *
 *  Created on: 2 Jan 2017
 *      Author: rowan
 */

#ifndef MOUNTMOCK_H_
#define MOUNTMOCK_H_

#include "fuse.hpp"
#include "gmock/gmock.h"

namespace fusepp {
namespace testing {

class MockMount : public mount {
public:
	MOCK_METHOD1(get_node, std::shared_ptr<node>(path_t rel_path));
};

class MockNode : public node {
public:
	MockNode(path_t &rel_path) : node(rel_path) {}

	MOCK_METHOD1(getattr,
			void(struct stat *statbuf));
	MOCK_METHOD2(readlink,
			void(char *link, size_t size));
	MOCK_METHOD1(create,
			void(mode_t mode));
	MOCK_METHOD1(mkfifo,
			void(mode_t mode));
	MOCK_METHOD2(mknod,
			void(mode_t mode, dev_t dev));
	MOCK_METHOD1(mkdir,
			void(mode_t mode));
	MOCK_METHOD0(unlink,
			void());
	MOCK_METHOD0(rmdir,
			void());
	MOCK_METHOD1(symlink,
			void(path_t target));
	MOCK_METHOD1(rename,
			void(path_t new_name));
	MOCK_METHOD1(link,
			void(path_t target));
	MOCK_METHOD1(chmod,
			void(mode_t mode));
	MOCK_METHOD2(chown,
			void(uid_t uid, gid_t gid));
	MOCK_METHOD1(truncate,
			void(off_t new_length));
	MOCK_METHOD1(open,
			file_handle*(int flags));
	MOCK_METHOD1(statvfs,
			void(struct statvfs * statbuf));

#ifdef HAVE_SYS_XATTR_H
	MOCK_METHOD4(setxattr,
			void(xattr_name_t &name, char const * value, size_t size, int flags));
	MOCK_METHOD3(getxattr,
			int (xattr_name_t &name, char * buf, size_t size));
	MOCK_METHOD2(listxattr, int(char * buf, size_t size));
	MOCK_METHOD1(removexattr, void(xattr_name_t &name));
#endif
};

}
}

#endif /* MOUNTMOCK_H_ */
