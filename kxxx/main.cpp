// File: main.cpp
// Creator: Not specified.
// Created: 2026-11-06
// Description: Step-by-step examples showing container contents after each operation.
// Copyright: Not specified.

#include "kvector.h"
#include "klist.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace
{

void PrintVector(const char *operation, const KVector &values)
{
	std::cout << "  " << operation << " -> [";
	for (KVector::ConstIterator iterator = values.begin(); iterator != values.end(); ++iterator)
	{
		if (iterator != values.begin())
			std::cout << ", ";
		std::cout << *iterator;
	}
	std::cout << "]  size=" << values.size()
		<< "  capacity=" << values.capacity() << "  empty=" << values.empty() << '\n';
}

void PrintList(const char *operation, const KList &values)
{
	std::cout << "  " << operation << " -> [";
	for (KList::ConstIterator iterator = values.begin(); iterator != values.end(); ++iterator)
	{
		if (iterator != values.begin())
			std::cout << ", ";
		std::cout << *iterator;
	}
	std::cout << "]  size=" << values.size() << "  empty=" << values.empty() << '\n';
}

// Returns one failed example or zero, allowing main to report a real exit status.
int CheckResult(bool correct)
{
	std::cout << "  Check: " << (correct ? "PASS" : "FAIL") << "\n\n";
	return correct ? 0 : 1;
}

int DemoVector()
{
	std::cout << "========== KVector ==========\n\n";
	KVector values;
	int failedCount = 0;

	std::cout << "1. Insert 1, 2, 3 at the back\n";
	PrintVector("initial", values);
	const bool initiallyEmpty = values.empty() && values.size() == 0
		&& values.capacity() == 0 && values.begin() == values.end();
	values.push_back(1);
	PrintVector("push_back(1)", values);
	const bool firstCapacityCorrect = values.capacity() == 1;
	values.push_back(2);
	PrintVector("push_back(2)", values);
	const bool secondCapacityCorrect = values.capacity() == 2;
	values.push_back(3);
	PrintVector("push_back(3)", values);
	std::cout << "  Expected: [1, 2, 3]; capacity grows 0 -> 1 -> 2 -> 4\n";
	failedCount += CheckResult(initiallyEmpty && firstCapacityCorrect && secondCapacityCorrect
		&& !values.empty() && values.size() == 3 && values.capacity() == 4
		&& values.at(0) == 1 && values.at(1) == 2 && values.at(2) == 3);

	std::cout << "2. Read by index and random-access iterator\n";
	const int indexedValue = values.at(1);
	const int iteratorValue = *(values.begin() + 2);
	const auto distance = values.end() - values.begin();
	std::cout << "  at(1) = " << indexedValue << '\n'
		<< "  *(begin() + 2) = " << iteratorValue << '\n'
		<< "  end() - begin() = " << distance << '\n'
		<< "  Expected: 2, 3, 3\n";
	failedCount += CheckResult(indexedValue == 2 && iteratorValue == 3 && distance == 3);

	std::cout << "3. Change the second element\n";
	PrintVector("before", values);
	values.at(1) = 20;
	PrintVector("at(1) = 20", values);
	std::cout << "  Expected: [1, 20, 3]\n";
	failedCount += CheckResult(values.size() == 3
		&& values.at(0) == 1 && values.at(1) == 20 && values.at(2) == 3);

	std::cout << "4. Remove the last element\n";
	values.pop_back();
	PrintVector("pop_back()", values);
	std::cout << "  Expected: [1, 20]; size=2, capacity stays 4\n";
	failedCount += CheckResult(values.size() == 2 && values.capacity() == 4
		&& values.at(0) == 1 && values.at(1) == 20);

	std::cout << "5. Remove all elements, then remove from an empty vector\n";
	values.pop_back();
	PrintVector("pop_back()", values);
	values.pop_back();
	PrintVector("pop_back()", values);
	values.pop_back();
	PrintVector("pop_back() on empty vector", values);
	std::cout << "  Expected: []; size=0, empty=true, capacity stays 4\n";
	failedCount += CheckResult(values.empty() && values.size() == 0
		&& values.capacity() == 4 && values.begin() == values.end());

	std::cout << "6. Check invalid indices on the empty vector\n";
	const int invalidIndices[] = {-1, 0};
	int caughtCount = 0;
	for (int index : invalidIndices)
	{
		std::cout << "  at(" << index << ") -> ";
		try
		{
			values.at(index);
			std::cout << "no exception\n";
		}
		catch (const std::out_of_range &error)
		{
			std::cout << "out_of_range: " << error.what() << '\n';
			++caughtCount;
		}
	}
	std::cout << "  Expected: both accesses throw; vector remains empty\n";
	failedCount += CheckResult(caughtCount == 2 && values.empty() && values.size() == 0);

	std::cout << "7. Insert again after removing all elements\n";
	values.push_back(7);
	values.push_back(8);
	values.push_back(9);
	PrintVector("push_back(7), push_back(8), push_back(9)", values);
	std::cout << "  Expected: [7, 8, 9]; reuse capacity 4\n";
	failedCount += CheckResult(values.size() == 3 && values.capacity() == 4
		&& values.at(0) == 7 && values.at(1) == 8 && values.at(2) == 9);

	return failedCount;
}

int DemoList()
{
	std::cout << "========== KList ==========\n\n";
	KList values;
	int failedCount = 0;

	std::cout << "1. Insert 1, 2, 3 at the front\n";
	PrintList("initial", values);
	const bool initiallyEmpty = values.empty() && values.size() == 0 && values.begin() == values.end();
	values.push_front(1);
	PrintList("push_front(1)", values);
	values.push_front(2);
	PrintList("push_front(2)", values);
	values.push_front(3);
	PrintList("push_front(3)", values);
	std::cout << "  Expected: [3, 2, 1]; each insertion becomes the new head\n";
	KList::Iterator iterator = values.begin();
	const bool insertionOrderCorrect = *iterator++ == 3 && *iterator++ == 2
		&& *iterator++ == 1 && iterator == values.end();
	failedCount += CheckResult(initiallyEmpty && !values.empty()
		&& values.size() == 3 && insertionOrderCorrect);

	std::cout << "2. Move the iterator forward\n";
	iterator = values.begin();
	const int firstValue = *iterator;
	const int secondValue = *(++iterator);
	const int thirdValue = *(++iterator);
	++iterator;
	std::cout << "  *begin() = " << firstValue << '\n'
		<< "  advance once = " << secondValue << '\n'
		<< "  advance twice = " << thirdValue << '\n'
		<< "  advance again reaches end(): " << (iterator == values.end()) << '\n'
		<< "  Expected: 3 -> 2 -> 1 -> end\n";
	failedCount += CheckResult(firstValue == 3 && secondValue == 2
		&& thirdValue == 1 && iterator == values.end());

	std::cout << "3. Change the head value through the iterator\n";
	PrintList("before", values);
	*values.begin() = 30;
	PrintList("*begin() = 30", values);
	std::cout << "  Expected: [30, 2, 1]; node count stays 3\n";
	failedCount += CheckResult(*values.begin() == 30 && values.size() == 3);

	std::cout << "4. Remove the head node\n";
	values.pop_front();
	PrintList("pop_front()", values);
	std::cout << "  Expected: [2, 1]\n";
	iterator = values.begin();
	const bool removalOrderCorrect = *iterator++ == 2 && *iterator++ == 1 && iterator == values.end();
	failedCount += CheckResult(values.size() == 2 && removalOrderCorrect);

	std::cout << "5. Remove all nodes, then remove from an empty list\n";
	values.pop_front();
	PrintList("pop_front()", values);
	values.pop_front();
	PrintList("pop_front()", values);
	values.pop_front();
	PrintList("pop_front() on empty list", values);
	std::cout << "  Expected: []; size=0, empty=true\n";
	failedCount += CheckResult(values.empty() && values.size() == 0 && values.begin() == values.end());

	std::cout << "6. Insert again after removing all nodes\n";
	values.push_front(9);
	PrintList("push_front(9)", values);
	std::cout << "  Expected: [9]; size=1\n";
	failedCount += CheckResult(!values.empty() && values.size() == 1 && *values.begin() == 9);

	std::cout << "7. Existing node iterator survives insertion and removal of a new head\n";
	KList::Iterator savedIterator = values.begin();
	values.push_front(8);
	PrintList("push_front(8)", values);
	const int savedValueAfterInsertion = *savedIterator;
	std::cout << "  saved iterator still reads " << savedValueAfterInsertion << '\n';
	values.pop_front();
	PrintList("pop_front()", values);
	std::cout << "  saved iterator still reads " << *savedIterator << '\n'
		<< "  Expected: [8, 9] -> [9]; saved iterator always reads 9\n";
	failedCount += CheckResult(savedValueAfterInsertion == 9
		&& *savedIterator == 9 && values.begin() == savedIterator && values.size() == 1);

	return failedCount;
}

} // namespace

int main()
{
	std::cout << std::boolalpha;
	try
	{
		const int vectorFailures = DemoVector();
		const int listFailures = DemoList();
		const int failureCount = vectorFailures + listFailures;
		std::cout << "========== Result ==========\n";
		if (failureCount == 0)
			std::cout << "All 14 examples passed.\n";
		else
			std::cout << failureCount << " example(s) failed.\n";
		return failureCount == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
	}
	catch (const std::exception &error)
	{
		std::cout << "Unexpected error: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
