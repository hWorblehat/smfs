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

class Buffer {
protected:
	static const int defaultCopyFlags;

	Buffer();

public:

	Buffer(Buffer const &other) = delete;
	Buffer& operator=(Buffer const &other) = delete;

	virtual ~Buffer();

	size_t copyDataTo(Buffer &other, int flags = defaultCopyFlags);
	size_t copyDataTo(void* mem, size_t size, int flags = defaultCopyFlags);
	size_t copyDataTo(int fd, off_t offset = 0, size_t size = SIZE_MAX, int flags = defaultCopyFlags);
	size_t size();
	size_t remaining();
	void setOffset(off_t offset);
	void seek(off_t offset);

	class Builder {
	protected:
		Builder();

	public:
		Builder(Builder const &other) = delete;
		Builder& operator=(Builder const &other) = delete;

		virtual ~Builder();

		Builder& addComponent(void* mem, size_t length);
		Builder& addComponent(int fd, off_t offset = 0, size_t length = SIZE_MAX);

		std::shared_ptr<Buffer> build();
	};

	static std::unique_ptr<Builder> newBuilder();

	static std::shared_ptr<Buffer> create(void* mem, size_t length);
	static std::shared_ptr<Buffer> create(int fd, off_t offset = 0, size_t length = SIZE_MAX);

};

}

#endif /* FUSEPP_BUFFER_H_ */
