/*
 * AnyTest.cpp
 *
 *  Created on: 28 Apr 2017
 *      Author: rowan
 */

#include "../../uulib/headers/uulib/Capsule.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <iostream>

namespace uulib {
namespace test {

using ::testing::StrictMock;

struct Tracer {
	MOCK_METHOD0(move, void());
	MOCK_METHOD0(copy, void());
	MOCK_METHOD0(destroy, void());
	MOCK_METHOD0(move_assign, void());
	MOCK_METHOD0(copy_assign, void());
};

struct Thing {
	virtual ~Thing() noexcept {}
	virtual int value() const = 0;
	virtual void change() = 0;
	virtual void trace(Tracer * tracer) = 0;

	friend bool operator==(Thing const &a, Thing const &b) {
		return a.value()==b.value();
	}

	friend bool operator!=(Thing const &a, Thing const &b) {
		return a.value()!=b.value();
	}

};

std::ostream& operator<<(std::ostream& stream, Thing const &thing) {
	return stream << "Thing(" << thing.value() << ")";
}

template<std::size_t padding>
struct SomeThing : virtual Thing {

	char junk[padding];
	// Inspected value should go after padding, so we access 'deeper' memory
	int val;
	Tracer * tracer;

	SomeThing(int val, Tracer *tracer)
				: val(val), tracer(tracer) {}

	SomeThing(int val) : SomeThing(val, nullptr) {}

	SomeThing(SomeThing<padding> const &other) : SomeThing(other.val, other.tracer) {
		if(tracer) tracer->copy();
	}

	SomeThing(SomeThing<padding> &&other) : SomeThing(other.val, other.tracer) {
		if(tracer) tracer->move();
	}

	~SomeThing() {
		if(tracer) tracer->destroy();
	}

	int value() const override {
		return val;
	}

	void change() override {
		++val;
	}

