/*
 * containerTest.cpp
 *
 *  Created on: 8 Apr 2016
 *      Author: Rowan Lonsdale
 */


#include "gtest/gtest.h"

#include "smfs/container.h"

#include <vector>
#include <list>
#include <string>
#include <type_traits>

using namespace smfs;
using namespace std;

TEST(collection_wrapper, can_be_constructed_from_shared_ptr_vector) {
	shared_ptr<vector<string>> vec = make_shared<vector<string>>();
	collection<string> *col = wrap_collection<string, vector<string>>(vec);

	ASSERT_TRUE((is_same<remove_reference<decltype(*col)>::type::value_type, string>::value))
			<< "Collection member value type is not as expected.";

	EXPECT_TRUE(col->empty());

	vec->push_back("hi");
	ASSERT_EQ(vec->size(), 1);
	EXPECT_EQ(col->size(), 1);

	EXPECT_EQ(col->at(0), "hi");
	EXPECT_EQ((*col)[0], "hi");

	delete col;
	delete vec;
}

TEST(collection_wrapper, can_be_constructed_from_ptr_list) {
	list<string> *lst = new list<string>();
	collection<string> *col = wrap_collection<string, list<string>>(lst);

	ASSERT_TRUE((is_same<remove_reference<decltype(*col)>::type::value_type, string>::value))
				<< "Collection member value type is not as expected.";

	EXPECT_TRUE(col->empty()) << "Collection is not empty on creation.";

	lst->push_back("bye");
	ASSERT_EQ(lst->size(), 1) << "List should contain one element but actually contains " << lst->size();
	EXPECT_EQ(col->size(), 1) << "Collection should contain one element but actually contains " << col->size();
	EXPECT_FALSE(col->empty()) << "Collection is empty even though it contains an element.";

	EXPECT_EQ(col->at(0), "bye") << "Element zero of the collection is not as expected.";
	EXPECT_EQ((*col)[0], "bye") << "The index operator of the collection does not return the correct element.";

	delete col;
	delete lst;
}

// Should not compile
//TEST(collection_wrapper, cannot_be_constructed_from_int) {
//	collection<string> *col = new collection_wrapper<int, string>(2);
//}
