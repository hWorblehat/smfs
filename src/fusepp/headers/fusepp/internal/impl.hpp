/*
 * impl.hpp
 *
 *  Created on: 19 Nov 2016
 *      Author: rowan
 */

#ifndef FUSEPP_INTERNAL_IMPL_HPP_
#define FUSEPP_INTERNAL_IMPL_HPP_

#include "fuse.hpp"
#include "fusepp/internal/Buffer.h"
#include "fusepp/internal/cfuse.h"

#include <utility>
#include <memory>
#include <type_traits>

using std::forward;

namespace fusepp {

#include "fusepp/internal/using_std.h"

static_assert(sizeof(std::uintptr_t)<=sizeof(decltype(fuse_file_info::fh)),
		"fuse_file_info 'fh' field not big enough to store pointer.");

namespace internal {

typedef Mount1 * (*getMount1)();

/**
 * Converts the given path string to a @ref path_t.
 * @param path The path string to convert.
 * @return The resultant @ref path_t.
 */
inline path_t convert_path(char const *path) {
	return path_t(path);
}

template<getMount1 get_mount>
struct with_mount1 {

	/**
	 * Gets a pointer to the @ref node under the current fuse context's @ref mount
	 * at the given path.
	 * @param path The path of the node to get.
	 * @return The requested node.
	 */
	static inline shared_ptr<Node1> get_node(char const *path) {
		return get_mount()->get_node(convert_path(path));
	}

	template<typename T>
	using Handle = std::enable_if_t<std::is_base_of<NodeHandle1, T>::value, T>;

	template<typename T>
	static inline Handle<T>* get_handle(fuse_file_info* fi) {
		return reinterpret_cast<T*>(fi->fh);
	}

	/**
	 * The type of a pointer to a function that takes the specified arguments and returns
	 * an `int`.
	 * @tparam The argument types of the function.
	 */
	template<typename... Args>
	using pOperation = int (*)(Args...);

	template<typename Sig, Sig F> struct returning_int_for;

	/**
	 * Takes a pointer to a function returning void and wraps it in a function that
	 * returns 0 on completion.
	 */
	template<typename... Args, void (*F)(Args...)>
	struct returning_int_for<void (*)(Args...), F> {
		static int wrapper(Args... args) {
			(*F)(forward<Args>(args)...);
			return 0;
		}
	};

	/**
	 * Wraps a function pointer `F` in a function that invokes `F`,
	 * catching any instances of @ref fuse_error thrown during execution.
	 * @tparam Sig The type of the function pointer to wrap
	 * @tparam F The function pointer to wrap
	 */
	template<typename Sig, Sig F> struct call_and_catch;

	/**
	 * Wraps a pointer `F` to a function that returns an `int` in a function that invokes `F`,
	 * catching any instances of @ref fuse_error thrown during execution.
	 * @tparam Args The argument types of the function pointer to wrap
	 * @tparam F The function pointer to wrap
	 */
	template<typename... Args, pOperation<Args...> F>
	class call_and_catch<pOperation<Args...>, F> {

		static int invoke(Args... args) {
			try {
				return (*F)(forward<Args>(args)...);
			} catch (fuse_error &fe) {
				return -fe.error;
			}
		}

	public:

		/**
		 * A pointer to the wrapper function defined by this class.
		 */
		static constexpr pOperation<Args...> ptr = &invoke;
	};

	/**
	 * Wraps a function pointer `fpointer` in a function that invokes `fpointer`,
	 * catching any instances of @ref fuse_error thrown during execution. If
	 * `fpointer` returns `void`, then the wrapper function returns `0` on
	 * successful completion.
	 *
	 * @param fpointer The function pointer to wrap.
	 * @return A pointer to the statically-defined wrapper function.
	 */
	#define P_CALL_AND_CATCH(fpointer) call_and_catch<decltype(fpointer), fpointer>::ptr

	/**
	 * Wraps a pointer `F` to a `void` function in a function that invokes `F`,
	 * catching any instances of @ref fuse_error thrown during execution, and returning `0`
	 * on successful completion.
	 * @tparam Args The argument types of the function pointer to wrap
	 * @tparam F The function pointer to wrap
	 */
	template<typename... Args, void (*F)(Args...)>
	class call_and_catch<void (*)(Args...), F> {
		using ret_int = returning_int_for<void (*)(Args...), F>;

	public:

		/**
		 * A pointer to the wrapper function defined by this class.
		 */
		static constexpr pOperation<Args...> ptr = P_CALL_AND_CATCH(&ret_int::wrapper);
	};

	template<typename Sig, Sig F> struct on_node;

	template<typename Ret, typename... Args, Ret (Node1::*F)(Args...)>
	struct on_node<Ret (Node1::*)(Args...), F> {
		static Ret call(char const * path, Args... args) {
			return std::bind(F, get_node(path), forward<Args>(args)...)();
		}
	};

	template<typename Ret, typename... Args, Ret (Node1::*F)(Args...)>
	class call_and_catch<Ret (Node1::*)(Args...), F> {
		using onNode = on_node<Ret (Node1::*)(Args...), F>;

