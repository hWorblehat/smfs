/*
 * DirEntry.h
 *
 *  Created on: 4 Apr 2017
 *      Author: rowan
 */

#ifndef FUSEPP_DIRENTRY_H_
#define FUSEPP_DIRENTRY_H_

#include <string>
#include <cinttypes>
#include <memory>
#include <optional>

#include <uulib/Capsule.h>

#include "fusepp/internal/fusepp_forward.h"

namespace fusepp {

struct DirEntry {
	virtual ~DirEntry(){}
	virtual std::string getName() const = 0;
	virtual std::size_t getNextOffset() const = 0;
	virtual std::optional<uint64_t> getIndexNumber() const {
		return std::optional<uint64_t>;
	}
	virtual unsigned char getType() const = 0;
	virtual std::shared_ptr<Node1> lookupNode() = 0;
};

using AnyDirEntry = uulib::Capsule<DirEntry, 256>;

}

#endif /* FUSEPP_DIRENTRY_H_ */
