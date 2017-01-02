/*
 * fusepp.h
 *
 *  Created on: 20 Jul 2016
 *      Author: rowan
 */

#ifndef FUSE_HPP_
#define FUSE_HPP_

// Std includes
#include <string>
#include <cstddef> // for size_t
#include <memory>

// POSIX includes
#include <sys/types.h> // for off_t
#include <sys/stat.h>

#ifndef HAVE_SYS_XATTR_H
#include <fusepp/common.hpp>
#endif

namespace fusepp {

/**
 * The type used to represent path arguments to the fuse operations,
 * always given relative to the mount point.
 */
typedef std::string const path_t;

/**
 * The type used to represent the names of a file's extended attributes.
 */
typedef std::string const xattr_name_t;

/**
 * Represents a handle to an open file within a fuse filesystem.
 *
 * Extending classes should ensure that any underlying file handles are closed
 * when the object is destroyed.
 */
class file_handle {
public:

	/**
	 * Destructor for file_handle.
	 *
	 * Extending classes should ensure that any underlying file handles are
	 * closed when the destructor is called.
	 *
	 * @throws fuse_error if an error occurs during closing.
	 */
	virtual ~file_handle() {}

	/**
	 * Read data from this file.
	 * @param buf The buffer in which to copy the read data. Must be at least
	 *            nbytes in size.
	 * @param nbytes The number of bytes to read.
	 * @param offset The offset within the file to read from.
	 * @return The number of bytes actually read.
	 * @throws fuse_error if an error occurs.
	 */
	virtual int read(char * buf, size_t nbytes, off_t offset) = 0;

	/**
	 * Write data to this file.
	 * @param data The data to write. Must be at least nbytes in size.
	 * @param nbytes The number of bytes to write.
	 * @param offset The offset within the file to write to.
	 * @return The number of bytes actually written.
	 * @throws fuse_error if an error occurs.
	 */
	virtual int write(char const * data, size_t nbytes, off_t offset) = 0;

	/**
	 * Possibly flush cached data
	 *
	 * BIG NOTE: This is not equivalent to fsync().  It's not a
	 * request to sync dirty data.
	 *
	 * Flush is called on each close() of a file descriptor.  So if a
	 * filesystem wants to return write errors in close() and the file
	 * has cached dirty data, this is a good place to write back data
	 * and return any errors.  Since many applications ignore close()
	 * errors this is not always useful.
	 *
	 * NOTE: The flush() method may be called more than once for each
	 * open().	This happens if more than one file descriptor refers
	 * to an opened file due to dup(), dup2() or fork() calls.	It is
	 * not possible to determine if a flush is final, so each flush
	 * should be treated equally.  Multiple write-flush sequences are
	 * relatively rare, so this shouldn't be a problem.
	 *
	 * Filesystems shouldn't assume that flush will always be called
	 * after some writes, or that if will be called at all.
	 *
	 * @throws fuse_error if an error occurs.
	 */
	virtual void flush() = 0;

	/**
	 * Synchronises the file contents with the underlying device.
	 * @param datasync true iff only the user data should be synchronised, not
	 *                 the metadata.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void fsync(bool datasync) = 0;

};

/**
 * Represents a single filesystem node (file/directory/symlink/etc) in a fuse
 * filesystem.
 */
class node {
public:

	/**
	 * The path, relative to the mount point, of this node.
	 */
	path_t rel_path;

protected:

	/**
	 * Constructor for node.
	 * @param rel_path The path, relative to the mount point, of this node.
	 */
	node(path_t &rel_path) : rel_path(rel_path) {}

public:

	/**
	 * Destructor for node.
	 */
	virtual ~node() {}

