/*
 * test_main.cpp
 *
 *  Created on: 19 Nov 2016
 *      Author: rowan
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <iostream>

using namespace testing;

int main(int argc, char **argv) {
	std::cerr << "Argc: " << argc << std::endl;
	std::cerr << argv[0] << std::endl;
	::testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}
