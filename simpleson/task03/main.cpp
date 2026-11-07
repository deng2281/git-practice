// Purpose: Execute the complete assignment workload for CPU or heap profiling.
// Author / Copyright: Not provided. Created: 2026-11-06.
#include "profile_workload.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>

#ifdef PROFILE_WITH_MTUNER
#include <filesystem>

#include "rmem.h"
#include "rmem_entry.h"
#endif // PROFILE_WITH_MTUNER

namespace
{
std::uint64_t RunSuite(std::uint64_t rounds)
{
	constexpr profile::Scenario SCENARIOS[] = {
		profile::Scenario::ParseSmall, profile::Scenario::ParseMedium,
		profile::Scenario::ParseLarge, profile::Scenario::ParseArray,
		profile::Scenario::ReadString, profile::Scenario::ReadInteger,
		profile::Scenario::SerializeCompact, profile::Scenario::SerializePretty
	};
	profile::ProfileWorkload cases[8];
	std::uint64_t expectedPerRound = 0;
	for (size_t i = 0; i < 8; ++i)
	{
		cases[i].prepare(SCENARIOS[i]);
		expectedPerRound += cases[i].expectedResult();
	}
	std::uint64_t checksum = 0;
	for (std::uint64_t round = 0; round < rounds; ++round)
		for (size_t i = 0; i < 8; ++i)
			checksum += cases[i].runOnce();
	if (checksum != rounds * expectedPerRound)
		throw std::runtime_error("Workload checksum mismatch.");
	return checksum;
}
}

int main(int argumentCount, char *arguments[])
{
	std::uint64_t rounds = 0;
	const int expectedArguments =
#ifdef PROFILE_WITH_MTUNER
		3;
#else
		2;
#endif // PROFILE_WITH_MTUNER
	if (argumentCount != expectedArguments || !profile::ParseIterations(arguments[1], rounds))
	{
		std::cerr << "Usage: program <rounds> [existing ASCII capture directory]\n";
		return 2;
	}
#ifdef PROFILE_WITH_MTUNER
	bool captureActive = false;
#endif // PROFILE_WITH_MTUNER
	try
	{
#ifdef PROFILE_WITH_MTUNER
		const auto directory = std::filesystem::absolute(arguments[2]);
		if (!std::filesystem::is_directory(directory))
			throw std::invalid_argument("Capture directory does not exist.");
		std::string prefix = directory.string();
		for (const unsigned char character : prefix)
			if (character > 127) throw std::invalid_argument("Capture directory must be ASCII.");
		prefix += std::filesystem::path::preferred_separator;
		rmemInit(prefix.c_str());
		RMemMarker beginMarker = rmemCreateMarker("suite_begin", MARKER_COLOR_GREEN);
		RMemMarker endMarker = rmemCreateMarker("suite_end", MARKER_COLOR_RED);
		rmemHookAllocs(1, RMEM_ALLOCATOR_DEFAULT);
		captureActive = true;
		rmemSetMarker(&beginMarker);
#endif // PROFILE_WITH_MTUNER
		const auto start = std::chrono::steady_clock::now();
		// Prepared inputs/objects, preflight and all business teardown are in the measured scope.
		const std::uint64_t checksum = RunSuite(rounds);
		const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
#ifdef PROFILE_WITH_MTUNER
		rmemSetMarker(&endMarker);
		rmemUnhookAllocs();
		captureActive = false;
#endif // PROFILE_WITH_MTUNER
		// Console output is outside MTuner tracking. This duration is wall time, not VTune CPU Time.
		std::cout << "Stage: " << PROFILE_STAGE
			<< "\nScope: preparation + preflight + mixed loop + teardown"
			<< "\nOrder: 1,2,3,4,5a,5b,6a,6b\nRounds: " << rounds
			<< "\nLoop operations: " << rounds * 8 << "\nChecksum: " << checksum
			<< "\nElapsed wall seconds: " << elapsed.count() << "\nValidation passed.\n";
	}
	catch (const std::exception &error)
	{
#ifdef PROFILE_WITH_MTUNER
		if (captureActive) rmemUnhookAllocs();
#endif // PROFILE_WITH_MTUNER
		std::cerr << error.what() << '\n';
		return 1;
	}
	return 0;
}