	/**
	 * @brief Get file attributes of this node.
	 *
	 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
	 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
	 * mount option is given.
	 *
	 * @param statbuf The structure to store the attributes in.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void getattr(struct stat *statbuf) = 0;

	/**
	 * @brief Determines the target of a symbolic link at this node.
	 *
	 * The given link buffer should be filled with a null terminated string. The
	 * buffer size argument includes the space for the terminating
	 * null character.	If the linkname is too long to fit in the
	 * buffer, it should be truncated.	The return value should be 0
	 * for success.
	 *
	 * @param link The buffer in which to store the link target.
	 * @param size The size of the buffer.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void readlink(char *link, size_t size) = 0;

	/**
	 * @brief Create a new file at this node.
	 *
	 * This will be called for the creation of all regular files.
	 *
	 * @param mode The mode of the file to create.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void create(mode_t mode) = 0;

	/**
	 * @brief Create a fifo (named pipe) at this node.
	 * @param mode The mode of the fifo to create.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void mkfifo(mode_t mode) = 0;

	/**
	 * @brief Create a block or character device at this node.
	 *
	 * @param mode The mode of the file create.
	 * @param dev The device type to create, if creating a device file.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void mknod(mode_t mode, dev_t dev) = 0;

	/**
	 * @brief Create a directory at this node.
	 *
	 * @param mode The mode of the directory to create. This may not have the
	 *            type specification bit set, i.e. S_ISDIR(mode) can be false.
	 *            To obtain the correct directory type bits use  mode|S_IFDIR.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void mkdir(mode_t mode) = 0;

	/**
	 * @brief Remove a file at this node.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void unlink() = 0;

	/**
	 * @brief Remove a directory at this node.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void rmdir() = 0;

	/**
	 * Create a symbolic link at this node to the given target.
	 * @param target The full path of the target the symlink should point to.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void symlink(path_t target) = 0;

	/**
	 * Rename this node.
	 * @param new_name The new name to give the node.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void rename(path_t new_name) = 0;

	/**
	 * Create a hard link at this node to the given target.
	 * @param target The full path of the target the link should point to.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void link(path_t target) = 0;

	/**
	 * Change the permission bits of this node.
	 * @param mode The mode (permission bits) to use.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void chmod(mode_t mode) = 0;

	/**
	 * Change the owner of this node.
	 * @param uid The UID of the new owner.
	 * @param gid The GID of the new owner.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void chown(uid_t uid, gid_t gid) = 0;

	/**
	 * Truncate the file at this node.
	 * @param new_length The length (in bytes) to truncate the file to.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void truncate(off_t new_length) = 0;

	/**
	 * Opens a file at this node for the operations indicated by the given
	 * flags.
	 * @param flags Open flags.
	 * @return A pointer to @file_handle object that can be used to perform
	 *         read/write operations.
	 * @throws fuse_error if an error occurs.
	 */
	virtual file_handle* open(int flags) = 0;

	/**
	 * Gets file system statistics for the underlying filesystem on which this
	 * node is stored.
	 * @param statbuf The structure to store the statistics in.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void statvfs(struct statvfs * statbuf) = 0;

#ifdef HAVE_SYS_XATTR_H
#define XATTR_FN(sig) virtual sig = 0;
#else
#define XATTR_FN(sig) sig { throw fuse_error(ENOTSUP); }
#endif

	/**
	 * @brief Set an extended attribute for this node.
	 * @param name The name of the attribute to set.
	 * @param value The value to assign to the attribute
	 * @param flags Creation flags.
	 * @throws fuse_error if an error occurs.
	 */
	XATTR_FN(void setxattr(xattr_name_t &name, char const * value, size_t size,
			int flags))

	/**
	 * @brief Get an extended attribute for this node.
	 * @param name The name of the attribute to get.
	 * @param buf A buffer in which to copy the attribute's value.
	 * @param size The size of buf. Also, zero can be given here in order to
	 *             determine the length of the attribute, so that an
	 *             appropriately-sized buffer can be allocated.
	 * @return The number of bytes in the attribute's value.
	 * @throws fuse_error if an error occurs or the size is greater than zero
	 *                    but too small.
	 */
	XATTR_FN(int getxattr(xattr_name_t &name, char * buf, size_t size))

	/**
	 * @brief List the extended attributes associated with this node.
	 *
	 * A null-separated list of extended attribute name strings will be copied
	 * into the given buffer.
	 * @param buf The buffer to copy the list into.
	 * @param size The size of the buf. Also, zero can be given here in order to
	 *             determine the length of the list, so that an
	 *             appropriately-sized buffer can be allocated.
	 * @return The number of bytes in the null-separated list.
	 * @throws fuse_error if an error occurs or the size is greater than zero
	 *                    but too small.
	 */
	XATTR_FN(int listxattr(char * buf, size_t size))

	/**
	 * @brief Removes the specified extended attribute from this node.
	 * @param name The name of the attribute to remove.
	 * @throws fuse_error if an error occurs or the size is greater than zero
	 *                    but too small.
	 */
	XATTR_FN(void removexattr(xattr_name_t &name))

#undef XATTR_FN

};

class mount {
public:

	bool const has_xattr =
#ifdef HAVE_SYS_XATTR_H
	true;
#else
	false;
#endif

	virtual ~mount() {}

	virtual std::shared_ptr<node> get_node(path_t rel_path) = 0;

};

int main(int argc, char *argv[], mount *mount);

}

#endif /* FUSE_HPP_ */
