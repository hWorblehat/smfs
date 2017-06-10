/*
 * Buffer.cpp
 *
 *  Created on: 29 Jan 2017
 *      Author: rowan
 */

#include "fusepp/internal/Buffer.h"
#include "fusepp/common.hpp"

#include <algorithm>

namespace fusepp {

const int Buffer::defaultCopyFlags = 0;

inline size_t bufvec_size(size_t count) {
	return sizeof(struct ::fuse_bufvec) + (count-1)*sizeof(struct ::fuse_buf);
}

inline void setToMem(::fuse_buf& buf, void* mem, size_t length) {
	buf.mem = mem;
	buf.fd = -1;
	buf.size = length;
	buf.pos = 0;
	buf.flags = static_cast<fuse_buf_flags>(0);
}

inline void setToFD(::fuse_buf& buf, int fd, off_t offset, size_t length) {
	buf.mem = nullptr;
	buf.fd = fd;
	buf.size = length;
	buf.pos = offset;
	buf.flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
}

inline void initBufvec(::fuse_bufvec& bufvec, size_t count) {
	bufvec.idx = 0;
	bufvec.off = 0;
	bufvec.count = count;
}

namespace internal {

inline AbstractBuffer& internals(Buffer& buf) {
	return static_cast<AbstractBuffer&>(buf);
}

/*
 * ==================================================
 * BufInternals
 * ==================================================
 */

size_t AbstractBuffer::copyDataFrom(Buffer &other, int flags) {
	ssize_t rc = ::fuse_buf_copy(&getBufvec(), &internals(other).getBufvec(),
			static_cast<::fuse_buf_copy_flags>(flags));
	if(rc<0) {
		throw fuse_error(-rc);
	}
	return rc;
}

size_t AbstractBuffer::size() const {
	return ::fuse_buf_size(&getBufvec());
}

size_t AbstractBuffer::remaining() const {
	::fuse_bufvec const & bufvec = getBufvec();
	size_t size = 0;
	for(unsigned int i = bufvec.idx; i < bufvec.count; ++i) {
		size_t s = bufvec.buf[i].size;
		if(s==SIZE_MAX){
			return SIZE_MAX;
		}
		size += s;
	}
	return size - bufvec.off;
}

void setPos(fuse_bufvec& bufvec, size_t index, off_t offset) {
	for(;;) {
		size_t bufsize = bufvec.buf[bufvec.idx].size;
		if(offset>=bufsize) {
			++index;
			if(index==bufvec.count) {
				offset = 0;
				break;
			}
			offset -= bufsize;
		} else {
			break;
		}
	}
	bufvec.idx = index;
	bufvec.off = offset;
}

void AbstractBuffer::setOffset(off_t offset) {
	setPos(getBufvec(), 0, offset);
}

void AbstractBuffer::seek(off_t offset) {
	fuse_bufvec& bufvec = getBufvec();
	setPos(bufvec, bufvec.idx, offset+bufvec.off);
}

/*
 * ===============================================
 * END BufInternals
 * ===============================================
 */

/*
 * ===============================================
 * DynamicBuffer
 * ===============================================
 */

DynamicBuffer::DynamicBuffer(std::function<void (::fuse_buf&)>&& bufSetup)
		: bufvec(make_unique<::fuse_bufvec>()) {
	initBufvec(*bufvec, 1);
	bufSetup(bufvec->buf[0]);
}

DynamicBuffer::DynamicBuffer(unique_ptr<::fuse_bufvec>&& bufvec)
		: bufvec(forward<unique_ptr<::fuse_bufvec>>(bufvec)) {}

::fuse_bufvec const & DynamicBuffer::getBufvec() const {
	return *bufvec;
}

::fuse_bufvec * DynamicBuffer::extractBufvec() const {
	::fuse_bufvec& bufvec = getBufvec();
	size_t size = bufvec_size(bufvec.count);

	::fuse_bufvec * rc = malloc(size);
	memcpy(rc, &bufvec, size);
	return rc;
}

::fuse_bufvec* DynamicBuffer::releaseBufvec() {
	return bufvec.release();
}

/*
 * ===============================================
 * END DynamicBuffer
 * ===============================================
 */

/*
 * ======================================================
 * AbstractDataBuffer
 * ======================================================
 */

inline void const * AbstractDataBuffer::data() const {
	return getBufvec().buf[0].mem;
}

inline void const * AbstractDataBuffer::tail() const {
	::fuse_bufvec const & bufvec = getBufvec();
	return bufvec.buf[0].mem + bufvec.off;
}

off_t AbstractDataBuffer::position() const {
	return getBufvec().off;
}

/*
 * ======================================================
 * END AbstractDataBuffer
 * ======================================================
 */

/*
 * ======================================================
 * AutomaticDataBuffer
 * ======================================================
 */

AutomaticDataBuffer::AutomaticDataBuffer(void* mem, size_t size)
		: bufvec(FUSE_BUFVEC_INIT(size)) {
	bufvec.buf[0].mem = mem;
}

::fuse_bufvec const & AutomaticDataBuffer::getBufvec() const {
	return bufvec;
}

/*
 * ======================================================
 * END AutomaticDataBuffer
 * ======================================================
 */

/*
 * ======================================================
 * DynamicDataBuffer
 * ======================================================
 */

struct DynamicDataBuffer : DynamicBuffer, virtual AbstractDataBuffer {

