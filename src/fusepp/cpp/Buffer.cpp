/*
 * Buffer.cpp
 *
 *  Created on: 29 Jan 2017
 *      Author: rowan
 */

#include "fusepp/internal/Buffer.h"
#include "fusepp/common.hpp"

namespace fusepp {

inline size_t bufvec_size(size_t count) {
	return sizeof(struct ::fuse_bufvec) + (count-1)*sizeof(struct ::fuse_buf);
}

inline struct ::fuse_bufvec * bv(Buffer* buffer) {
	return static_cast<internal::BufferImpl*>(buffer)->bufvec;
}

inline void setToMem(struct fuse_buf * buf, void* mem, size_t length) {
	buf->mem = mem;
	buf->size = length;
	buf->flags = static_cast<fuse_buf_flags>(0);
}

inline void setToFD(struct fuse_buf* buf, int fd, off_t offset, size_t length) {
	buf->fd = fd;
	buf->size = length;
	buf->pos = offset;
	buf->flags = FUSE_BUF_IS_FD;
	if(offset>0) buf->flags = static_cast<fuse_buf_flags>(buf->flags | FUSE_BUF_FD_SEEK);
}

namespace internal {

struct ::fuse_bufvec* extractBufvec(std::shared_ptr<Buffer> buffer) {
	BufferImpl* bi = static_cast<BufferImpl*>(buffer.get());
	if(buffer.unique()) {
		struct ::fuse_bufvec* rc = bi->bufvec;
		bi->bufvec = nullptr;
		return rc;
	} else {
		size_t size = bufvec_size(bi->bufvec->count);
		struct ::fuse_bufvec* rc = static_cast<::fuse_bufvec*>(malloc(size));
		memcpy(rc, bi->bufvec, size);
		return rc;
	}
}

} //namespace internal

const int Buffer::defaultCopyFlags = 0;

Buffer::Buffer(){}
Buffer::~Buffer(){}

inline size_t copyTo(Buffer* thisBuf, struct fuse_bufvec* dest, int flags) {
	ssize_t val = fuse_buf_copy(dest, bv(thisBuf), static_cast<fuse_buf_copy_flags>(flags));
	if(val < 0) throw fuse_error(-val);
	return val;
}

size_t Buffer::copyDataTo(Buffer& other, int flags) {
	return copyTo(this, bv(&other), flags);
}

size_t Buffer::copyDataTo(int fd, off_t offset, size_t size, int flags) {
	struct fuse_bufvec dest = FUSE_BUFVEC_INIT(0);
	setToFD(dest.buf, fd, offset, size);
	return copyTo(this, &dest, flags);
}

size_t Buffer::copyDataTo(void* mem, size_t size, int flags) {
	struct fuse_bufvec dest = FUSE_BUFVEC_INIT(0);
	setToMem(dest.buf, mem, size);
	return copyTo(this, &dest, flags);
}

size_t Buffer::size() {
	return fuse_buf_size(bv(this));
}

size_t Buffer::remaining() {
	struct fuse_bufvec* bufvec = bv(this);
	size_t size = 0;
	for(unsigned int i=bufvec->idx; i<bufvec->count; ++i) {
		size_t s = bufvec->buf[i].size;
		if(s==SIZE_MAX) return SIZE_MAX;
		size += s;
	}
	return size - bufvec->off;
}

void setPos(struct fuse_bufvec* bufvec, size_t index, off_t offset) {
	for(;;) {
		size_t bufsize = bufvec->buf[bufvec->idx].size;
		if(offset>=bufsize) {
			++index;
			if(index==bufvec->count) {
				offset = 0;
				break;
			}
			offset -= bufsize;
		} else {
			break;
		}
	}
	bufvec->idx = index;
	bufvec->off = offset;
}

void Buffer::setOffset(off_t offset) {
	setPos(bv(this), 0, offset);
}

void Buffer::seek(off_t offset) {
	struct fuse_bufvec* bufvec = bv(this);
	setPos(bufvec, bufvec->idx, offset+bufvec->off);
}

inline struct fuse_bufvec * allocateBufvec(size_t capacity, size_t count) {
	struct fuse_bufvec * buf = static_cast<fuse_bufvec*>(malloc(bufvec_size(capacity)));
	buf->idx = 0;
	buf->off = 0;
	buf->count = count;
	return buf;
}

inline struct fuse_bufvec * allocateBufvec(size_t capacity) {
	return allocateBufvec(capacity, capacity);
}

struct BuilderImpl : Buffer::Builder {
	struct fuse_bufvec * bufvec;
	size_t capacity;

	BuilderImpl() : capacity(2), bufvec(allocateBufvec(capacity, 0)) {}

	~BuilderImpl() {
		free(bufvec);
	}

	inline struct fuse_buf* add(){
		if(bufvec->count==capacity) {
			capacity *= 2;
			bufvec = static_cast<fuse_bufvec*>(realloc(bufvec, bufvec_size(capacity)));
		}
		return bufvec->buf + (bufvec->count++);
	}

	inline struct fuse_bufvec* finalize() {
		struct fuse_bufvec* bv = capacity>bufvec->count
				? static_cast<fuse_bufvec*>(realloc(bufvec, bufvec_size(bufvec->count)))
				: bufvec;
		bufvec = nullptr;
		return bv;
	}
};

inline BuilderImpl* impl(Buffer::Builder* ptr) {
	return static_cast<BuilderImpl*>(ptr);
}

Buffer::Builder::Builder(){}
Buffer::Builder::~Builder(){}

Buffer::Builder& Buffer::Builder::addComponent(void* mem, size_t length) {
	setToMem(impl(this)->add(), mem, length);
	return *this;
}

Buffer::Builder& Buffer::Builder::addComponent(int fd, off_t offset, size_t length) {
	setToFD(impl(this)->add(), fd, offset, length);
	return *this;
}

shared_ptr<Buffer> Buffer::Builder::build(){
	return make_shared<internal::BufferImpl>(impl(this)->finalize());
}

unique_ptr<Buffer::Builder> Buffer::newBuilder() {
	return make_unique<BuilderImpl>();
}

shared_ptr<Buffer> Buffer::create(int fd, off_t offset, size_t length) {
	struct fuse_bufvec * bv = allocateBufvec(1);
	setToFD(bv->buf, fd, offset, length);
	return make_shared<internal::BufferImpl>(bv);
}

shared_ptr<Buffer> Buffer::create(void* mem, size_t length) {
	struct fuse_bufvec * bv = allocateBufvec(1);
	setToMem(bv->buf, mem, length);
	return make_shared<internal::BufferImpl>(bv);
}

}
