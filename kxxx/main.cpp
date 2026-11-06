// File: main.cpp
// Creator: Not specified.
// Created: 2026-11-06
// Description: Independent console tests for the first STL exercise stage.
// Copyright: Not specified.

#include "kvector.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{

typedef bool (*TestFunction)();

struct TestCase
{
	TestCase(const char *testName, TestFunction testFunction);

	const char *m_name;
	TestFunction m_function;
};

TestCase::TestCase(const char *testName, TestFunction testFunction)
	: m_name(testName)
	, m_function(testFunction)
{
}

bool Expect(bool condition, const char *message)
{
	if (!condition)
		std::cout << "    Check failed: " << message << '\n';

	return condition;
}

bool ExpectEqual(int actual, int expected, const char *message)
{
	if (actual != expected)
	{
		std::cout << "    Check failed: " << message
			<< " (expected " << expected << ", got " << actual << ")\n";
		return false;
	}

	return true;
}

bool TestInitialState()
{
	KVector values;
	const KVector &constantValues = values;
	return Expect(values.empty(), "new vector is empty")
		&& ExpectEqual(values.size(), 0, "initial size")
		&& ExpectEqual(values.capacity(), 0, "initial capacity")
		&& Expect(values.begin() == values.end(), "empty iterator range")
		&& Expect(constantValues.begin() == constantValues.end(), "empty const range");
}

bool TestInsertionOrder()
{
	KVector values;
	const int expectedValues[] = {10, -3, 0, 10};
	for (int value : expectedValues)
		values.push_back(value);

	if (!ExpectEqual(values.size(), 4, "size after insertion")
		|| !Expect(!values.empty(), "inserted vector is not empty"))
		return false;

	for (int index = 0; index < values.size(); ++index)
	{
		if (!ExpectEqual(values.at(index), expectedValues[index], "insertion order"))
			return false;
	}

	return true;
}

bool TestCapacityDoubling()
{
	KVector values;
	const int expectedCapacities[] = {1, 2, 4, 4, 8, 8, 8, 8, 16};
	for (int index = 0; index < 9; ++index)
	{
		values.push_back(index);
		if (!ExpectEqual(values.capacity(), expectedCapacities[index], "doubling capacity")
			|| !ExpectEqual(values.size(), index + 1, "size remains separate from capacity"))
			return false;
	}

	return true;
}

bool TestRepeatedGrowthPreservesValues()
{
	KVector values;
	constexpr int elementCount = 10000;
	for (int index = 0; index < elementCount; ++index)
		values.push_back(index * 3 - 11);

	if (!ExpectEqual(values.size(), elementCount, "size after repeated growth"))
		return false;

	for (int index = 0; index < elementCount; ++index)
	{
		if (!ExpectEqual(values.at(index), index * 3 - 11, "values survive reallocation"))
			return false;
	}

	return true;
}

bool TestMutableAccess()
{
	KVector values;
	values.push_back(10);
	values.push_back(20);
	values.at(1) = 42;
	*values.begin() = -7;

	return ExpectEqual(values.at(0), -7, "write through iterator")
		&& ExpectEqual(values.at(1), 42, "write through at");
}

bool TestRandomAccessIterators()
{
	KVector values;
	for (int value = 10; value <= 50; value += 10)
		values.push_back(value);

	KVector::Iterator iterator = values.begin();
	if (!ExpectEqual(*(iterator++), 10, "post-increment")
		|| !ExpectEqual(*iterator, 20, "position after increment"))
		return false;

	iterator += 2;
	if (!ExpectEqual(*iterator, 40, "iterator offset")
		|| !ExpectEqual(iterator[-1], 30, "iterator indexing"))
		return false;

	--iterator;
	iterator -= 2;
	return Expect(iterator == values.begin(), "backward movement")
		&& Expect(values.begin() < values.end(), "iterator ordering")
		&& Expect(values.end() - values.begin() == values.size(), "iterator distance")
		&& ExpectEqual(*(values.end() - 1), 50, "last element");
}

bool TestConstTraversal()
{
	KVector values;
	values.push_back(-5);
	values.push_back(0);
	values.push_back(8);
	const KVector &constantValues = values;

	int total = 0;
	int visitedCount = 0;
	for (KVector::ConstIterator iterator = constantValues.begin(); iterator != constantValues.end(); ++iterator)
	{
		total += *iterator;
		++visitedCount;
	}

	return ExpectEqual(total, 3, "sum through const iterators")
		&& ExpectEqual(visitedCount, 3, "const traversal count")
		&& ExpectEqual(constantValues.at(2), 8, "const at");
}

bool TestMutableBounds()
{
	KVector values;
	const int invalidIndices[] = {-1, 0, std::numeric_limits<int>::max()};
	for (int index : invalidIndices)
	{
		try
		{
			values.at(index);
			return Expect(false, "empty mutable at must throw");
		}
		catch (const std::out_of_range &)
		{
			// The expected exception is the result being tested.
		}
	}

	values.push_back(17);
	try
	{
		values.at(values.size());
		return Expect(false, "index equal to size must throw");
	}
	catch (const std::out_of_range &)
	{
		return ExpectEqual(values.at(0), 17, "invalid access preserves values");
	}
}

