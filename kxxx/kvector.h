// File: kvector.h
// Creator: Not specified.
// Created: 2026-11-06
// Description: Integer vector interface for the STL implementation exercise.
// Copyright: Not specified.

#ifndef KXXX_KVECTOR_H
#define KXXX_KVECTOR_H

class KVector
{
public:
	typedef int *Iterator;
	typedef const int *ConstIterator;

	KVector() noexcept;
	~KVector();

	void push_back(int value);
	// Removing from an empty vector has no effect. Capacity is retained.
	void pop_back() noexcept;
	int size() const noexcept;
	bool empty() const noexcept;
	// Exposes allocated capacity so the doubling policy can be verified.
	int capacity() const noexcept;

	// Throws std::out_of_range for a negative index or index >= size().
	int &at(int index);
	const int &at(int index) const;

	Iterator begin() noexcept;
	Iterator end() noexcept;
	ConstIterator begin() const noexcept;
	ConstIterator end() const noexcept;

private:
	KVector(const KVector &other) = delete;
	KVector &operator=(const KVector &other) = delete;

	void grow();
	void checkIndex(int index) const;

	// Assignment-required names take precedence over the usual member prefix.
	int *data_;
	int size_;
	int capacity_;
};

#endif // KXXX_KVECTOR_H
