// File: main.cpp
// Author: Not provided
// Created: 2026-11-04
// Purpose: Demonstrate JSON parsing, value access, and serialization with Simpleson.
// Copyright: Not provided

#include <exception>
#include <iostream>
#include <string>

#include "json.h"

int main()
{
	try
	{
		const std::string input = R"({"name":"Tom","age":22,"graduated":false})";
		std::cout << "Input JSON:\n" << input << '\n';

		const json::jobject student = json::jobject::parse(input.c_str());
		const std::string name = student["name"];
		const int age = student["age"];
		const bool graduated = student["graduated"].is_true();
		std::cout << "\nRead values:\n"
			<< "Name: " << name << '\n'
			<< "Age: " << age << '\n'
			<< "Graduated: " << std::boolalpha << graduated << '\n';

		std::cout << "\nCompact JSON:\n" << student.as_string() << '\n';
		std::cout << "\nPretty JSON:\n" << student.pretty() << '\n';
	}
	catch (const std::exception &error)
	{
		std::cerr << "JSON demo failed: " << error.what() << '\n';
		return 1;
	}

	return 0;
}