	void trace(Tracer* tracer) override {
		SomeThing<padding>::tracer = tracer;
	}

};

using SmallThing = SomeThing<0>;
using BigThing = SomeThing<256>;

using AnyThing = uulib::CapsuleBigEnough<Thing, SmallThing>;

static_assert(sizeof(SmallThing) <= AnyThing::static_limit,
		"SmallThing is not small enough to validly test Any's static form.");
static_assert(sizeof(BigThing) > sizeof(AnyThing),
		"BigThing is not big enough to validly test Any's dynamic form");

static_assert(!std::is_move_assignable<SmallThing>::value,
		"SmallThing must be move-assignable");
static_assert(!std::is_copy_assignable<SmallThing>::value,
		"SmallThing must be copy-assignable");
static_assert(!std::is_move_assignable<BigThing>::value,
		"BigThing must be move-assignable");
static_assert(!std::is_copy_assignable<BigThing>::value,
		"BigThing must be copy-assignable");

template<typename T>
void testConstructAndDereference() {
	StrictMock<Tracer> tracer;
	{
		AnyThing thing(T(3));
		EXPECT_EQ(3, thing->value());

		thing->trace(&tracer);
		EXPECT_CALL(tracer, destroy()).Times(1);
	}
}

TEST(AnyTest, construct_and_dereference_static) {
	testConstructAndDereference<SmallThing>();
}

TEST(AnyTest, construct_and_dereference_dynamic) {
	testConstructAndDereference<BigThing>();
}

TEST(AnyTest, compare) {
	AnyThing thing(SmallThing(5));
	AnyThing otherThing(SmallThing(6));
	EXPECT_NE(thing, otherThing);
	EXPECT_EQ(thing, thing);

	AnyThing thirdThing(SmallThing(6));
	EXPECT_EQ(otherThing, thirdThing);
}

template<typename T>
void testCopyConstruction() {
	StrictMock<Tracer> tracer;
	{
		AnyThing thing(T(4));
		ASSERT_EQ(4, thing->value()) << "Failed to construct";;
		thing->trace(&tracer);


		EXPECT_CALL(tracer, copy()).Times(1);
		AnyThing otherThing(thing);
		EXPECT_EQ(4, otherThing->value()) << "Failed to copy-construct";

		otherThing->change();
		EXPECT_EQ(4, thing->value()) << "Copied value did not change as expected";
		EXPECT_EQ(5, otherThing->value()) << "Original value changed with copy";

		thing->change();
		EXPECT_EQ(5, thing->value()) << "Original value did not change as expected";
		EXPECT_EQ(5, otherThing->value()) << "Copied value changed with copy";

		EXPECT_CALL(tracer, destroy()).Times(2);
	}
}

TEST(AnyTest, copy_construct_static) {
	testCopyConstruction<SmallThing>();
}

TEST(AnyTest, copy_construct_dynamic) {
	testCopyConstruction<BigThing>();
}

template<typename T, bool shouldMove>
void testMoveConstruction() {
	StrictMock<Tracer> tracer;
	{
		AnyThing thing(T(4));
		ASSERT_EQ(4, thing->value()) << "Failed to construct";;

		thing->trace(&tracer);
		EXPECT_CALL(tracer, move()).Times(shouldMove ? 1 : 0);
		AnyThing otherThing(std::move(thing));
		EXPECT_EQ(4, otherThing->value()) << "Failed to move-construct";

		otherThing->trace(nullptr);
		EXPECT_CALL(tracer, destroy()).Times(shouldMove ? 1 : 0);
	}
}

TEST(AnyTest, move_construct_static) {
	testMoveConstruction<SmallThing, true>();
}

TEST(AnyTest, move_construct_dynamic) {
	testMoveConstruction<BigThing, false>();
}

template<typename T>
void testCopyAssignment() {
	StrictMock<Tracer> tracer;
	{
		AnyThing thing(T(17));
		ASSERT_EQ(17, thing->value()) << "Failed to construct";

		thing->trace(&tracer);
		EXPECT_CALL(tracer, copy()).Times(1);
		AnyThing assignee = thing;
		EXPECT_EQ(17, assignee->value()) << "Failed to assign to empty";

		thing->change();
		ASSERT_EQ(18, thing->value()) << "Failed to change original";
		EXPECT_EQ(17, assignee->value()) << "Changing original changed copy";

		EXPECT_CALL(tracer, destroy()).Times(1);
		EXPECT_CALL(tracer, copy()).Times(1);
		assignee = thing;
		EXPECT_EQ(18, assignee->value()) << "Failed to assign to already-initialised";

		EXPECT_CALL(tracer, destroy()).Times(2);
	}
}

TEST(AnyTest, copy_assign_static_static) {
	testCopyAssignment<SmallThing>();
}

TEST(AnyTest, copy_assign_dynamic_dynamic) {
	testCopyAssignment<BigThing>();
}

template<typename T1, typename T2>
void testCrossCopyAssignment() {
	StrictMock<Tracer> tracer;
	StrictMock<Tracer> otherTracer;

	{
		AnyThing thing(T1(724));
		thing->trace(&tracer);

		AnyThing otherThing(T2(658));
		otherThing->trace(&otherTracer);

		EXPECT_CALL(tracer, destroy()).Times(1);
		EXPECT_CALL(otherTracer, copy()).Times(1);
		thing = otherThing;
		ASSERT_EQ(658, thing->value()) << "Failed to copy-assign";

		EXPECT_CALL(otherTracer, destroy()).Times(2);
	}
}

TEST(AnyTest, copy_assign_dynamic_static) {
	testCrossCopyAssignment<SmallThing, BigThing>();
}

TEST(AnyTest, copy_assign_static_dynamic) {
	testCrossCopyAssignment<BigThing, SmallThing>();
}

TEST(AnyTest, move_assign_static_static) {
	StrictMock<Tracer> tracer1;
	StrictMock<Tracer> tracer2;
	{
		AnyThing thing(SmallThing(17));
		ASSERT_EQ(17, thing->value()) << "Failed to construct";
		thing->trace(&tracer1);

		AnyThing assignee(SmallThing(5)); //Optimises to move-construction
		ASSERT_EQ(5, assignee->value()) << "Failed to construct";
		assignee->trace(&tracer2);

		//Assignee's initial value will be destroyed...
		EXPECT_CALL(tracer2, destroy()).Times(1);

		//... then move-constructed
		EXPECT_CALL(tracer1, move()).Times(1);

		assignee = std::move(thing);
		EXPECT_EQ(17, assignee->value()) << "Failed to move-assign";

		EXPECT_CALL(tracer1, destroy()).Times(2); /*'thing' and  'assignee'
		                                             should be cleaned up*/
	}
}

template<typename T>
void testDynamicMoveAssignment() {
	StrictMock<Tracer> tracer1;
	StrictMock<Tracer> tracer2;
	{
		AnyThing* thing = new AnyThing(T(564));
		ASSERT_EQ(564, (*thing)->value()) << "Failed to construct";
		(*thing)->trace(&tracer1);

		AnyThing assignee(T(5)); //Optimises to move-construction
		ASSERT_EQ(5, assignee->value()) << "Failed to construct";
		assignee->trace(&tracer2);

		// No operations should occur during move-assignment,
		// since it should just be a pointer swap
		assignee = std::move(*thing);
		EXPECT_EQ(564, assignee->value()) << "Failed to move-assign";

		EXPECT_CALL(tracer2, destroy()).Times(1); /* 'thing' should now
		                                             contain 'assignee' */
		delete thing;

		EXPECT_CALL(tracer1, destroy()).Times(1); /* 'assignee' should be
		                                             cleaned up */
	}
}

TEST(AnyTest, move_assign_dynamic_dynamic) {
	testDynamicMoveAssignment<BigThing>();
}

template<typename T1, typename T2, bool shouldMove>
void testCrossMoveAssignment() {
	StrictMock<Tracer> tracer;
	StrictMock<Tracer> otherTracer;

	{
		AnyThing thing(T1(724));
		thing->trace(&tracer);

		AnyThing otherThing(T2(658));
		otherThing->trace(&otherTracer);

		// Assignee should always be cleaned up first
		EXPECT_CALL(tracer, destroy()).Times(1);
		EXPECT_CALL(otherTracer, move()).Times(shouldMove ? 1 : 0);
		thing = std::move(otherThing);
		ASSERT_EQ(658, thing->value()) << "Failed to copy-assign";


		EXPECT_CALL(otherTracer, destroy()).Times(shouldMove ? 2 : 1);
	}
}

TEST(AnyTest, move_assign_dynamic_static) {
	testCrossMoveAssignment<SmallThing, BigThing, false>();
}

TEST(AnyTest, move_assign_static_dynamic) {
	testCrossMoveAssignment<BigThing, SmallThing, true>();
}

template<std::size_t padding>
struct SomeAssignableThing : SomeThing<padding> {