	DynamicDataBuffer(void* mem, size_t size)
			: DynamicBuffer([=](::fuse_buf& buf) {setToMem(buf, mem, size);}) {}

};

/*
 * ======================================================
 * END DynamicDataBuffer
 * ======================================================
 */

/*
 * ======================================================
 * DynamicFileBuffer
 * ======================================================
 */

struct DynamicFileBuffer : DynamicBuffer, FileBuffer {

	DynamicFileBuffer(int fd, off_t offset, size_t size)
			: DynamicBuffer([=](::fuse_buf& buf){setToFD(buf, fd, offset, size);}) {}

	int getFD() const override {
		return getBufvec().buf[0].fd;
	}

};

/*
 * ======================================================
 * END DynamicFileBuffer
 * ======================================================
 */

}

/*
 * ======================================================
 * CompoundBufferBuilder
 * ======================================================
 */

inline ::fuse_bufvec * allocateBufvec(size_t capacity, size_t count) {
	::fuse_bufvec * buf = static_cast<::fuse_bufvec*>(malloc(bufvec_size(capacity)));
	initBufvec(*buf, count);
	return buf;
}

inline ::fuse_bufvec * allocateBufvec(size_t capacity) {
	return allocateBufvec(capacity, capacity);
}

struct CompoundBufferBuilder::Data {
	size_t capacity;
	::fuse_bufvec* bufvec;

	Data() : capacity(2), bufvec(allocateBufvec(capacity, 0)) {}

	inline ::fuse_buf* add(size_t num){
		if(bufvec->count > capacity-num) {
			capacity = std::max(capacity*2, bufvec->count+num);
			bufvec = static_cast<fuse_bufvec*>(realloc(bufvec, bufvec_size(capacity)));
		}
		::fuse_buf* rc = bufvec->buf + bufvec->count;
		bufvec->count += num;
		return rc;
	}

	inline struct fuse_bufvec* release() {
		struct fuse_bufvec* bv = capacity > bufvec->count
				? static_cast<fuse_bufvec*>(realloc(bufvec, bufvec_size(bufvec->count)))
				: bufvec;
		bufvec = nullptr;
		return bv;
	}

};

CompoundBufferBuilder::CompoundBufferBuilder()
		: data(make_unique<CompoundBufferBuilder::Data>()) {}

CompoundBufferBuilder& CompoundBufferBuilder::add(void* mem, size_t length) {
	setToMem(*data->add(1), mem, length);
	return *this;
}

CompoundBufferBuilder& CompoundBufferBuilder::add(int fd, off_t offset, size_t length) {
	setToFD(*data->add(1), fd, offset, length);
	return *this;
}

CompoundBufferBuilder& CompoundBufferBuilder::add(Buffer const & buf) {
	::fuse_bufvec const & src = static_cast<internal::AbstractBuffer const &>(buf).getBufvec();
	std::memcpy(data->add(src.count), src.buf, src.count * sizeof(::fuse_buf));
	return *this;
}

std::shared_ptr<Buffer> CompoundBufferBuilder::build() {
	return make_shared<internal::DynamicBuffer>(unique_ptr<::fuse_bufvec>(data->release()));
}

/*
 * ======================================================
 * END CompoundBufferBuilder
 * ======================================================
 */

static std::shared_ptr<DataBuffer> DataBuffer::create(void* mem, size_t length) {
	return make_shared<internal::DynamicDataBuffer>(mem, length);
}

static std::shared_ptr<Buffer> FileBuffer::create(int fd, off_t offset = 0, size_t length ) {
	return make_shared<internal::DynamicFileBuffer>(fd, offset, length);
}

}


