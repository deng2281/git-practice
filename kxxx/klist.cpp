// File: klist.cpp
// Creator: Not specified.
// Created: 2026-11-06
// Description: Node ownership and forward iteration for KList.
// Copyright: Not specified.

#include "klist.h"

#include <limits>
#include <stdexcept>

KList::Node::Node(int nodeValue, Node *nextNode) noexcept
	: value(nodeValue)
	, next(nextNode)
{
}

KList::KList() noexcept
	: m_head(nullptr)
	, m_size(0)
{
}

KList::~KList()
{
	// Iterative cleanup avoids recursive destruction of a long chain.
	while (!empty())
		pop_front();
}

void KList::push_front(int value)
{
	if (m_size == std::numeric_limits<int>::max())
		throw std::length_error("KList size limit reached");

	// Allocate before changing links so allocation failure preserves the list.
	Node *newHead = new Node(value, m_head);
	m_head = newHead;
	++m_size;
}

void KList::pop_front() noexcept
{
	if (empty())
		return;

	Node *removedNode = m_head;
	m_head = removedNode->next;
	delete removedNode;
	--m_size;
}

int KList::size() const noexcept
{
	return m_size;
}

bool KList::empty() const noexcept
{
	return m_size == 0;
}

KList::Iterator KList::begin() noexcept
{
	return Iterator(m_head);
}

KList::Iterator KList::end() noexcept
{
	return Iterator(nullptr);
}

KList::ConstIterator KList::begin() const noexcept
{
	return ConstIterator(m_head);
}

KList::ConstIterator KList::end() const noexcept
{
	return ConstIterator(nullptr);
}

KList::Iterator::Iterator() noexcept
	: m_node(nullptr)
{
}

KList::Iterator::Iterator(Node *node) noexcept
	: m_node(node)
{
}

KList::Iterator::reference KList::Iterator::operator*() const
{
	if (m_node == nullptr)
		throw std::out_of_range("Cannot dereference KList end iterator");

	return m_node->value;
}

KList::Iterator::pointer KList::Iterator::operator->() const
{
	return &operator*();
}

KList::Iterator &KList::Iterator::operator++()
{
	if (m_node == nullptr)
		throw std::out_of_range("Cannot increment KList end iterator");

	m_node = m_node->next;
	return *this;
}

KList::Iterator KList::Iterator::operator++(int)
{
	Iterator previous = *this;
	++(*this);
	return previous;
}

bool KList::Iterator::operator==(const Iterator &other) const noexcept
{
	return m_node == other.m_node;
}

bool KList::Iterator::operator!=(const Iterator &other) const noexcept
{
	return !(*this == other);
}

KList::ConstIterator::ConstIterator() noexcept
	: m_node(nullptr)
{
}

KList::ConstIterator::ConstIterator(const Iterator &iterator) noexcept
	: m_node(iterator.m_node)
{
}

KList::ConstIterator::ConstIterator(const Node *node) noexcept
	: m_node(node)
{
}

KList::ConstIterator::reference KList::ConstIterator::operator*() const
{
	if (m_node == nullptr)
		throw std::out_of_range("Cannot dereference KList const end iterator");

	return m_node->value;
}

KList::ConstIterator::pointer KList::ConstIterator::operator->() const
{
	return &operator*();
}

KList::ConstIterator &KList::ConstIterator::operator++()
{
	if (m_node == nullptr)
		throw std::out_of_range("Cannot increment KList const end iterator");

	m_node = m_node->next;
	return *this;
}

KList::ConstIterator KList::ConstIterator::operator++(int)
{
	ConstIterator previous = *this;
	++(*this);
	return previous;
}

bool KList::ConstIterator::operator==(const ConstIterator &other) const noexcept
{
	return m_node == other.m_node;
}

bool KList::ConstIterator::operator!=(const ConstIterator &other) const noexcept
{
	return !(*this == other);
}
