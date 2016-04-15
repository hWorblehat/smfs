/*
 * containerTest.cpp
 *
 *  Created on: 8 Apr 2016
 *      Author: Rowan Lonsdale
 */


#include "gtest/gtest.h"

#include "smfs/container.h"

#include <vector>
#include <string>
#include <type_traits>

using namespace smfs;
using namespace std;

TEST(collection_wrapper, can_be_constructed_from_vector) {
	shared_ptr<vector<string>> vec = make_shared<vector<string>>();
	collection<string> *col = new collection_wrapper<vector<string>, string>(vec);

	ASSERT_TRUE((is_same<remove_reference<decltype(*col)>::type::value_type, string>::value))
			<< "Collection member value type is not as expected.";
}
