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
#include <optional>

extern "C" {
	// POSIX includes
	#include <sys/types.h> // for off_t
	#include <sys/stat.h>
	#include <sys/statvfs.h>
	#include <fcntl.h>
	#include <dirent.h>
}

#include "fusepp/common.hpp"
#include "fusepp/Buffer.h"
#include "fusepp/DirEntry.h"
#include "fusepp/Timestamp.h"

namespace fusepp {

struct Ino {
	uint64_t ino;
	uint64_t generation;
};

#define NOTIMP { throw fuse_error(ENOTSUP); }

/**
 * The type used to represent path arguments to the fuse operations,
 * always given relative to the mount point.
 */
using path_t = std::string;

struct NodeHandle1 {

	/**
	 * Destructor for NodeHandle.
	 *
	 * Extending classes should ensure that any underlying resource handles
	 * are closed (released) when the destructor is called.
	 */
	virtual ~NodeHandle1() {}

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
	virtual void getattr(struct stat& statbuf) = 0;

	/**
	 * Synchronises the file contents with the underlying device.
	 * @param datasync true iff only the user data should be synchronised, not
	 *                 the metadata.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void fsync(bool datasync);

	/**
	 * @brief Performs an `ioctl` operation.
	 *
	 * @param cmd The ioctl command.
	 * @param arg The literal (verbatim) value of the final ioctl argument.
	 * @param flags Fuse flags for ioctl
	 * @param data A (copy of the) data buffer containing input/output data
	 *             pointed to by the third argument passed to ioctl, if it
	 *             was a pointer.
	 * @throws fuse_error if an error occurs
	 */
	virtual void ioctl(int cmd, void * arg, unsigned int flags, void * data);

};

/**
 * Represents a handle to an open file within a fuse filesystem.
 *
 * Extending classes should ensure that any underlying file handles are closed
 * when the object is destroyed.
 */
struct FileHandle1 : NodeHandle1 {

	/**
	 * Destructor for FileHandle.
	 *
	 * Extending classes should ensure that any underlying file handles are
	 * closed (released) when the destructor is called.
	 */
	virtual ~FileHandle1() noexcept {}

	/**
	 * Read data from this file. The returned @ref Buffer should contain the data to read,
	 * with its position set to the index of the first byte to read. The caller will not
	 * advance the position of the buffer by the number of bytes read.
	 * @param nbytes The number of bytes requested.
	 * @param offset The offset within the file to read from.
	 * @return A shared pointer to a @ref Buffer containing the read data.
	 * @throws fuse_error if an error occurs.
	 */
	virtual std::shared_ptr<Buffer> read(size_t nbytes, off_t offset);

	/**
	 * Write data to this file.
	 * @param buffer A @ref Buffer containing the data to write.
	 * @param offset The offset within the file to write to.
	 * @return The number of bytes actually written.
	 * @throws fuse_error if an error occurs.
	 */
	virtual size_t write(Buffer& buffer, off_t offset);

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
	virtual void flush();

	virtual void lock(int cmd, struct flock& flock);

	virtual void truncate(off_t newLength) = 0;

};

struct DirHandle1 : NodeHandle1 {

	virtual ~DirHandle1() {}

	virtual AnyDirEntry readdir();

	virtual void seekdir(std::size_t offset);

	virtual std::size_t telldir();
};

/**
 * Represents a single filesystem node (file/directory/symlink/etc) in a fuse
 * filesystem.
 */
class Node1 {
public:

	/**
	 * The path, relative to the mount point, of this node.
	 */
	path_t const rel_path;

protected:

	/**
	 * Constructor for node.
	 * @param rel_path The path, relative to the mount point, of this node.
	 */
	Node1(path_t &rel_path) : rel_path(rel_path) {}

public:

	/**
	 * Destructor for node.
	 */
	virtual ~Node1() {}

	virtual std::tuple<std::shared_ptr<Node1>, double> lookup(std::string name);

	virtual std::optional<Ino> ino();

	//TODO doc
	/**
	 * @brief Get file attributes of this node.
	 *
	 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
	 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
	 * mount option is given.
	 *
	 * @param statbuf The structure to store the attributes in.
	 * @return The timeout, in seconds, after which the attributes should be
	 *         considered stale.
	 * @throws fuse_error if an error occurs.
	 */
	virtual double getattr(stat& statbuf);

	/**
	 * @brief Determines the target of a symbolic link at this node.
	 *
	 * @param size The maximum number of characters that will be conveyed
	 *             to the caller. If a longer path than this is returned,
	 *             it will be truncated.
	 * @return The target of the symbolic link at this node.
	 * @throws fuse_error if an error occurs.
	 */
	virtual path_t readlink(size_t size);

