// File: alloc_counter.cpp
// Author: Not provided
// Created: 2026-11-01
// Purpose: Implement allocation counters and global allocation replacements.
// Copyright: Not provided

#include "alloc_counter.hpp"

#include <cstdio>
#include <cstdlib>
#include <new>

void AllocCounter::reset() noexcept
{
	// Call only between experiments, with no concurrent allocations or frees.
	s_allocCount.store(0, std::memory_order_relaxed);
	s_allocBytes.store(0, std::memory_order_relaxed);
	s_deallocCount.store(0, std::memory_order_relaxed);
}

void AllocCounter::report(const char *label) noexcept
{
	const bool wasEnabled = s_enabled.exchange(false, std::memory_order_relaxed);
	const std::size_t allocCount = s_allocCount.load(std::memory_order_relaxed);
	const std::size_t allocBytes = s_allocBytes.load(std::memory_order_relaxed);
	const std::size_t deallocCount = s_deallocCount.load(std::memory_order_relaxed);

	std::printf("  [%s]\n", label != nullptr ? label : "unnamed");
	std::printf("    堆内存分配次数: %zu\n", allocCount);
	std::printf("    堆内存分配字节: %zu bytes\n", allocBytes);
	std::printf("    堆内存释放次数: %zu\n\n", deallocCount);

	s_enabled.store(wasEnabled, std::memory_order_relaxed);
}

void AllocCounter::recordAllocation(std::size_t size) noexcept
{
	if (!s_enabled.load(std::memory_order_relaxed))
		return;

	s_allocCount.fetch_add(1, std::memory_order_relaxed);
	s_allocBytes.fetch_add(size, std::memory_order_relaxed);
}

void AllocCounter::recordDeallocation() noexcept
{
	if (!s_enabled.load(std::memory_order_relaxed))
		return;

	s_deallocCount.fetch_add(1, std::memory_order_relaxed);
}

namespace
{
// malloc/free are intentional here: using new/delete would recurse.
void *AllocateMemory(std::size_t size)
{
	void *pMemory = std::malloc(size == 0 ? 1 : size);
	if (pMemory == nullptr)
		throw std::bad_alloc();

	AllocCounter::recordAllocation(size);
	return pMemory;
}

void ReleaseMemory(void *pMemory) noexcept
{
	if (pMemory == nullptr)
		return;

	AllocCounter::recordDeallocation();
	std::free(pMemory);
}
}

// Define replacements in this translation unit only, never in the header.
void *operator new(std::size_t size)
{
	return AllocateMemory(size);
}

void *operator new[](std::size_t size)
{
	return AllocateMemory(size);
}

void operator delete(void *pMemory) noexcept
{
	ReleaseMemory(pMemory);
}

void operator delete[](void *pMemory) noexcept
{
	ReleaseMemory(pMemory);
}

// C++14+ compilers may select sized deletion; forward without double counting.
void operator delete(void *pMemory, std::size_t) noexcept
{
	::operator delete(pMemory);
}

void operator delete[](void *pMemory, std::size_t) noexcept
{
	::operator delete[](pMemory);
}
