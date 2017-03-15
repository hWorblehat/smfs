#ifndef FUSEPP_MOCKS_H
#define FUSEPP_MOCKS_H

#include "fuse.hpp"
#include "gmock/gmock.h"

namespace fusepp {

class MockNodeHandle : public NodeHandle {
public:
	MockNodeHandle();
	virtual ~MockNodeHandle();

	MOCK_METHOD1(getattr,
			void(struct stat *statbuf));
	MOCK_METHOD1(fsync,
			void(bool datasync));
	MOCK_METHOD4(ioctl,
			void(int cmd, void * arg, unsigned int flags, void * data));
};

class MockFileHandle : public virtual FileHandle {
public:
	MockFileHandle();
	virtual ~MockFileHandle();

	MOCK_METHOD2(read,
			std::shared_ptr<Buffer>(size_t nbytes, off_t offset));
	MOCK_METHOD2(write,
			int(Buffer& buffer, off_t offset));
	MOCK_METHOD0(flush,
			void());
	MOCK_METHOD2(lock,
			void(int cmd, struct flock * flock));
	MOCK_METHOD1(truncate,
			void(off_t newLength));

	MOCK_METHOD1(getattr,
			void(struct stat *statbuf));
	MOCK_METHOD1(fsync,
			void(bool datasync));
	MOCK_METHOD4(ioctl,
			void(int cmd, void * arg, unsigned int flags, void * data));
};

class MockDirHandle : public virtual DirHandle {
public:
	MockDirHandle();
	virtual ~MockDirHandle();

	MOCK_METHOD1(readdir,
			struct dirent*(struct dirent * entry));

	MOCK_METHOD1(getattr,
			void(struct stat *statbuf));
	MOCK_METHOD1(fsync,
			void(bool datasync));
	MOCK_METHOD4(ioctl,
			void(int cmd, void * arg, unsigned int flags, void * data));
};

class MockNode : public virtual Node {
public:
	MockNode(path_t rel_path);
	virtual ~MockNode();

	MOCK_METHOD1(getattr,
			void(struct stat *statbuf));
	MOCK_METHOD2(readlink,
			void(char *link, size_t size));
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
	MOCK_METHOD1(open,
			FileHandle1*(int flags));
	MOCK_METHOD2(createAndOpen,
			FileHandle1*(mode_t mode, int flags));
	MOCK_METHOD1(statfs,
			void(struct statvfs * statbuf));
	MOCK_METHOD4(setxattr,
			void(char const * name, char const * value, size_t size, int flags));
	MOCK_METHOD3(getxattr,
			int(char const * name, char * buf, size_t size));
	MOCK_METHOD2(listxattr,
			int(char * buf, size_t size));
	MOCK_METHOD1(removexattr,
			void(char const * name));
	MOCK_METHOD1(truncate,
			void(off_t newLength));
	MOCK_METHOD1(opendir,
			DirHandle1*(int flags));
	MOCK_METHOD1(access,
			void(int mode));
	MOCK_METHOD1(utimens,
			void(struct timespec const tv[2]));
};

class MockMount : public virtual Mount {
public:
	MockMount();
	virtual ~MockMount();

	MOCK_METHOD1(get_node,
			std::shared_ptr<Node1>(path_t rel_path));
};

}  // namespace fusepp

#endif //FUSEPP_MOCKS_H
