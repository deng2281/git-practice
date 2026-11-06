// File: main.cpp
// Creator: Not specified.
// Created: 2026-11-06
// Description: Independent console tests for KVector and KList.
// Copyright: Not specified.

#include "kvector.h"
#include "klist.h"

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

bool ExpectListSequence(const KList &values, const int *expectedValues, int expectedCount)
{
	if (!ExpectEqual(values.size(), expectedCount, "list size matches expected sequence"))
		return false;

	KList::ConstIterator iterator = values.begin();
	for (int index = 0; index < expectedCount; ++index)
	{
		if (!Expect(iterator != values.end(), "list has the expected node")
			|| !ExpectEqual(*iterator, expectedValues[index], "list element order"))
			return false;
		++iterator;
	}

	return Expect(iterator == values.end(), "list has no extra nodes");
}

bool TestListInitialState()
{
	KList values;
	const KList &constantValues = values;
	const KList::Iterator defaultIterator;
	const KList::ConstIterator defaultConstIterator;
	return Expect(values.empty(), "new list is empty")
		&& ExpectEqual(values.size(), 0, "initial list size")
		&& Expect(values.begin() == values.end(), "empty list iterator range")
		&& Expect(constantValues.begin() == constantValues.end(), "empty list const range")
		&& Expect(defaultIterator == values.end(), "default iterator is null")
		&& Expect(defaultConstIterator == constantValues.end(), "default const iterator is null");
}

bool TestListHeadInsertion()
{
	KList values;
	const int insertedValues[] = {10, -3, 0, 10};
	for (int value : insertedValues)
		values.push_front(value);

	const int expectedValues[] = {10, 0, -3, 10};
	return Expect(!values.empty(), "inserted list is not empty")
		&& ExpectListSequence(values, expectedValues, 4);
}

bool TestListHeadRemoval()
{
	KList values;
	values.push_front(10);
	values.push_front(20);
	values.push_front(30);
	values.pop_front();
	const int remainingValues[] = {20, 10};
	if (!ExpectListSequence(values, remainingValues, 2))
		return false;

	values.pop_front();
	const int lastValue[] = {10};
	return ExpectListSequence(values, lastValue, 1);
}

bool TestListEmptyRemovalAndReuse()
{
	KList values;
	values.pop_front();
	values.pop_front();
	if (!Expect(values.empty(), "empty removal has no effect")
		|| !ExpectEqual(values.size(), 0, "empty removal keeps size zero"))
		return false;

	values.push_front(42);
	if (!ExpectEqual(*values.begin(), 42, "single inserted node"))
		return false;
	values.pop_front();
	if (!Expect(values.begin() == values.end(), "single node removed completely"))
		return false;

	values.pop_front();
	values.push_front(-8);
	const int expectedValues[] = {-8};
	return ExpectListSequence(values, expectedValues, 1);
}

bool TestListMutableTraversal()
{
	KList values;
	values.push_front(1);
	values.push_front(2);
	values.push_front(3);
	for (int &value : values)
		value *= 10;

	KList::Iterator iterator = values.begin();
	*iterator = 99;
	const int expectedValues[] = {99, 20, 10};
	return Expect(iterator.operator->() == &(*iterator), "arrow exposes the same element")
		&& ExpectListSequence(values, expectedValues, 3);
}

bool TestListIteratorIncrementAndCopies()
{
	KList values;
	values.push_front(10);
	values.push_front(20);
	values.push_front(30);
	KList::Iterator iterator = values.begin();
	KList::Iterator copy = iterator;
	KList::Iterator previous = iterator++;
	if (!Expect(previous == copy, "post-increment returns the old position")
		|| !ExpectEqual(*previous, 30, "old position remains readable")
		|| !ExpectEqual(*iterator, 20, "post-increment advances once")
		|| !Expect(iterator != copy, "advancing a copy does not advance the original"))
		return false;

	KList::Iterator &advanced = ++iterator;
	if (!Expect(&advanced == &iterator, "prefix increment returns this iterator")
		|| !ExpectEqual(*iterator, 10, "prefix increment advances once"))
		return false;

	++iterator;
	return Expect(iterator == values.end(), "increment from last node reaches end")
		&& ExpectEqual(*copy, 30, "copied iterator has an independent position");
}

bool TestListConstTraversal()
{
	KList values;
	values.push_front(1);
	values.push_front(2);
	values.push_front(3);
	const KList &constantValues = values;
	KList::ConstIterator iterator = constantValues.begin();
	KList::ConstIterator previous = iterator++;
	if (!ExpectEqual(*previous, 3, "const postfix increment returns old value")
		|| !ExpectEqual(*iterator, 2, "const iterator advances")
		|| !Expect(iterator.operator->() == &(*iterator), "const arrow points at the element"))
		return false;

	KList::ConstIterator converted = values.begin();
	if (!Expect(converted == constantValues.begin(), "mutable iterator converts to const iterator")
		|| !Expect(converted != iterator, "const iterator comparison"))
		return false;

	int total = 0;
	int visitedCount = 0;
	for (int value : constantValues)
	{
		total += value;
		++visitedCount;
	}

	return ExpectEqual(total, 6, "const traversal sum")
		&& ExpectEqual(visitedCount, 3, "const traversal count")
		&& Expect(std::distance(constantValues.begin(), constantValues.end()) == 3,
			"iterator traits support a standard traversal algorithm");
}

bool TestListInsertionPreservesIterators()
{
	KList values;
	values.push_front(10);
	KList::Iterator existingNode = values.begin();
	int *existingValue = &(*existingNode);
	values.push_front(20);
	KList::Iterator nextNode = values.begin();
	++nextNode;

	return ExpectEqual(*existingNode, 10, "head insertion preserves existing node")
		&& Expect(&(*existingNode) == existingValue, "head insertion preserves node address")
		&& Expect(nextNode == existingNode, "new head links to the previous head")
		&& ExpectEqual(values.size(), 2, "size after inserting a new head");
}

