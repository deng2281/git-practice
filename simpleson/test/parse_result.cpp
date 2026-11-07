// Purpose: Check parsed-result ownership while keeping public copies unchanged.
// Author / Copyright: Not provided. Created: 2026-11-06.
#include "json.h"

#include <string>
#include <utility>

#include "test.h"

namespace
{
json::jobject ParseLocalInput()
{
	const std::string input = "{\"" + std::string(100, 'k') + "\":\"" + std::string(10000, 'v') + "\"}";
	return json::jobject::parse(input);
}
}

int main()
{
	json::jobject object = ParseLocalInput();
	const std::string original = object.as_string();
	TEST_TRUE(object.size() == 1 && !object.is_array());
	TEST_TRUE(object[std::string(100, 'k')].as_string() == std::string(10000, 'v'));

	// Explicit std::move on a public jobject still selects the original copy
	// interface. Only the parser's private temporary is allowed to give up data.
	json::jobject copied(std::move(object));
	TEST_TRUE(object.as_string() == original && copied.as_string() == original);
	object[std::string(100, 'k')] = "changed";
	TEST_TRUE(copied.as_string() == original);
	const std::string sourceBeforeAssignment = object.as_string();
	json::jobject assigned;
	assigned = std::move(object);
	TEST_TRUE(object.as_string() == sourceBeforeAssignment);
	TEST_TRUE(assigned.as_string() == sourceBeforeAssignment);
	object = std::move(object);
	TEST_TRUE(object.as_string() == sourceBeforeAssignment);

	std::string input = R"([{"x":[1,2,{}]},"long_string_for_heap_storage",null])";
	json::jobject array = json::jobject::parse(input);
	const std::string arrayText = array.as_string();
	input.assign(input.size(), 'x');
	TEST_TRUE(array.is_array() && array.size() == 3);
	TEST_TRUE(arrayText == R"([{"x":[1,2,{}]},"long_string_for_heap_storage",null])");
	json::jobject arrayCopy(std::move(array));
	TEST_TRUE(array.is_array() && array.as_string() == arrayText);
	TEST_TRUE(arrayCopy.is_array() && arrayCopy.as_string() == arrayText);
	arrayCopy.remove(0);
	TEST_TRUE(array.size() == 3 && array.as_string() == arrayText);
	TEST_TRUE(json::jobject::parse(array.pretty(2)).as_string() == arrayText);

	json::jobject output = json::jobject::parse(R"({"keep":123})");
	const std::string beforeFailure = output.as_string();
	TEST_FALSE(json::jobject::tryparse(R"({"missing":})", output));
	TEST_TRUE(output.as_string() == beforeFailure && !output.is_array());
	TEST_FALSE(json::jobject::tryparse(R"({"same":1,"same":2})", output));
	TEST_TRUE(output.as_string() == beforeFailure);
	TEST_TRUE(json::jobject::tryparse(arrayText.c_str(), output));
	TEST_TRUE(output.is_array() && output.as_string() == arrayText);
	TEST_TRUE(json::jobject::parse("{}").as_string() == "{}");
	TEST_TRUE(json::jobject::parse("[]").is_array());
	TEST_TRUE(json::jobject::parse(arrayText + " ignored_suffix").as_string() == arrayText);
	return 0;
}
