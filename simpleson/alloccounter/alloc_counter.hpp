// File: alloc_counter.hpp
// Author: Not provided
// Created: 2026-11-01
// Purpose: Count successful ordinary new/new[] calls and matching delete calls.
// Copyright: Not provided

#ifndef ALLOCCOUNTERDEMO_ALLOC_COUNTER_HPP
#define ALLOCCOUNTERDEMO_ALLOC_COUNTER_HPP

#include <atomic>
#include <cstddef>

class AllocCounter
{
public:
	AllocCounter() = delete;

	static void reset() noexcept;
	static void report(const char *label) noexcept;

	// Internal hooks for the replacement operators; these never allocate.
	static void recordAllocation(std::size_t size) noexcept;
	static void recordDeallocation() noexcept;

private:
	inline static std::atomic<bool> s_enabled{true};
	inline static std::atomic<std::size_t> s_allocCount{0};
	inline static std::atomic<std::size_t> s_allocBytes{0};
	inline static std::atomic<std::size_t> s_deallocCount{0};
};

#endif // ALLOCCOUNTERDEMO_ALLOC_COUNTER_HPP