	using SomeThing<padding>::tracer;
	using SomeThing<padding>::val;

	SomeAssignableThing(int val) : SomeThing<padding>(val) {}
	SomeAssignableThing(SomeAssignableThing<padding> const & other)
		: SomeThing<padding>(other) {}
	SomeAssignableThing(SomeAssignableThing<padding>&& other)
		: SomeThing<padding>(std::move(other)) {}

	SomeAssignableThing<padding>& operator=(
			SomeAssignableThing<padding> const & other) {
		if(this!=&other) {
			if(tracer) tracer->copy_assign();
			tracer = other.tracer;
			val = other.val;
		}
		return *this;
	}

	SomeAssignableThing<padding>& operator=(
			SomeAssignableThing<padding>&& other) {
		if(this!=&other) {
			if(tracer) tracer->move_assign();
			using std::swap;
			swap(tracer, other.tracer);
			swap(val, other.val);
		}
		return *this;
	}

};

using SmallAssignableThing = SomeAssignableThing<0>;
using BigAssignableThing = SomeAssignableThing<sizeof(AnyThing)>;

static_assert(sizeof(SmallAssignableThing) <= AnyThing::static_limit,
		"SmallAssignableThing is not small enough to validly test Any's static form.");
static_assert(sizeof(BigAssignableThing) > sizeof(AnyThing),
		"BigAssignableThing is not big enough to validly test Any's dynamic form");

static_assert(std::is_move_assignable<SmallAssignableThing>::value,
		"SmallAssignableThing must be move-assignable");
static_assert(std::is_copy_assignable<SmallAssignableThing>::value,
		"SmallAssignableThing must be copy-assignable");
static_assert(std::is_move_assignable<BigAssignableThing>::value,
		"BigAssignableThing must be move-assignable");
static_assert(std::is_copy_assignable<BigAssignableThing>::value,
		"BigAssignableThing must be copy-assignable");

TEST(AnyTest, move_assign_with_operator) {
	StrictMock<Tracer> tracer1;
	StrictMock<Tracer> tracer2;
	{
		AnyThing thing(SmallAssignableThing(17));
		ASSERT_EQ(17, thing->value()) << "Failed to construct";
		thing->trace(&tracer1);

		AnyThing assignee(SmallAssignableThing(5)); //Optimises to move-construction
		ASSERT_EQ(5, assignee->value()) << "Failed to construct";
		assignee->trace(&tracer2);

		// Assignee's move-assignment operator should be called
		EXPECT_CALL(tracer2, move_assign());

		assignee = std::move(thing);
		EXPECT_EQ(17, assignee->value()) << "Failed to move-assign";

		// Things were swapped, so each tracer should be 'destroyed' once
		EXPECT_CALL(tracer1, destroy()).Times(1);
		EXPECT_CALL(tracer2, destroy()).Times(1);
	}
}

TEST(AnyTest, move_assign_dynamic_with_operator) {
	// Shouldn't use assignment operator as it'll almost certainly
	// be quicker to just swap pointers.
	testDynamicMoveAssignment<BigAssignableThing>();
}

template<typename T>
void testCopyAssignWithOperator() {
	StrictMock<Tracer> tracer1;
	StrictMock<Tracer> tracer2;
	{
		AnyThing thing(T(17));
		ASSERT_EQ(17, thing->value()) << "Failed to construct";
		thing->trace(&tracer1);

		AnyThing assignee(T(5)); //Optimises to move-construction
		ASSERT_EQ(5, assignee->value()) << "Failed to construct";
		assignee->trace(&tracer2);

		// Assignee's move-assignment operator should be called
		EXPECT_CALL(tracer2, copy_assign());

		assignee = thing;
		EXPECT_EQ(17, assignee->value()) << "Failed to move-assign";

		// tracer1 has now been copied to both things
		EXPECT_CALL(tracer1, destroy()).Times(2);
	}
}

TEST(AnyTest, copy_assign_static_with_operator) {
	testCopyAssignWithOperator<SmallAssignableThing>();
}

TEST(AnyTest, copy_assign_dynamic_with_operator) {
	testCopyAssignWithOperator<BigAssignableThing>();
}

}
}