	public:
		static constexpr pOperation<char const *, Args...> ptr = P_CALL_AND_CATCH(&onNode::call);
	};

	template<std::size_t I, typename... Args1>
	struct first_args {

		template<typename Arg, typename... Args2>
		struct second_args : public first_args<I-1, Args1..., Arg>::template second_args<Args2...> {};

	};

	template<typename... Args1>
	struct first_args<0, Args1...> {

		template<typename... Args2>
		struct second_args {
			template<typename T, typename Ret, Ret (Handle<T>::*F)(Args1..., Args2...)>
			struct method {
				using pOp = pOperation<char const *, Args1..., fuse_file_info*, Args2...>;

				static Ret call(char const *, Args1... args1, fuse_file_info* info, Args2... args2) {
					return (get_handle<T>(info)->*F)(forward<Args1>(args1)..., forward<Args2>(args2)...);
				}
			};
		};

	};

	template<typename Sig, Sig F> struct count_args;
	template<typename T, typename Ret, typename... Args, Ret (Handle<T>::*F)(Args...)>
	struct count_args<Ret (T::*)(Args...), F> {
		static constexpr std::size_t num = sizeof...(Args);
	};

	template<typename Sig, Sig F, std::size_t I=count_args<Sig, F>::num> struct on_file_handle;

	template<std::size_t I, typename T, typename Ret, typename... Args, Ret (Handle<T>::*F)(Args...)>
	struct on_file_handle<Ret (T::*)(Args...), F, I>
	: public first_args<I>::template second_args<Args...>::template method<T, Ret, F>{};

	template<typename T, typename Ret, typename... Args, Ret (Handle<T>::*F)(Args...)>
	class call_and_catch<Ret (T::*)(Args...), F> {
		using onFH = on_file_handle<Ret (T::*)(Args...), F>;

	public:
		static constexpr typename onFH::pOp ptr = P_CALL_AND_CATCH(&onFH::call);
	};

#define P_FH_CALL_AND_CATCH(fpointer, index) P_CALL_AND_CATCH((&on_file_handle<decltype(fpointer), fpointer, index>::call))

	/**
	 * A wrapper round any any operation that 'links' two file paths
	 * (e.g. rename, symlink) that converts both the source and destination path strings
	 * to @ref path_t objects.
	 */
	template<void (Node1::*F)(path_t)>
	struct link_op {

		/**
		 * Performs a linking operation between the two file paths.
		 * @param from The source path string.
		 * @param to The destination path string.
		 */
		static void wrapper(char const * from, char const * to) {
			std::bind(F, get_node(from), convert_path(to))();
		}
	};

	/**
	 * Wraps a pointer to a member function `F` of a @ref node that 'links' it to another node
	 * in a function that invokes `F`, catching any instances of @ref fuse_error thrown during
	 * execution, and returning `0` on successful completion.
	 * @tparam F The member function to wrap.
	 */
	template<void (Node1::*F)(path_t)> struct call_link_and_catch :
			public call_and_catch<void (*)(char const *, char const *), link_op<F>::wrapper> {};

	/**
	 * An alias for `call_link_and_catch<F>::ptr`.
	 * @tparam F The member function of a @ref node to wrap.
	 * @see call_link_and_catch
	 */
	template<void (Node1::*F)(path_t)>
	static constexpr pOperation<char const *, char const *> call_link_and_catch_p =
			call_link_and_catch<F>::ptr;

	/**
	 * A branching `mknod` function.
	 * Calls `mkfifo` on the @ref node if `mode` indicates a fifo, and `mknod` otherwise.
	 * @param path The path string to the node to invoke.
	 * @param mode The node create mode flags.
	 * @param dev The device type to create, if creating a device file.
	 */
	static void mknod_real(char const * path, mode_t mode, dev_t dev) {
		if(S_ISFIFO(mode)) {
			get_node(path)->mkfifo(mode);
		} else {
			get_node(path)->mknod(mode, dev);
		}
	}

	/**
	 * Calls `open` on a @ref node, then puts the resultant @ref FileHandle
	 * in the given @ref fuse_file_info `fh` member.
	 * @param path The path string to the node to open.
	 * @param fi Provides open flags and receives the resultant handle.
	 */
	static void open_real(char const * path, struct fuse_file_info * fi) {
		FileHandle1 *fh = get_node(path)->open(fi->flags);
		//TODO fi->direct_io, fi->keep_cache, fi->nonseekable
		fi->fh = (uintptr_t) fh;
	}

	static void create_real(char const * path, mode_t mode, struct fuse_file_info * fi) {
		FileHandle1 *fh = get_node(path)->createAndOpen(mode, fi->flags);
		fi->fh = (uintptr_t) fh;
	}

	/**
	 * Deletes the @ref FileHandle in the given @ref fuse_file_info `fh` member.
	 * @param path Ignored
	 * @param fi Contains the file handle to close/release.
	 */
	static void release_real(char const * path, struct fuse_file_info *fi) {
		NodeHandle1 *fh = get_handle<NodeHandle1>(fi);
		delete fh;
		fi->fh = (uintptr_t) nullptr;
	}

