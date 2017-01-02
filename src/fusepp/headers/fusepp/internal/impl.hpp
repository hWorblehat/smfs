/*
 * impl.hpp
 *
 *  Created on: 19 Nov 2016
 *      Author: rowan
 */

#ifndef FUSEPP_INTERNAL_IMPL_HPP_
#define FUSEPP_INTERNAL_IMPL_HPP_

#include <fuse.h>
#include "fuse.hpp"

#include <utility>
#include <memory>

#include <iostream>

using std::forward;

namespace fusepp {
namespace internal {

typedef mount * (*getMount)();

/**
 * Converts the given path string to a @ref path_t.
 * @param path The path string to convert.
 * @return The resultant @ref path_t.
 */
inline path_t convert_path(char const *path) {
	std::cerr << "Converting path...";
	return path_t(path);
}

/**
 * Converts the given extended attribute name string to a @ref xattr_name_t
 * @param name The attribute name string to convert.
 * @return The resultant @ref xattr_name_t.
 */
inline xattr_name_t convert_xattr_name(char const *name) {
	return xattr_name_t(name);
}

template<getMount get_mount>
struct with_mount {

	/**
	 * Gets a pointer to the @ref node under the current fuse context's @ref mount
	 * at the given path.
	 * @param path The path of the node to get.
	 * @return The requested node.
	 */
	static inline std::shared_ptr<node> get_node(char const *path) {
		std::cerr << "Getting node...";
		return get_mount()->get_node(convert_path(path));
	}

	/**
	 * Gets a pointer to the @ref file_handle object associated with the specified
	 * @ref fuse_file_info.
	 * @param fi The fuse_file_info to get the associated file_handle of.
	 * @return The associated file_handle.
	 */
	static inline file_handle* get_file_handle(fuse_file_info * fi) {
		return (file_handle*) fi->fh;
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
				std::cerr << "Hi1";
				int rc = (*F)(forward<Args>(args)...);
				std::cerr << "Hi2";
				return rc;
			} catch (fuse_error &fe) {
				std::cerr << "Hi3";
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

	template<typename Ret, typename... Args, Ret (node::*F)(Args...)>
	struct on_node<Ret (node::*)(Args...), F> {
		static Ret call(char const * path, Args... args) {
			return std::bind(F, get_node(path), forward<Args>(args)...)();
		}
	};

	template<typename Ret, typename... Args, Ret (node::*F)(Args...)>
	class call_and_catch<Ret (node::*)(Args...), F> {
		using onNode = on_node<Ret (node::*)(Args...), F>;

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
			template<typename Ret, Ret (file_handle::*F)(Args1..., Args2...)>
			struct method {
				using pOp = pOperation<char const *, Args1..., fuse_file_info*, Args2...>;

				static Ret call(char const *, Args1... args1, fuse_file_info* info, Args2... args2) {
					return (get_file_handle(info)->*F)(forward<Args1>(args1)..., forward<Args2>(args2)...);
				}
			};
		};

	};

	template<typename Sig, Sig F> struct count_args;
	template<typename Ret, typename... Args, Ret (file_handle::*F)(Args...)>
	struct count_args<Ret (file_handle::*)(Args...), F> {
		static constexpr std::size_t num = sizeof...(Args);
	};

	template<typename Sig, Sig F, std::size_t I=count_args<Sig, F>::num> struct on_file_handle;

	template<std::size_t I, typename Ret, typename... Args, Ret (file_handle::*F)(Args...)>
	struct on_file_handle<Ret (file_handle::*)(Args...), F, I>
	: public first_args<I>::template second_args<Args...>::template method<Ret, F>{};

	template<typename Ret, typename... Args, Ret (file_handle::*F)(Args...)>
	class call_and_catch<Ret (file_handle::*)(Args...), F> {
		using onFH = on_file_handle<Ret (file_handle::*)(Args...), F>;

	public:
		static constexpr typename onFH::pOp ptr = P_CALL_AND_CATCH(&onFH::call);
	};

#define P_FH_CALL_AND_CATCH(fpointer, index) P_CALL_AND_CATCH((&on_file_handle<decltype(fpointer), fpointer, index>::call))

	//template <typename Sig, Sig F, int I> struct on_file_handle;
	//
	//template<typename Ret, typename... Args, Ret (file_handle::*F)(Args...), int I>
	//struct on_file_handle<Ret (file_handle::*)(Args...), F, I> {
	//	static Ret call(char const *, Args... args, fuse_file_info* info) {
	//		return (get_file_handle(info)->*F)(std::forward<Args>(args));
	//	}
	//};

