// Purpose: Check ownership, temporary arguments and aliasing for reference inputs.
// Author / Copyright: Not provided. Created: 2026-11-06.
#include "json.h"

#include <string>
#include <utility>
#include <vector>

#include "test.h"

int main()
{
	const std::string key(80, 'k');
	const std::string text(200, 't');
	json::jobject object;
	object[key] = text;
	const std::string original = object.as_string();

	// Proxies must own the key even after a temporary or named argument disappears.
	auto temporaryProxy = object[std::string(key)];
	std::string namedKey = key;
	auto namedProxy = object[namedKey];
	namedKey.assign(80, 'x');
	TEST_TRUE(temporaryProxy.as_string() == text);
	TEST_TRUE(namedProxy.as_string() == text);
	const json::jobject &constantObject = object;
	auto constantProxy = constantObject[std::string(key)];
	TEST_TRUE(constantProxy.as_string() == text);
	TEST_TRUE(constantProxy == text);
	TEST_FALSE(constantProxy != text);

	std::string encoded = json::parsing::encode_string(text.c_str());
	json::jobject::const_value ownedValue(encoded);
	encoded.assign("0");
	TEST_TRUE(ownedValue.as_string() == text);
	json::jobject::const_value temporaryValue(json::parsing::encode_string(text.c_str()));
	TEST_TRUE(temporaryValue.as_string() == text);

	std::string input = original;
	const json::jobject parsed = json::jobject::parse(input);
	TEST_TRUE(input == original);
	input.assign(input.size(), 'x');
	TEST_TRUE(parsed.as_string() == original);
	TEST_TRUE(json::jobject::parse(std::string(original)).as_string() == original);

	// Assignment and comparisons still preserve independent owned destination data.
	json::jobject copy;
	copy = object;
	copy = copy;
	TEST_TRUE(copy == object);
	TEST_FALSE(copy != object);
	object[key] = std::string(100, 'z');
	TEST_TRUE(copy.as_string() == original);
	TEST_TRUE(copy != object);

	json::jobject target;
	target["child"] = copy;
	TEST_TRUE(copy.as_string() == original);
	copy.clear();
	TEST_TRUE(target.get("child") == original);
	target["temporary"] = json::jobject::parse(R"({"x":1})");
	TEST_TRUE(target.get("temporary") == R"({"x":1})");
	const std::string beforeSelfAssignment = target.as_string();
	target["self_snapshot"] = target;
	TEST_TRUE(target.get("self_snapshot") == beforeSelfAssignment);

	const std::vector<int> integers{1, -2};
	const std::vector<unsigned int> unsignedIntegers{1, 2};
	const std::vector<long> longs{1, -2};
	const std::vector<unsigned long> unsignedLongs{1, 2};
	const std::vector<char> characters{'A', 'B'};
	const std::vector<float> floats{1.5F, 2.5F};
	const std::vector<double> doubles{1.5, 2.5};
	std::vector<std::string> strings{std::string(80, 'a'), "quoted\"value"};
	std::vector<json::jobject> objects{parsed, json::jobject::parse(R"({"x":1})")};
	target["integers"] = integers;
	target["unsigned"] = unsignedIntegers;
	target["longs"] = longs;
	target["unsigned_longs"] = unsignedLongs;
	target["characters"] = characters;
	target["floats"] = floats;
	target["doubles"] = doubles;
	target["strings"] = strings;
	target["objects"] = objects;
	TEST_TRUE(integers == std::vector<int>({1, -2}));
	TEST_TRUE(unsignedIntegers == std::vector<unsigned int>({1, 2}));
	TEST_TRUE(longs == std::vector<long>({1, -2}));
	TEST_TRUE(unsignedLongs == std::vector<unsigned long>({1, 2}));
	TEST_TRUE(characters == std::vector<char>({'A', 'B'}));
	TEST_TRUE(floats == std::vector<float>({1.5F, 2.5F}));
	TEST_TRUE(doubles == std::vector<double>({1.5, 2.5}));
	TEST_TRUE(strings[0] == std::string(80, 'a') && strings[1] == "quoted\"value");
	TEST_TRUE(objects[0].as_string() == original);
	TEST_TRUE(target.get("integers") == "[1,-2]");
	TEST_TRUE(target.get("unsigned") == "[1,2]");
	// Preserve the original %c array formatting; do not normalize it in this optimization.
	TEST_TRUE(target.get("characters") == "[A,B]");
	TEST_TRUE(target["floats"].as_array<float>() == floats);
	TEST_TRUE(target["doubles"].as_array<double>() == doubles);
	const std::string storedStrings = target.get("strings");
	const std::string storedObjects = target.get("objects");
	strings[0] = "changed";
	objects[0].clear();
	TEST_TRUE(target.get("strings") == storedStrings);
	TEST_TRUE(target.get("objects") == storedObjects);

	// The existing public insertion still copies, even with an rvalue expression.
	json::kvp entry("copy_source", "25");
	target += std::move(entry);
	TEST_TRUE(entry.first == "copy_source" && entry.second == "25");
	return 0;
}
