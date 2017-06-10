/*
 * Bufvec.h
 *
 *  Created on: 22 Feb 2017
 *      Author: rowan
 */

#ifndef FUSEPP_INTERNAL_BUFFER_H_
#define FUSEPP_INTERNAL_BUFFER_H_

#include "fusepp/internal/cfuse.h"
#include "fusepp/Buffer.h"

#include <functional>

namespace fusepp{

#include "fusepp/internal/using_std.h"

#define UNCONST(type, ret, method, spec) \
	inline ret method() spec {\
		return const_cast<ret>(static_cast<type const *>(this)->method());\
	}

template<typename Sig>
auto unconst();

template<typename T, typename R, R const (T::*F)()>
inline auto unconst<R (T::*)()>() {
	return const_cast<R>((static_cast<T const *>(this)->*F)());
}

namespace internal {

struct AbstractBuffer : virtual Buffer {
	virtual ~AbstractBuffer(){}

	size_t copyDataFrom(Buffer &other, int flags) override;
	size_t size() const override;
	size_t remaining() const override;
	void setOffset(off_t offset) override;
	void seek(off_t offset) override;

protected:
	virtual ::fuse_bufvec const & getBufvec() const;
	virtual UNCONST(AbstractBuffer, ::fuse_bufvec&, getBufvec,)
};

class DynamicBuffer : public virtual AbstractBuffer {
protected:
	unique_ptr<::fuse_bufvec> bufvec;

public:
	DynamicBuffer(std::function<void (::fuse_buf&)>&& bufSetup);
	DynamicBuffer(unique_ptr<::fuse_bufvec>&& bufvec);
	virtual ~DynamicBuffer(){}

	::fuse_bufvec* extractBufvec() const;
	::fuse_bufvec* releaseBufvec();

	::fuse_bufvec const & getBufvec() const override;
};

inline ::fuse_bufvec* extractBufvec(shared_ptr<Buffer> buffer) {
	internal::DynamicBuffer* dyn = static_cast<internal::DynamicBuffer*>(buffer.get());
	return buffer.unique() ? dyn->releaseBufvec() : dyn->extractBufvec();
}

class AbstractDataBuffer : public DataBuffer, public virtual AbstractBuffer {
protected:
	AbstractDataBuffer(){}
	virtual ~AbstractDataBuffer(){}

public:
	void const * data() const override;
	UNCONST(AbstractDataBuffer, void*, data, override);

	void const * tail() const override;
	UNCONST(AbstractDataBuffer, void*, tail, override);

	off_t position() const override;
};

struct AutomaticDataBuffer : AbstractDataBuffer {
	::fuse_bufvec bufvec;

	AutomaticDataBuffer(void* mem, size_t size);

	::fuse_bufvec const & getBufvec() const override;
};

}
}

#endif /* FUSEPP_INTERNAL_BUFFER_H_ */
