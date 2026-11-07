// File: profile_workload.h
// Author: Not provided
// Created: 2026-11-05
// Purpose: Share eight fixed workloads between MTuner and VTune.
// Copyright: Not provided

#ifndef WEEK02_PROFILE_WORKLOAD_H
#define WEEK02_PROFILE_WORKLOAD_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "json.h"

namespace profile
{
enum class Scenario
{
	ParseSmall, ParseMedium, ParseLarge, ParseArray,
	ReadString, ReadInteger, SerializeCompact, SerializePretty
};

class ProfileWorkload
{
public:
	ProfileWorkload();
	void prepare(Scenario scenario);
	std::uint64_t runOnce();
	std::uint64_t expectedResult() const;

private:
	ProfileWorkload(const ProfileWorkload &) = delete;
	ProfileWorkload &operator=(const ProfileWorkload &) = delete;

	Scenario m_scenario;
	std::string m_input;
	json::jobject m_object;
	std::uint64_t m_expectedResult;
};

bool ParseIterations(std::string_view text, std::uint64_t &iterations);
} // namespace profile

#endif // WEEK02_PROFILE_WORKLOAD_H