	/**
	 * Extended attribute operations
	 */
	//template <typename... Params> class xattr_ops {
	//
	//	typedef with_sig<char const *, char const *, Params...> ops;
	//
	//	template<typename Ret, Ret (node::*F)(xattr_name_t, Params...)>
	//	static Ret call(char const *path, char const *name, Params... params) {
	//		return get_node(path)->*F(convert_xattr_name(name), std::forward<Params>(params)...);
	//	}
	//
	//public:
	//
	//	typedef ops::operation operation;
	//
	//	template<void (node::*F)(xattr_name_t, Params...)>
	//	static inline operation get() {
	//		return &ops::call_and_catch<call<F>>;
	//	}
	//};

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
	 * A wrapper round any any operation that 'links' two file paths
	 * (e.g. rename, symlink) that converts both the source and destination path strings
	 * to @ref path_t objects.
	 */
	template<void (node::*F)(path_t)>
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
	template<void (node::*F)(path_t)> struct call_link_and_catch :
			public call_and_catch<void (*)(char const *, char const *), link_op<F>::wrapper> {};

	/**
	 * An alias for `call_link_and_catch<F>::ptr`.
	 * @tparam F The member function of a @ref node to wrap.
	 * @see call_link_and_catch
	 */
	template<void (node::*F)(path_t)>
	static constexpr pOperation<char const *, char const *> call_link_and_catch_p =
			call_link_and_catch<F>::ptr;

	/**
	 * Calls `open` on a @ref node, then puts the resultant @ref file_handle
	 * in the given @ref fuse_file_info `fh` member.
	 * @param path The path string to the node to open.
	 * @param fi Provides open flags and receives the resultant handle.
	 */
	static void open_real(char const * path, struct fuse_file_info * fi) {
		file_handle *fh = get_node(path)->open(fi->flags);
		fi->fh = (uintptr_t) fh;
	}

	//int release_file(char const * path, struct fuse_file_info *fi) {
	//	try {
	//		delete get_file_handle(fi);
	//		fi->fh = NULL;
	//		return 0;
	//	} catch (fuse_error &oe) {
	//		return -oe.error;
	//	}
	//}
	//
	//int fsync_real(char const * path, int datasync, struct fuse_file_info *fi) {
	//	bool b = datasync!=0;
	//	return with_args<bool>::fh_op<&fh::fsync>(path, b, fi);
	//}

#define FORWARD(op, type) operations->op = P_CALL_AND_CATCH(&type::op)
#define DEPRECATED(op) operations->op = NULL

	static void bind(fuse_operations *operations) {
		FORWARD(getattr, node); //@snr
		FORWARD(readlink, node); //@snr
		DEPRECATED(getdir);
		operations->mknod = P_CALL_AND_CATCH(&with_mount<get_mount>::mknod_real);
		FORWARD(mkdir, node); //@snr
		FORWARD(unlink, node); //@snr
		FORWARD(rmdir, node); //@snr
		operations->symlink = call_link_and_catch_p<&node::symlink>;
		operations->rename = call_link_and_catch_p<&node::rename>;
		operations->link = call_link_and_catch_p<&node::link>;
		FORWARD(chmod, node); //@snr
		FORWARD(chown, node); //@snr
		FORWARD(truncate, node); //@snr
		DEPRECATED(utime);
		//operations->open = P_CALL_AND_CATCH(&with_mount<get_mount>::open_real);
		//operations->read = P_CALL_AND_CATCH(&file_handle::read); //@snr
		//	operations.write = read_write<char const, &fh::write>;
		//	operations.statfs = with_args<struct statvfs *>::node_op<&node::statvfs>;
		//	SET_OP(fh, flush);
		//	operations.release = release_file;
		//	operations.fsync = fsync_real;
		// setxattr
		// getxattr
		// listxattr
		// removexattr
		// opendir
		// readdir
		// releasedir
		// fsyncdir
		// access
		// create
		// ftruncate
		// fgetattr
		// lock
		// utimens
		// bmap

		// flag_nullpath_ok
		// flag_nopath
		// flag_utime_omit_ok

		// ioctl
		// poll
		// write_buf
		// read_buf
		// flock
		// fallocate

		//	if(mount->has_xattr) {
		//
		//	}


		// init?
		// destroy?
	}

#undef FORWARD
#undef DEPRECATED

}; //struct with_mount

} // namespace internal
} // namespace fusepp

#endif /* FUSEPP_INTERNAL_IMPL_HPP_ */
