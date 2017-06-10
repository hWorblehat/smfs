/*
 * Timestamp.h
 *
 *  Created on: 4 Apr 2017
 *      Author: rowan
 */

#ifndef FUSEPP_TIMESTAMP_H_
#define FUSEPP_TIMESTAMP_H_

namespace fusepp {

struct Timestamp {
	timespec times[2];

	Timestamp(timespec times[2]) : times(times) {}

	timespec& accessTime() {
		return times[0];
	}

	timespec& modificationTime() {
		return times[1];
	}

};

}

#endif /* FUSEPP_TIMESTAMP_H_ */
