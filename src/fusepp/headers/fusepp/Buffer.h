/*
 * Buffer.h
 *
 *  Created on: 29 Jan 2017
 *      Author: rowan
 */

#ifndef FUSEPP_BUFFER_H_
#define FUSEPP_BUFFER_H_

#include <memory>

namespace fusepp {

class DataBuffer;
class FileBuffer;

/**
 * Represents a container of binary data.
 *
 * Each buffer includes:
 *     * Information describing how to access the container
 *     (e.g. its location and size). This is constant for each instance.
 *     * A cursor describing a position or index within the container.
 *     This is mutable for non-const instances.
 */
class Buffer {
	friend class DataBuffer;
	friend class FileBuffer;

	Buffer();

public:
	static const int defaultCopyFlags;

	Buffer(Buffer const &other) = delete;
	Buffer& operator=(Buffer const &other) = delete;

	virtual ~Buffer(){}

	virtual size_t copyDataFrom(Buffer &other, int flags = defaultCopyFlags) = 0;

	inline size_t copyDataFrom(std::shared_ptr<Buffer> other, int flags = defaultCopyFlags) {
		return copyDataFrom(*other, flags);
	}

	virtual size_t size() const = 0;
	virtual size_t remaining() const = 0;
	virtual void setOffset(off_t offset) = 0;
	virtual void seek(off_t offset) = 0;
};

/**
 * A \ref Buffer whose container is a single contiguous region of binary data in memory.
 */
class DataBuffer : public virtual Buffer {
public:
	DataBuffer();

	/**
	 * Obtains a pointer to the beginning of buffer's memory region.
	 */
	virtual void * data() = 0;

	/**
	 * Obtains a pointer to the beginning of buffer's memory region.
	 */
	virtual void const * data() const = 0;

	/**
	 * Obtains a pointer to the current location in memory
	 * represented by this buffer's cursor.
	 */
	virtual void * tail() = 0;

	/**
	 * Obtains a pointer to the current location in memory
	 * represented by this buffer's cursor.
	 */
	virtual void const * tail() const = 0;

	/**
	 * @return The position of the cursor within the data buffer.
	 */
	virtual off_t position() const = 0;

	/**
	 * Creates a new DataBuffer that wraps the specified region of memory.
	 * @param mem A pointer to the beginning of the memory region.
	 * @param length The number of bytes in the memory region.
	 * @return A shared pointer to the newly-created DataBuffer.
	 */
	static std::shared_ptr<DataBuffer> create(void* mem, size_t length);
};

/**
 * A \ref Buffer whose container is a file or portion of a file on disk.
 */
class FileBuffer : public virtual Buffer {
	FileBuffer();

public:

	virtual int getFD() const = 0;

	static std::shared_ptr<Buffer> create(int fd, off_t offset = 0, size_t length = SIZE_MAX);
};

/**
 * A \ref Buffer whose container is made up of a series of sub-containers.
 * Each sub-container can be a region of memory or a file or portion of a file.
 */
class CompoundBufferBuilder final {
	struct Data;
	std::unique_ptr<Data> const data;

public:
	CompoundBufferBuilder();

	CompoundBufferBuilder(CompoundBufferBuilder const &other) = delete;
	CompoundBufferBuilder& operator=(CompoundBufferBuilder const &other) = delete;

	virtual ~CompoundBufferBuilder();

	CompoundBufferBuilder& add(void* mem, size_t length);
	CompoundBufferBuilder& add(int fd, off_t offset = 0, size_t length = SIZE_MAX);
	CompoundBufferBuilder& add(Buffer const & buf);

	std::shared_ptr<Buffer> build();
};

}

#endif /* FUSEPP_BUFFER_H_ */
