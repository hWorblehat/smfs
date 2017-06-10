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
#include <functional>

#include "../../../uulib/headers/uulib/Capsule.h"

extern "C" {
	#include <sys/stat.h>
}

namespace fusepp {

struct DirEntry {
	virtual ~DirEntry(){}
	virtual std::string getName() const = 0;
	virtual struct stat const & getAttr() const = 0;
};

struct DirEntryIterator {
	virtual ~DirEntryIterator(){}

	virtual DirEntry const & get() const = 0;
	virtual std::shared_ptr<DirEntryIterator> next() = 0;
};

using AnyDirEntry = uulib::Capsule<DirEntry, 256>;

}

#endif /* FUSEPP_DIRENTRY_H_ */