bool TestConstBounds()
{
	KVector values;
	values.push_back(23);
	const KVector &constantValues = values;
	const int invalidIndices[] = {-1, 1, std::numeric_limits<int>::max()};
	for (int index : invalidIndices)
	{
		try
		{
			constantValues.at(index);
			return Expect(false, "invalid const at must throw");
		}
		catch (const std::out_of_range &)
		{
			// No other exception type is accepted by this test.
		}
	}

	return ExpectEqual(constantValues.size(), 1, "const bounds checks preserve size")
		&& ExpectEqual(constantValues.at(0), 23, "const bounds checks preserve data");
}

bool TestRemovalAndEmptyRemoval()
{
	KVector values;
	values.pop_back();
	if (!Expect(values.empty(), "removal from an initially empty vector")
		|| !ExpectEqual(values.capacity(), 0, "empty removal does not allocate"))
		return false;

	values.push_back(10);
	values.push_back(20);
	const int savedCapacity = values.capacity();
	KVector::Iterator first = values.begin();
	values.pop_back();
	if (!ExpectEqual(values.size(), 1, "size after removing the last value")
		|| !ExpectEqual(values.at(0), 10, "first value remains")
		|| !Expect(values.begin() == first, "pop does not reallocate"))
		return false;

	values.pop_back();
	values.pop_back();
	return Expect(values.empty(), "vector becomes empty")
		&& ExpectEqual(values.size(), 0, "repeated empty removal keeps size zero")
		&& ExpectEqual(values.capacity(), savedCapacity, "removal retains capacity")
		&& Expect(values.begin() == values.end(), "emptied iterator range");
}

bool TestRefillWithoutGrowth()
{
	KVector values;
	for (int index = 0; index < 5; ++index)
		values.push_back(index);

	const int savedCapacity = values.capacity();
	KVector::Iterator savedBuffer = values.begin();
	while (!values.empty())
		values.pop_back();

	for (int index = 0; index < 5; ++index)
		values.push_back(100 + index);

	if (!ExpectEqual(values.capacity(), savedCapacity, "refill reuses capacity")
		|| !Expect(values.begin() == savedBuffer, "refill reuses the buffer"))
		return false;

	for (int index = 0; index < 5; ++index)
	{
		if (!ExpectEqual(values.at(index), 100 + index, "refilled value"))
			return false;
	}

	return true;
}

bool TestInsertFromOwnElement()
{
	KVector values;
	values.push_back(123);
	// This insertion grows the buffer while the argument comes from that buffer.
	values.push_back(values.at(0));
	return ExpectEqual(values.size(), 2, "size after insertion from own element")
		&& ExpectEqual(values.at(0), 123, "original value survives")
		&& ExpectEqual(values.at(1), 123, "argument is captured before reallocation");
}

bool TestIndependentInstancesAndLifetime()
{
	KVector persistentValues;
	persistentValues.push_back(99);
	for (int round = 0; round < 100; ++round)
	{
		KVector temporaryValues;
		for (int index = 0; index < 100; ++index)
			temporaryValues.push_back(round + index);

		temporaryValues.at(0) = -1;
		if (!ExpectEqual(persistentValues.at(0), 99, "separate instances own separate storage")
			|| !ExpectEqual(temporaryValues.at(99), round + 99, "temporary instance contents"))
			return false;
	}

	return ExpectEqual(persistentValues.size(), 1, "other destructors do not change persistent instance");
}

bool RunTest(const TestCase &testCase)
{
	std::cout << "[RUN ] " << testCase.m_name << '\n';
	try
	{
		const bool passed = testCase.m_function();
		std::cout << (passed ? "[PASS] " : "[FAIL] ") << testCase.m_name << "\n\n";
		return passed;
	}
	catch (const std::exception &error)
	{
		std::cout << "[FAIL] " << testCase.m_name << ": " << error.what() << "\n\n";
		return false;
	}
}

} // namespace

int main()
{
	const TestCase testCases[] = {
		{"KVector / initial state", TestInitialState},
		{"KVector / insertion order and duplicate values", TestInsertionOrder},
		{"KVector / capacity doubles", TestCapacityDoubling},
		{"KVector / values survive repeated growth", TestRepeatedGrowthPreservesValues},
		{"KVector / writable element access", TestMutableAccess},
		{"KVector / random-access iterators", TestRandomAccessIterators},
		{"KVector / const traversal", TestConstTraversal},
		{"KVector / mutable access bounds", TestMutableBounds},
		{"KVector / const access bounds", TestConstBounds},
		{"KVector / removal and empty removal", TestRemovalAndEmptyRemoval},
		{"KVector / refill retained storage", TestRefillWithoutGrowth},
		{"KVector / insert an existing element during growth", TestInsertFromOwnElement},
		{"KVector / independent instances and repeated destruction", TestIndependentInstancesAndLifetime}
	};

	std::cout << "=== KVector tests ===\n\n";
	int passedCount = 0;
	int totalCount = 0;
	for (const TestCase &testCase : testCases)
	{
		++totalCount;
		if (RunTest(testCase))
			++passedCount;
	}

	std::cout << "Summary: " << passedCount << '/' << totalCount << " passed\n";
	return passedCount == totalCount ? EXIT_SUCCESS : EXIT_FAILURE;
}
