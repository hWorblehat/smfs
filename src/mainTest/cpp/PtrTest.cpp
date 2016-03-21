/*
 * PtrTest.cpp
 *
 *  Created on: 12 Mar 2016
 *      Author: rowan
 */

#include "gtest/gtest.h"
#include <string>
#include <memory>

#include "smfs/ptr.h"

using namespace std;

TEST(any_ptr, constructFromShared) {
	shared_ptr<string> sp = shared_ptr<string>(new string("hello"));
	smfs::any_ptr<string> ap(sp);
	EXPECT_EQ(*ap, string("hello"));
	EXPECT_EQ(ap->length(), 5);
	EXPECT_TRUE(ap);
}

TEST(any_ptr, deleteFromShared) {
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

TEST(any_ptr, fromSharedUnique) {

}