bool TestListRemovalPreservesSurvivingIterators()
{
	KList values;
	values.push_front(10);
	values.push_front(20);
	values.push_front(30);
	KList::Iterator survivingNode = values.begin();
	++survivingNode;
	int *survivingValue = &(*survivingNode);
	values.pop_front();

	// The erased head's iterator must not be dereferenced.
	return Expect(values.begin() == survivingNode, "surviving node becomes the new head")
		&& Expect(&(*survivingNode) == survivingValue, "surviving node address is unchanged")
		&& ExpectEqual(*survivingNode, 20, "surviving iterator remains readable")
		&& ExpectEqual(values.size(), 2, "size after head removal");
}

bool TestListCheckedEndOperations()
{
	KList values;
	KList::Iterator iterator = values.end();
	const KList &constantValues = values;
	KList::ConstIterator constIterator = constantValues.end();
	int caughtCount = 0;
	try
	{
		*iterator;
	}
	catch (const std::out_of_range &)
	{
		++caughtCount;
	}
	try
	{
		++iterator;
	}
	catch (const std::out_of_range &)
	{
		++caughtCount;
	}
	try
	{
		*constIterator;
	}
	catch (const std::out_of_range &)
	{
		++caughtCount;
	}
	try
	{
		++constIterator;
	}
	catch (const std::out_of_range &)
	{
		++caughtCount;
	}

	return ExpectEqual(caughtCount, 4, "end dereference and increment are rejected")
		&& Expect(values.empty(), "invalid operations do not change the list")
		&& Expect(iterator == values.end(), "failed increment preserves mutable end")
		&& Expect(constIterator == constantValues.end(), "failed increment preserves const end");
}

bool TestListManyNodesAndRemoval()
{
	KList values;
	constexpr int elementCount = 10000;
	for (int index = 0; index < elementCount; ++index)
		values.push_front(index);

	if (!ExpectEqual(values.size(), elementCount, "large list size"))
		return false;
	KList::Iterator iterator = values.begin();
	for (int index = elementCount - 1; index >= 0; --index)
	{
		if (!Expect(iterator != values.end(), "large list contains the expected node")
			|| !ExpectEqual(*iterator, index, "large list traversal order"))
			return false;
		++iterator;
	}
	if (!Expect(iterator == values.end(), "large list traversal terminates"))
		return false;

	for (int index = elementCount - 1; index >= 0; --index)
	{
		if (!ExpectEqual(*values.begin(), index, "head changes correctly during removal"))
			return false;
		values.pop_front();
		if (!ExpectEqual(values.size(), index, "size decreases once per removal"))
			return false;
	}

	return Expect(values.empty(), "all nodes removed")
		&& Expect(values.begin() == values.end(), "empty range after large removal");
}

bool TestListIndependentInstancesAndLifetime()
{
	KList persistentValues;
	persistentValues.push_front(99);
	for (int round = 0; round < 100; ++round)
	{
		KList temporaryValues;
		for (int index = 0; index < 100; ++index)
			temporaryValues.push_front(round + index);
		*temporaryValues.begin() = -1;
		if (!ExpectEqual(*persistentValues.begin(), 99, "separate lists own separate nodes")
			|| !ExpectEqual(temporaryValues.size(), 100, "temporary list size"))
			return false;
	}

	return ExpectEqual(*persistentValues.begin(), 99, "other destructors preserve this list")
		&& ExpectEqual(persistentValues.size(), 1, "persistent list size");
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

int RunSuite(const char *name, const TestCase *testCases, int testCount)
{
	std::cout << "=== " << name << " tests ===\n\n";
	int passedCount = 0;
	for (int index = 0; index < testCount; ++index)
	{
		if (RunTest(testCases[index]))
			++passedCount;
	}

	std::cout << name << " summary: " << passedCount << '/' << testCount << " passed\n\n";
	return testCount - passedCount;
}

} // namespace

int main()
{
	const TestCase vectorTests[] = {
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
	const TestCase listTests[] = {
		{"KList / initial state", TestListInitialState},
		{"KList / head insertion order and duplicate values", TestListHeadInsertion},
		{"KList / head removal", TestListHeadRemoval},
		{"KList / empty removal, single node and reuse", TestListEmptyRemovalAndReuse},
		{"KList / writable traversal", TestListMutableTraversal},
		{"KList / iterator increment and copies", TestListIteratorIncrementAndCopies},
		{"KList / const traversal and iterator traits", TestListConstTraversal},
		{"KList / insertion preserves existing iterators", TestListInsertionPreservesIterators},
		{"KList / removal preserves surviving iterators", TestListRemovalPreservesSurvivingIterators},
		{"KList / checked end operations", TestListCheckedEndOperations},
		{"KList / many nodes, traversal and removal", TestListManyNodesAndRemoval},
		{"KList / independent instances and repeated destruction", TestListIndependentInstancesAndLifetime}
	};

	const int vectorTestCount = static_cast<int>(sizeof(vectorTests) / sizeof(vectorTests[0]));
	const int listTestCount = static_cast<int>(sizeof(listTests) / sizeof(listTests[0]));
	const int vectorFailures = RunSuite("KVector", vectorTests, vectorTestCount);
	const int listFailures = RunSuite("KList", listTests, listTestCount);
	const int totalCount = vectorTestCount + listTestCount;
	const int failureCount = vectorFailures + listFailures;
	std::cout << "Total: " << totalCount - failureCount << '/' << totalCount << " passed\n";
	return failureCount == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
