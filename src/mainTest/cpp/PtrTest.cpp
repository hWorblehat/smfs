/*
 * PtrTest.cpp
 *
 *  Created on: 12 Mar 2016
 *      Author: rowan
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <string>
#include <memory>

#include "smfs/ptr.h"
#include "DestructorTraced.h"

using namespace std;

TEST(any_ptr, can_be_constructed_from_shared_ptr) {
	shared_ptr<string> sp(new string("hello"));
	smfs::any_ptr<string> ap(sp);
	EXPECT_EQ(*ap, string("hello"));
	EXPECT_EQ(ap->length(), 5);
	EXPECT_TRUE(ap);
}

TEST(any_ptr, deletion_when_holding_shared_ptr_decrements_ref_count) {
	shared_ptr<string> sp(new string("hello"));
	EXPECT_EQ(*sp, string("hello"));
	EXPECT_EQ(sp.use_count(), 1);

	smfs::any_ptr<string>* pap = new smfs::any_ptr<string>(sp);
	EXPECT_EQ(**pap, string("hello"));
	EXPECT_EQ(sp.use_count(), 2);

	delete(pap);
	EXPECT_EQ(*sp, string("hello"));
	EXPECT_EQ(sp.use_count(), 1);
}

TEST(any_ptr, deletion_when_holding_unique_shared_ptr_deletes_referent) {
	DestructorTraced* dt = new DestructorTraced();
	smfs::any_ptr<DestructorTraced>* pap =
			new smfs::any_ptr<DestructorTraced>(shared_ptr<DestructorTraced>(dt));

	EXPECT_CALL(*dt, Die());
	delete(pap);
}
