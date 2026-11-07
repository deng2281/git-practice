// Purpose: Measure the assignment's std::string overload independently of the six experiments.
// Author / Copyright: Not provided.
#include "json.h"

#include <string>

#include "alloc_counter.hpp"

int main()
{
	const std::string input = R"({"name_of_student":"Tom_the_great_one","age_of_student":25})";
	AllocCounter::reset();
	{
		const json::jobject object = json::jobject::parse(input);
		if (object.size() != 2) return 1;
	}
	AllocCounter::report("parse(std::string)");
	AllocCounter::reset();
	{
		const json::jobject object = json::jobject::parse(input.c_str());
		if (object.size() != 2) return 1;
	}
	AllocCounter::report("parse(const char*) control");
	return 0;
}