	/**
	 * @brief Create a fifo (named pipe) at this node.
	 * @param mode The mode of the fifo to create.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void mkfifo(mode_t mode);

	/**
	 * @brief Create a block or character device at this node.
	 *
	 * @param mode The mode of the file create.
	 * @param dev The device type to create, if creating a device file.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void mknod(mode_t mode, dev_t dev);

	/**
	 * @brief Create a directory at this node.
	 *
	 * @param mode The mode of the directory to create. This may not have the
	 *            type specification bit set, i.e. S_ISDIR(mode) can be false.
	 *            To obtain the correct directory type bits use  mode|S_IFDIR.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void mkdir(mode_t mode);

	/**
	 * @brief Remove a file at this node.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void unlink();

	/**
	 * @brief Remove a directory at this node.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void rmdir();

	/**
	 * Create a symbolic link at this node to the given target.
	 * @param target The full path of the target the symlink should point to.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void symlink(path_t target);

	/**
	 * Rename this node.
	 * @param new_name The new name to give the node.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void rename(path_t new_name);

	/**
	 * Create a hard link at this node to the given target.
	 * @param target The full path of the target the link should point to.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void link(path_t target);

	/**
	 * Change the permission bits of this node.
	 * @param mode The mode (permission bits) to use.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void chmod(mode_t mode);

	/**
	 * Change the owner of this node.
	 * @param uid The UID of the new owner.
	 * @param gid The GID of the new owner.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void chown(uid_t uid, gid_t gid);

	/**
	 * Opens a file at this node for the operations indicated by the given
	 * flags.
	 * @param flags Open flags.
	 * @return A pointer to a @ref FileHandle object that can be used to perform
	 *         read/write operations.
	 * @throws fuse_error if an error occurs.
	 */
	virtual std::unique_ptr<FileHandle1> open(int flags);


	virtual std::unique_ptr<FileHandle1> createAndOpen(mode_t mode, int flags);

	/**
	 * Gets file system statistics for the underlying filesystem on which this
	 * node is stored.
	 * @param statbuf The structure to store the statistics in.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void statfs(struct statvfs& statbuf);

	/**
	 * @brief Set an extended attribute for this node.
	 * @param name The name of the attribute to set.
	 * @param value The value to assign to the attribute
	 * @param flags Creation flags.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void setxattr(std::string const name, DataBuffer const & value, int flags);

	/**
	 * @brief Get the size of an extended attribute's value.
	 * @param name The name of the attribute to get the size of.
	 * @return The number of bytes required to store the attribute's value.
	 */
	virtual size_t xattrSize(std::string const name);

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
	virtual size_t getxattr(std::string const name, DataBuffer& buffer);

	virtual size_t xattrListSize();

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
	virtual size_t listxattr(DataBuffer& buffer);

	/**
	 * @brief Removes the specified extended attribute from this node.
	 * @param name The name of the attribute to remove.
	 * @throws fuse_error if an error occurs or the size is greater than zero
	 *                    but too small.
	 */
	virtual void removexattr(std::string const name);

	virtual void truncate(off_t newLength);

	/**
	 * @brief Opens an existing directory at this node.
	 *
	 * @param flags The file-open flags to use.
	 * @return A pointer to a @ref DirHandle object that can be used to read the
	 *         directory's contents.
	 * @throws fuse_error if an error occurs
	 */
	virtual std::unique_ptr<DirHandle1> opendir(int flags);

	/**
	 * Tests if this node can be accessed in the given mode by the caller.
	 *
	 * @param mode The mode with which access is required.
	 * @throws fuse_error if an error occurs or if access would not be allowed.
	 */
	virtual void access(int mode);

	/**
	 * @brief Changes the node's access and modification times with nanosecond precision.
	 *
	 * @param tv An array of 2 `timespec` objects specifying first the last-accessed time,
	 *           and second the last-modified time.
	 * @throws fuse_error if an error occurs.
	 */
	virtual void utime(Timestamp& timestamp);

};

struct Mount1 {

	virtual ~Mount1() {}

	virtual std::shared_ptr<Node1> get_node(path_t rel_path) = 0;

};

int main(int argc, char *argv[], Mount1 *mount);


using Mount = Mount1;
using NodeHandle = NodeHandle1;
using FileHandle = FileHandle1;
using DirHandle = DirHandle1;
using Node = Node1;


}

#endif /* FUSE_HPP_ */
