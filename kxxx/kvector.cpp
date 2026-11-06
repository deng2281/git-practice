// File: kvector.cpp
// Creator: Not specified.
// Created: 2026-11-06
// Description: Dynamic-array storage and operations for KVector.
// Copyright: Not specified.

#include "kvector.h"

#include <limits>
#include <stdexcept>

KVector::KVector() noexcept
	: data_(nullptr)
	, size_(0)
	, capacity_(0)
{
}

KVector::~KVector()
{
	delete[] data_;
}

void KVector::push_back(int value)
{
	if (size_ == capacity_)
		grow();

	data_[size_] = value;
	++size_;
}

void KVector::pop_back() noexcept
{
	if (empty())
		return;

	// Other elements remain valid; the old end and removed element do not.
	--size_;
}

int KVector::size() const noexcept
{
	return size_;
}

bool KVector::empty() const noexcept
{
	return size_ == 0;
}

int KVector::capacity() const noexcept
{
	return capacity_;
}

int &KVector::at(int index)
{
	checkIndex(index);
	return data_[index];
}

const int &KVector::at(int index) const
{
	checkIndex(index);
	return data_[index];
}

KVector::Iterator KVector::begin() noexcept
{
	return data_;
}

KVector::Iterator KVector::end() noexcept
{
	// Avoid pointer arithmetic on the initial null buffer.
	if (size_ == 0)
		return data_;

	return data_ + size_;
}

KVector::ConstIterator KVector::begin() const noexcept
{
	return data_;
}

KVector::ConstIterator KVector::end() const noexcept
{
	if (size_ == 0)
		return data_;

	return data_ + size_;
}

void KVector::grow()
{
	// Check before doubling to avoid signed integer overflow.
	if (capacity_ > std::numeric_limits<int>::max() / 2)
		throw std::length_error("KVector capacity cannot be doubled");

	const int newCapacity = capacity_ == 0 ? 1 : capacity_ * 2;
	int *newData = new int[newCapacity];
	for (int index = 0; index < size_; ++index)
		newData[index] = data_[index];

	// Allocate and copy first: allocation failure leaves this vector unchanged.
	// Reallocation invalidates all previous iterators, pointers and references.
	delete[] data_;
	data_ = newData;
	capacity_ = newCapacity;
}

void KVector::checkIndex(int index) const
{
	if (index < 0 || index >= size_)
		throw std::out_of_range("KVector index out of range");
}
