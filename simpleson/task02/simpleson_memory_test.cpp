// File: simpleson_memory_test.cpp
// Author: Not provided
// Created: 2026-11-04
// Purpose: Measure the original Simpleson allocation baseline for six experiments.
// Copyright: Not provided

#include "alloc_counter.hpp"

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>

#include "json.h"

namespace
{
std::string MakeJsonObject(int fieldCount)
{
	std::string input = "{";
	for (int i = 0; i < fieldCount; ++i)
	{
		if (i > 0)
			input += ",";
		// Keep the assignment's long keys and values to exceed MSVC string SSO.
		input += "\"long_field_name_" + std::to_string(i) + "\":\"long_value_data_" + std::to_string(i) + "\"";
	}
	input += "}";
	return input;
}

std::string MakeJsonArray(int elementCount)
{
	std::string input = "[";
	for (int i = 0; i < elementCount; ++i)
	{
		if (i > 0)
			input += ",";
		input += "\"long_item_value_" + std::to_string(i) + "\"";
	}
	input += "]";
	return input;
}

void MeasureParsing(const char *label, const std::string &input, std::size_t expectedSize, bool expectedArray)
{
	std::printf("--- %s (input: %zu bytes) ---\n", label, input.size());
	AllocCounter::reset();
	{
		json::jobject object = json::jobject::parse(input.c_str());
	}
	AllocCounter::report("parse + destroy");

	// Validation repeats the operation after reporting; the next reset discards its counts.
	const json::jobject verifiedObject = json::jobject::parse(input.c_str());
	if (verifiedObject.size() != expectedSize || verifiedObject.is_array() != expectedArray
		|| verifiedObject.as_string() != input)
		throw std::runtime_error("Parsed JSON does not match the input.");
}

void MeasureValueAccess()
{
	const std::string input = R"({"name_of_student":"Tom_the_great_one","age_of_student":25,"city_of_origin":"Beijing_China_Asia"})";
	// Match the assignment's non-const operator[] overload.
	json::jobject object = json::jobject::parse(input.c_str());
	std::printf("--- Experiment 5: value access ---\n");

	AllocCounter::reset();
	{
		std::string name = object["name_of_student"];
		static_cast<void>(name);
	}
	AllocCounter::report("read string: name_of_student");

	AllocCounter::reset();
	{
		int age = object["age_of_student"];
		static_cast<void>(age);
	}
	AllocCounter::report("read int: age_of_student");

	const std::string verifiedName = object["name_of_student"];
	const int verifiedAge = object["age_of_student"];
	if (verifiedName != "Tom_the_great_one" || verifiedAge != 25)
		throw std::runtime_error("Field access returned an unexpected value.");
}

void MeasureSerialization()
{
	const std::string input = MakeJsonObject(50);
	json::jobject object = json::jobject::parse(input.c_str());
	std::printf("--- Experiment 6: serialize a 50-field object ---\n");

	AllocCounter::reset();
	{
		std::string output = object.as_string();
		static_cast<void>(output);
	}
	AllocCounter::report("as_string()");

	AllocCounter::reset();
	{
		std::string output = object.pretty();
		static_cast<void>(output);
	}
	AllocCounter::report("pretty()");

	const std::string compactOutput = object.as_string();
	const std::string prettyOutput = object.pretty();
	const json::jobject compactRoundTrip = json::jobject::parse(compactOutput.c_str());
	const json::jobject prettyRoundTrip = json::jobject::parse(prettyOutput.c_str());
	if (compactRoundTrip.size() != 50 || prettyRoundTrip.size() != 50
		|| compactRoundTrip.as_string() != input || prettyRoundTrip.as_string() != input)
		throw std::runtime_error("Serialized JSON does not preserve the input.");
}
}

int main()
{
	try
	{
		std::printf("Simpleson memory baseline\n"
			"Six experiments, eight reports; inputs and validation are outside measurement intervals.\n\n");
		const std::string smallInput = R"({"name_of_student":"Tom_the_great_one","age_of_student":25})";
		MeasureParsing("Experiment 1: object, 2 fields", smallInput, 2, false);
		MeasureParsing("Experiment 2: object, 10 fields", MakeJsonObject(10), 10, false);
		MeasureParsing("Experiment 3: object, 100 fields", MakeJsonObject(100), 100, false);
		MeasureParsing("Experiment 4: array, 100 elements", MakeJsonArray(100), 100, true);
		MeasureValueAccess();
		MeasureSerialization();

		std::printf("Validation passed: parsed content, field values, and both serialization round trips.\n");
	}
	catch (const std::exception &error)
	{
		std::fprintf(stderr, "Memory experiment failed: %s\n", error.what());
		return 1;
	}

	return 0;
}
