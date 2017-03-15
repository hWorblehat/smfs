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

namespace fusepp{

#include "fusepp/internal/using_std.h"

namespace internal {

struct ::fuse_bufvec* extractBufvec(shared_ptr<Buffer> buffer);

struct BufferImpl : Buffer {
	struct ::fuse_bufvec * bufvec;
	bool const del;

	BufferImpl(struct ::fuse_bufvec * bufvec, bool del = true) : Buffer(), bufvec(bufvec), del(del) {}

	~BufferImpl() {
		if(del) free(bufvec);
	}
};

}
}

#endif /* FUSEPP_INTERNAL_BUFFER_H_ */
