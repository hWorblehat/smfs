/*
 * DestructorTraced.h
 *
 *  Created on: 14 Mar 2016
 *      Author: rowan
 */

#ifndef DESTRUCTORTRACED_H_
#define DESTRUCTORTRACED_H_

#include "gmock/gmock.h"

class DestructorTraced {
public:

	MOCK_METHOD0(Die, void());

	virtual ~DestructorTraced() {
		Die();
	}

};

#endif /* DESTRUCTORTRACED_H_ */
