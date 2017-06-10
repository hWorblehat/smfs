/*
 * fuse.cpp
 *
 *  Created on: 20 Jul 2016
 *      Author: rowan
 */

#include "fuse.hpp"
#include "fusepp/internal/impl.hpp"

#include <tuple>
#include <type_traits>

namespace fusepp {

#define NOT_IMP(sig) \
sig { \
	throw fuse_error(ENOSYS); \
}

template<unsigned I, typename... Args> struct Arguments;

template<typename Sig> struct Function;

template<typename Class, typename Ret, typename... Args>
struct Function<Ret (Class::*)(Args...)> {
	using Return = Ret;

	using Arguments = std::tuple<Args...>;

	template<std::size_t I>
	using Argument = typename std::tuple_element<I, Arguments>::type;
};

#define FUNCTION(func) Function<decltype(&func)>

#define NIN(func, ...) NOT_IMP(FUNCTION(func)::Return func(__VA_ARGS__))

#define ARGS0(func)
#define ARGS1(func) FUNCTION(func)::Argument<0>
#define ARGS2(func) ARGS1(func), FUNCTION(func)::Argument<1>
#define ARGS3(func) ARGS2(func), FUNCTION(func)::Argument<2>
#define ARGS4(func) ARGS3(func), FUNCTION(func)::Argument<3>
#define ARGS5(func) ARGS4(func), FUNCTION(func)::Argument<4>
#define ARGS6(func) ARGS5(func), FUNCTION(func)::Argument<5>
#define ARGS7(func) ARGS6(func), FUNCTION(func)::Argument<6>

#define NI(N, func) NIN(func, ARGS##N(func))

NI(1, NodeHandle1::fsync)
NI(4, NodeHandle1::ioctl)

NI(2, FileHandle1::read)
NI(2, FileHandle1::write)
NI(0, FileHandle1::flush)
NI(2, FileHandle1::lock)

NI(0, DirHandle1::readdir)

NI(1, Node1::getattr)
NI(1, Node1::readlink)
NI(1, Node1::mkfifo)
NI(2, Node1::mknod)
NI(1, Node1::mkdir)
NI(0, Node1::unlink)
NI(0, Node1::rmdir)
NI(1, Node1::symlink)
NI(1, Node1::rename)
NI(1, Node1::link)
NI(1, Node1::chmod)
NI(2, Node1::chown)
NI(1, Node1::open)
NI(1, Node1::statfs)
NI(3, Node1::setxattr)
NI(1, Node1::xattrSize)
NI(2, Node1::getxattr)
NI(0, Node1::xattrListSize)
NI(1, Node1::listxattr)
NI(1, Node1::removexattr)
NI(1, Node1::truncate)
NI(1, Node1::opendir)
NI(1, Node1::access)
NI(1, Node1::utime)

std::unique_ptr<FileHandle1> Node1::createAndOpen(mode_t mode, int flags) {
	mknod(mode, 0);
	return open(flags);
}

/**
 * @return The pointer to a @ref mount object held in the current fuse context.
 */
static Mount1* get_mount_real() {
	return (Mount1 *) fuse_get_context()->private_data;
}

int main(int argc, char *argv[], Mount1 *mount) {
	struct fuse_operations operations;
	internal::with_mount1<&fusepp::get_mount_real>::bind(&operations);
	return fuse_main(argc, argv, &operations, mount);
}

} // namespace fuse