	static void opendir_real(char const * path, struct fuse_file_info *fi) {
		DirHandle1 *dh = get_node(path)->opendir(fi->flags);
		fi->fh = (uintptr_t) dh;
	}

	static void readdir_real(char const * path, void * buf, fuse_fill_dir_t filler, off_t offset,
			struct fuse_file_info *fi) {
		DirHandle1* dh = get_handle<DirHandle1>(fi);
		struct stat statbuf;
		struct dirent buffer;
		for(struct dirent * entry = dh->readdir(&buffer); entry!=nullptr; entry = dh->readdir(&buffer)) {
			statbuf.st_ino = entry->d_ino;
			statbuf.st_mode = DTTOIF(entry->d_type);
			if(filler(buf, entry->d_name, &statbuf, 0) != 0) {
				throw fuse_error(ENOMEM);
			}
		}
	}

	static void fsync_real(char const * path, int datasync, struct fuse_file_info *fi) {
		get_handle<NodeHandle1>(fi)->fsync(datasync);
	}

	static void truncate_real(char const * path, off_t newLength) {
		get_node(path)->truncate(newLength);
	}

	static void ftruncate_real(char const * path, off_t newLength, struct fuse_file_info *fi) {
		get_handle<FileHandle1>(fi)->truncate(newLength);
	}

	static void read_buf_real(char const * path, struct fuse_bufvec **bufp, size_t size, off_t off,
			struct fuse_file_info * fi) {
		shared_ptr<Buffer> buf = get_handle<FileHandle1>(fi)->read(size, off);
		*bufp = extractBufvec(buf);
	}

	static int write_buf_real(char const * path, struct fuse_bufvec *buf, off_t offset, struct fuse_file_info *fi) {
		BufferImpl buffer(buf, false);
		return get_handle<FileHandle1>(fi)->write(buffer, offset);
	}

	static void getattr_real(char const * path, struct stat * statbuf) {
		get_node(path)->getattr(*statbuf);
	}

	static void readlink_real(char const * path, char * link, size_t size) {
		get_node(path)->readlink(size).copy(link, size, 0);
	}

#define FORWARD(op, type) operations->op = P_CALL_AND_CATCH(&type::op)
#define DEPRECATED(op) operations->op = NULL
#define FORWARD_WRAP(op, wrapper) operations->op = P_CALL_AND_CATCH(&with_mount1<get_mount>::wrapper)
#define FORWARD_FH(op, index) operations->op = P_FH_CALL_AND_CATCH(&FileHandle1::op, index)

	static void bind(fuse_operations *operations, ImplementedOps implementedOps) {
		FORWARD_WRAP(getattr, getattr_real);
		FORWARD_WRAP(readlink, readlink_real);
		DEPRECATED(getdir);
		FORWARD_WRAP(mknod, mknod_real);
		FORWARD(mkdir, Node1); //@snr
		FORWARD(unlink, Node1); //@snr
		FORWARD(rmdir, Node1); //@snr
		operations->symlink = call_link_and_catch_p<&Node1::symlink>;
		operations->rename = call_link_and_catch_p<&Node1::rename>;
		operations->link = call_link_and_catch_p<&Node1::link>;
		FORWARD(chmod, Node1); //@snr
		FORWARD(chown, Node1); //@snr
		FORWARD(truncate, Node1); //@snr
		DEPRECATED(utime);
		FORWARD_WRAP(open, open_real);
		DEPRECATED(read);
		DEPRECATED(write);
		FORWARD(statfs, Node1); //@snr
		FORWARD(flush, FileHandle1); //@snr
		FORWARD_WRAP(release, release_real);
		FORWARD_WRAP(fsync, fsync_real); //@snr
		if(implementedOps & xattr) {
			FORWARD(setxattr, Node1); //@snr
			FORWARD(getxattr, Node1); //@snr
			FORWARD(listxattr, Node1); //@snr
			FORWARD(removexattr, Node1); //@snr
		}
		FORWARD_WRAP(opendir, opendir_real);
		FORWARD_WRAP(readdir, readdir_real);
		FORWARD_WRAP(releasedir, release_real);
		FORWARD_WRAP(fsyncdir, fsync_real);
		FORWARD(access, Node1); //@snr
		FORWARD_WRAP(create, create_real);
		if(implementedOps & ftruncate)
			FORWARD(ftruncate, FileHandle1); //@snr
		if(implementedOps & fgetattr)
			FORWARD(fgetattr, NodeHandle1); //@snr
		FORWARD_FH(lock, 0);
		FORWARD(utimens, Node1); //@snr
		// bmap
		FORWARD_FH(ioctl, 2); //@snr
		// poll
		FORWARD_WRAP(write_buf, write_buf_real);
		FORWARD_WRAP(read_buf, read_buf_real);
		// flock
		// fallocate

		// flag_nullpath_ok
		// flag_nopath
		// flag_utime_omit_ok

		// init?
		// destroy?
	}

#undef FORWARD
#undef DEPRECATED
#undef FORWARD_WRAP
#undef FORWARD_F

}; //struct with_mount

} // namespace internal
} // namespace fusepp

#endif /* FUSEPP_INTERNAL_IMPL_HPP_ */
