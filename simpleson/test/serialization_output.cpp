// Purpose: Preserve exact serialization bytes while removing temporary strings.
// Author / Copyright: Not provided. Created: 2026-11-06.
#include "json.h"

#include <string>
#include <vector>

#include "test.h"

int main()
{
	const std::string input = R"({"a":[{},[],{"x":[1,{"z":true}]}],"b":null})";
	const json::jobject object = json::jobject::parse(input);
	const std::string expected = "{\n\t\"a\": [\n\t\t{},\n\t\t[],\n\t\t{\n"
		"\t\t\t\"x\": [\n\t\t\t\t1,\n\t\t\t\t{\n\t\t\t\t\t\"z\": true\n"
		"\t\t\t\t}\n\t\t\t]\n\t\t}\n\t],\n\t\"b\": null\n}";
	TEST_TRUE(object.pretty() == expected);
	TEST_TRUE(object.pretty() == expected);
	TEST_TRUE(object.as_string() == input);
	for (unsigned int indent = 0; indent < 5; ++indent)
	{
		TEST_TRUE(json::jobject::parse(object.pretty(indent)).as_string() == input);
		TEST_TRUE(json::jobject::parse("[]").pretty(indent) == std::string(indent, '\t') + "[]");
		TEST_TRUE(json::jobject::parse("{}").pretty(indent) == std::string(indent, '\t') + "{}");
	}
	const std::string arrayInput = "[" + input + ",[[],{}],null]";
	const json::jobject array = json::jobject::parse(arrayInput);
	TEST_TRUE(json::jobject::parse(array.pretty(2)).as_string() == arrayInput);
	TEST_TRUE(array.as_string() == arrayInput);

	const std::string controls = "\"\\/\b\f\n\r\t";
	TEST_TRUE(json::parsing::encode_string(controls.c_str()) == R"("\"\\\/\b\f\n\r\t")");
	json::jobject escapedKey;
	escapedKey.set("a\"b", R"("value")");
	TEST_TRUE(escapedKey.as_string() == R"({"a\"b":"value"})");
	// Preserve the original pretty() raw-key output, rather than fixing it.
	TEST_TRUE(escapedKey.pretty() == "{\n\t\"a\"b\": \"value\"\n}");

	const std::string rawValue("raw\0tail", 8);
	json::jobject raw;
	raw.set("value", rawValue);
	TEST_TRUE(raw.as_string() == "{\"value\":" + rawValue + "}");
	TEST_TRUE(raw.pretty() == "{\n\t\"value\": " + rawValue + "\n}");
	json::jobject nested;
	nested.set("child", std::string("{}\0ignored", 10));
	TEST_TRUE(nested.pretty() == "{\n\t\"child\": {}\n}");
	TEST_TRUE(nested.get("child") == std::string("{}\0ignored", 10));

	std::vector<std::string> strings{"", "a/b", "quoted\"value", std::string("a\0b", 3)};
	json::jobject assigned;
	assigned["items"] = strings;
	TEST_TRUE(assigned.get("items") == R"(["","a\/b","quoted\"value","a"])");
	TEST_TRUE(strings[3] == std::string("a\0b", 3));
	assigned["items"] = std::vector<std::string>();
	TEST_TRUE(assigned.get("items") == "[]");
	assigned["chars"] = std::vector<char>{'A', 'B'};
	TEST_TRUE(assigned.get("chars") == "[A,B]");

	const std::string key(200, 'k');
	const std::string value(20000, 'v');
	json::jobject large;
	large[key] = value;
	TEST_TRUE(large.as_string() == "{\"" + key + "\":\"" + value + "\"}");
	TEST_TRUE(large.pretty(2) == "\t\t{\n\t\t\t\"" + key + "\": \"" + value + "\"\n\t\t}");

	nested.set("child", "{");
	bool rejected = false;
	try
	{
		nested.pretty();
	}
	catch (const json::parsing_error &error)
	{
		rejected = std::string(error.what()) == "Input is not a valid object";
	}
	TEST_TRUE(rejected);
	return 0;
}
