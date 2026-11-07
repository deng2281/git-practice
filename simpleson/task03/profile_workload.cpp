// File: profile_workload.cpp
// Author: Not provided
// Created: 2026-11-05
// Purpose: Prepare, validate, and execute the assignment's profiling operations.
// Copyright: Not provided

#include "profile_workload.h"

#include <charconv>
#include <stdexcept>
#include <system_error>

namespace
{
std::string MakeObject(int fieldCount)
{
	std::string input = "{";
	for (int i = 0; i < fieldCount; ++i)
	{
		if (i > 0)
			input += ",";
		input += "\"long_field_name_" + std::to_string(i) + "\":\"long_value_data_" + std::to_string(i) + "\"";
	}
	input += "}";
	return input;
}

std::string MakeArray(int elementCount)
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
} // namespace

namespace profile
{
ProfileWorkload::ProfileWorkload()
	: m_scenario(Scenario::ParseSmall), m_input(), m_object(), m_expectedResult(0)
{
}

void ProfileWorkload::prepare(Scenario scenario)
{
	m_scenario = scenario;
	switch (scenario)
	{
	case Scenario::ParseSmall:
		m_input = R"({"name_of_student":"Tom_the_great_one","age_of_student":25})";
		m_expectedResult = 2;
		break;
	case Scenario::ParseMedium:
		m_input = MakeObject(10);
		m_expectedResult = 10;
		break;
	case Scenario::ParseLarge:
		m_input = MakeObject(100);
		m_expectedResult = 100;
		break;
	case Scenario::ParseArray:
		m_input = MakeArray(100);
		m_expectedResult = 100;
		break;
	case Scenario::ReadString:
	case Scenario::ReadInteger:
	{
		m_input = R"({"name_of_student":"Tom_the_great_one","age_of_student":25,"city_of_origin":"Beijing_China_Asia"})";
		m_object = json::jobject::parse(m_input.c_str());
		const std::string name = m_object["name_of_student"];
		const int age = m_object["age_of_student"];
		if (name != "Tom_the_great_one" || age != 25)
			throw std::runtime_error("Field preflight validation failed.");
		m_expectedResult = scenario == Scenario::ReadString ? name.size() : static_cast<std::uint64_t>(age);
		return;
	}
	case Scenario::SerializeCompact:
	case Scenario::SerializePretty:
	{
		m_input = MakeObject(50);
		m_object = json::jobject::parse(m_input.c_str());
		const std::string output = scenario == Scenario::SerializeCompact ? m_object.as_string() : m_object.pretty();
		const json::jobject verified = json::jobject::parse(output.c_str());
		if (verified.size() != 50 || verified.as_string() != m_input)
			throw std::runtime_error("Serialization preflight validation failed.");
		m_expectedResult = output.size();
		return;
	}
	default:
		throw std::invalid_argument("Unknown profiling scenario.");
	}

	// Preflight verifies content before the repeated aggregate loop.
	const json::jobject verified = json::jobject::parse(m_input.c_str());
	if (verified.size() != m_expectedResult || verified.is_array() != (scenario == Scenario::ParseArray)
		|| verified.as_string() != m_input)
		throw std::runtime_error("Parse preflight validation failed.");
}

std::uint64_t ProfileWorkload::runOnce()
{
	switch (m_scenario)
	{
	case Scenario::ParseSmall:
	case Scenario::ParseMedium:
	case Scenario::ParseLarge:
	case Scenario::ParseArray:
	{
		const json::jobject object = json::jobject::parse(m_input.c_str());
		return object.size();
	}
	case Scenario::ReadString:
	{
		const std::string name = m_object["name_of_student"];
		return name.size();
	}
	case Scenario::ReadInteger:
	{
		const int age = m_object["age_of_student"];
		return static_cast<std::uint64_t>(age);
	}
	case Scenario::SerializeCompact:
	{
		const std::string output = m_object.as_string();
		return output.size();
	}
	case Scenario::SerializePretty:
	{
		const std::string output = m_object.pretty();
		return output.size();
	}
	default:
		throw std::invalid_argument("Unknown profiling scenario.");
	}
}

std::uint64_t ProfileWorkload::expectedResult() const
{
	return m_expectedResult;
}

bool ParseIterations(std::string_view text, std::uint64_t &iterations)
{
	if (text.empty())
		return false;
	const auto result = std::from_chars(text.data(), text.data() + text.size(), iterations);
	return result.ec == std::errc() && result.ptr == text.data() + text.size()
		&& iterations > 0 && iterations <= 1000000000;
}

} // namespace profile
