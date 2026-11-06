// File: klist.h
// Creator: Not specified.
// Created: 2026-11-06
// Description: Integer singly linked list and forward iterator interfaces.
// Copyright: Not specified.

#ifndef KXXX_KLIST_H
#define KXXX_KLIST_H

#include <cstddef>
#include <iterator>

class KList
{
public:
	class Iterator;
	class ConstIterator;

	KList() noexcept;
	~KList();

	// Existing node iterators remain valid when a new head is inserted.
	void push_front(int value);
	// Empty removal has no effect. Only the removed node's iterators expire.
	void pop_front() noexcept;
	int size() const noexcept;
	bool empty() const noexcept;

	Iterator begin() noexcept;
	Iterator end() noexcept;
	ConstIterator begin() const noexcept;
	ConstIterator end() const noexcept;

private:
	struct Node
	{
		Node(int nodeValue, Node *nextNode) noexcept;

		// These names are specified by the assignment.
		int value;
		Node *next;
	};

	KList(const KList &other) = delete;
	KList &operator=(const KList &other) = delete;

	Node *m_head;
	int m_size;
};

// Defined after KList so its private Node type stays an implementation detail.
class KList::Iterator
{
public:
	typedef std::forward_iterator_tag iterator_category;
	typedef int value_type;
	typedef std::ptrdiff_t difference_type;
	typedef int *pointer;
	typedef int &reference;

	Iterator() noexcept;

	// Null/end dereference and increment throw std::out_of_range.
	reference operator*() const;
	pointer operator->() const;
	Iterator &operator++();
	Iterator operator++(int);
	bool operator==(const Iterator &other) const noexcept;
	bool operator!=(const Iterator &other) const noexcept;

private:
	friend class KList;
	friend class KList::ConstIterator;

	explicit Iterator(Node *node) noexcept;

	// Iterators borrow nodes; they never own or release them.
	Node *m_node;
};

class KList::ConstIterator
{
public:
	typedef std::forward_iterator_tag iterator_category;
	typedef int value_type;
	typedef std::ptrdiff_t difference_type;
	typedef const int *pointer;
	typedef const int &reference;

	ConstIterator() noexcept;
	// Intentionally allows conversion from a writable to a read-only iterator.
	ConstIterator(const Iterator &iterator) noexcept;

	reference operator*() const;
	pointer operator->() const;
	ConstIterator &operator++();
	ConstIterator operator++(int);
	bool operator==(const ConstIterator &other) const noexcept;
	bool operator!=(const ConstIterator &other) const noexcept;

private:
	friend class KList;

	explicit ConstIterator(const Node *node) noexcept;

	const Node *m_node;
};

#endif // KXXX_KLIST_H
