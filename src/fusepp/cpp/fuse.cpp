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

#include "fusepp/common.hpp"
#include "fuse.hpp"

#include <functional>

namespace fuse {

typedef file_handle fh;

inline mount* get_mount() {
	return (mount *) fuse_get_context()->private_data;
}

inline path_t convert_path(char const *path) {
	return path_t(path);
}

inline xattr_name_t convert_xattr_name(char const *name) {
	return xattr_name_t(name);
}

inline node* get_node(char const *path) {
	return get_mount()->get_node(convert_path(path));
}

inline fh* get_file_handle(fuse_file_info * fi) {
	return (fh*) fi->fh;
}

template <typename... Params> struct with_sig {

	template<std::function<int(Params...)> f>
	static int fuse_op(Params... params) {
		try {
			return f(std::forward<Params>(params)...);
		} catch (fuse_error &fe) {
			return -fe.error;
		}
	}

};

inline int fusepp_op(std::function<int()> op) {
	try {
		return op();
	} catch (fuse_error &fe) {
		return -fe.error;
	}
}

template<typename Sig> struct with_args;

template<typename R, typename... Params>
struct with_args<R(Params...)> {

	template<typename obj, int (*F)(obj*, Params...)>
	inline static int any_op(obj * subject, Params&&... params) {
		try {
			return *F(subject, std::forward<Params>(params)...);
		} catch (fuse_error &fe) {
			return -fe.error;
		}
	}

	template<typename obj, void (obj::*F)(Params...)>
	inline static int any_op(obj * subject, Params&&... params) {
		int (*f)(obj*, Params...) const = [](obj* s, Params&&... p) {
			s->*F(std::forward<Params>(p)...);
			return 0;
		};
		return any_op<obj, f>(subject, std::forward<Params>(params)...);
	}

	template<typename obj, int (obj::*F)(Params...)>
	inline static int any_op(obj * subject, Params&&... params) {
		int (*f)(obj*, Params...) const = [](obj* s, Params&&... p) {
			return s->*F(std::forward<Params>(p)...);
		};
		return any_op<obj, f>(subject, std::forward<Params>(params)...);
	}

	template<R (node::*F)(Params...)>
	static int node_op(char const * path, Params&... params) {
		return any_op<node,F>(get_node(path), std::forward<Params>(params)...);
	}

	template<R (node::*F)(xattr_name_t, Params...)>
	static int xattr_op(char const *path, char const *name, Params&... params) {
		return any_op<node,F>(get_node(path),
				convert_xattr_name(name), std::forward<Params>(params)...);
	}

	template<R (fh::*F)(Params...)>
	static int fh_op(char const * path, Params&... params, fuse_file_info * fi){
		return any_op<fh,F>(get_file_handle(fi),
				std::forward<Params>(params)...);
	}
};

int mknod_real(char const * path, mode_t mode, dev_t dev) {
	return S_ISFIFO(mode)
			? with_args<mode_t>::node_op<&node::mkfifo>(path, mode)
			: with_args<mode_t, dev_t>::node_op<&node::mknod>(path, mode, dev);
}

template<void (node::*F)(path_t)>
int link_op(char const * target, char const * link) {
	return with_args<path_t>::node_op<F>(link, convert_path(target));
}

int rename_real(char const * old_path, char const * new_path) {
	node* pnode = get_node(old_path);
	int rc = link_op<&node::rename>(new_path, old_path);
	if(rc==0) { //TODO
		delete pnode;
	}
	return rc;
}

int open_real(char const * path, struct fuse_file_info * fi) {
	try {
		fh *fh = get_node(path)->open(fi->flags);
		fi->fh = (uintptr_t) fh;
		return 0;
	} catch (fuse_error &oe) {
		return -oe.error;
	}
}

template<typename buff_t, int (fh::*F)(buff_t *, size_t, off_t)>
int read_write(char const * path, buff_t *buff, size_t nbytes, off_t offset) {
	try {
		return get_node(path)->*F(buff, nbytes, offset);
	} catch (fuse_error &fe) {
		return -fe.error;
	}
}

int release_file(char const * path, struct fuse_file_info *fi) {
	try {
		delete get_file_handle(fi);
		fi->fh = NULL;
		return 0;
	} catch (fuse_error &oe) {
		return -oe.error;
	}
}

int fsync_real(char const * path, int datasync, struct fuse_file_info *fi) {
	bool b = datasync!=0;
	return with_args<bool>::fh_op<&fh::fsync>(path, b, fi);
}

int main(int argc, char *argv[], mount *mount) {

	struct fuse_operations operations = {};

#define SET_OP(type, op, ...) \
	operations.op = with_args<__VA_ARGS__>::type##_op<&type::op>

#define DEPRECATED(op) operations.op = NULL

	SET_OP(node, getattr, struct stat *);
	DEPRECATED(getdir);
	SET_OP(node, readlink, char *, size_t);
	operations.mknod = mknod_real;
	SET_OP(node, mkdir, mode_t);
	SET_OP(node, create, mode_t);
	SET_OP(node, unlink);
	SET_OP(node, rmdir);
	operations.symlink = link_op<&node::symlink>;
	operations.rename = rename_real;
	operations.link = link_op<&node::link>;
	SET_OP(node, chmod, mode_t);
	SET_OP(node, chown, uid_t, gid_t);
	SET_OP(node, truncate, off_t);
	DEPRECATED(utime);
	operations.read = read_write<char, &fh::read>;
	operations.write = read_write<char const, &fh::write>;
	operations.statfs = with_args<struct statvfs *>::node_op<&node::statvfs>;
	SET_OP(fh, flush);
	operations.release = release_file;
	operations.fsync = fsync_real;

	if(mount->has_xattr) {

	}

#undef SET_OP
#undef DEPRECATED

	return fuse_main(argc, argv, &operations, mount);
}

} // namespace fuse
