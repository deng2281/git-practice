// Purpose: Preserve parsing/escaping/formatting behavior around capacity hints.
// Author / Copyright: Not provided. Created: 2026-11-06.
#include "json.h"

#include <string>
#include <vector>

#include "test.h"

namespace
{
bool Rejects(const char *input)
{
	try
	{
		json::jobject::parse(input);
	}
	catch (const json::parsing_error &)
	{
		return true;
	}
	return false;
}
}

int main()
{
	// Commas/brackets/quotes inside strings or nested values are not root entries.
	const std::string input = R"json({"a,b":"x,y","nested":[{"q":"\"},]\""},{"k":[1,2]}],"slash":"\\","empty":[]})json";
	const std::string suffix(100000, 's');
	const json::jobject object = json::jobject::parse(input + suffix);
	TEST_TRUE(object.size() == 4 && object.as_string() == input);
	TEST_TRUE(json::jobject::parse(object.pretty(3)).as_string() == input);
	const auto keys = object.list_keys();
	TEST_TRUE(keys == json::key_list_t({"a,b", "nested", "slash", "empty"}));
	TEST_TRUE(json::jobject::parse("{}").pretty(3) == "\t\t\t{}");
	TEST_TRUE(json::jobject::parse("[]").pretty(3) == "\t\t\t[]");

	// Capacity scans must leave the original acceptance and error rules intact.
	TEST_TRUE(json::jobject::parse(R"({"a":1,})").as_string() == R"({"a":1})");
	TEST_TRUE(json::jobject::parse("[1,]").as_string() == "[1]");
	TEST_TRUE(Rejects("[1,,2]"));
	TEST_TRUE(Rejects(R"({"x":})"));
	TEST_TRUE(Rejects(R"({"x":1,"x":2})"));
	TEST_TRUE(Rejects(R"({"x":[1,2])"));
	const auto array = json::parsing::parse_array(("[" + input + ",true,[1,2]]" + suffix).c_str());
	TEST_TRUE(array == std::vector<std::string>({input, "true", "[1,2]"}));

	// Preserve the old decoder's Unicode omission, including long escape runs.
	TEST_TRUE(json::parsing::decode_string(R"("A\u0042C")") == "AC");
	std::string unicode = "\"";
	for (int index = 0; index < 1000; ++index) unicode += "\\u0041";
	unicode += '"';
	TEST_TRUE(json::parsing::decode_string(unicode.c_str()).empty());
	TEST_TRUE(json::parsing::decode_string(("\"x\"" + suffix).c_str()) == "x");
	const std::string allEscapes = "\"\\/\b\f\n\r\t";
	TEST_TRUE(json::parsing::decode_string(json::parsing::encode_string(allEscapes.c_str()).c_str()) == allEscapes);
	for (const std::size_t length : {std::size_t(0), std::size_t(15), std::size_t(16), std::size_t(31), std::size_t(10000)})
	{
		const std::string text(length, 'x');
		TEST_TRUE(json::parsing::encode_string(text.c_str()) == "\"" + text + "\"");
		TEST_TRUE(json::parsing::decode_string(("\"" + text + "\"").c_str()) == text);
	}

	// The compact encoder uses C strings; pretty's original key path retains NUL.
	const std::string key("a\0b", 3);
	json::jobject nulKey;
	nulKey.set(key, "1");
	TEST_TRUE(nulKey.as_string() == R"({"a":1})");
	TEST_TRUE(nulKey.pretty() == "{\n\t\"" + key + "\": 1\n}");
	const std::string nestedWhitespace = "{" + std::string(100000, ' ') + "}";
	json::jobject nested;
	nested.set("child", nestedWhitespace);
	TEST_TRUE(nested.pretty() == "{\n\t\"child\": {}\n}");
	return 0;
}
