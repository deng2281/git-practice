// Purpose: Preserve public key ownership, length and mutation behavior across storage changes.
// Author / Copyright: Not provided. Created: 2026-11-06.
#include "json.h"

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test.h"

namespace
{
class ValueReference : public json::jobject::const_proxy
{
public:
	ValueReference(const json::jobject &source, const std::string &key)
		: json::jobject::const_proxy(source, key)
	{ }
	const std::string &raw() const { return ref(); }
};

void CheckKey(const std::string &key)
{
	json::jobject object;
	{
		std::string temporary = key;
		json::kvp entry(temporary, "123");
		object += entry;
		temporary.assign(temporary.size(), 'x');
		entry.first.assign(entry.first.size(), 'y');
	}
	TEST_TRUE(object.size() == 1 && object.has_key(key));
	TEST_TRUE(object.list_keys().at(0) == key);
	TEST_TRUE(object.get(key) == "123");
	ValueReference reference(object, key);
	TEST_TRUE(reference.raw() == "123");
	const std::string original = object.as_string();
	const std::string formatted = object.pretty();
	json::jobject copied(std::move(object));
	TEST_TRUE(object.as_string() == original && copied.as_string() == original);
	json::jobject assigned;
	assigned = std::move(object);
	TEST_TRUE(object.as_string() == original && assigned.pretty() == formatted);
	object.set(key, "456");
	TEST_TRUE(reference.raw() == "456");
	TEST_TRUE(copied.get(key) == "123" && assigned.get(key) == "123");
	assigned = assigned;
	TEST_TRUE(assigned.list_keys().at(0) == key);
	bool duplicateRejected = false;
	try { assigned += json::kvp(key, "789"); }
	catch (const json::parsing_error &error) { duplicateRejected = std::string(error.what()) == "Key conflict"; }
	TEST_TRUE(duplicateRejected && assigned.as_string() == original);
	object.remove(key);
	TEST_TRUE(object.size() == 0 && !object.has_key(key));
}
}

int main()
{
	static_assert(std::is_same<json::kvp, std::pair<std::string, std::string>>::value,
		"Public kvp must retain its original type");
	for (const std::size_t length : {1u, 15u, 16u, 18u, 21u, 22u, 23u, 24u, 31u, 32u, 1000u})
	{
		const std::string key(length, 'k');
		CheckKey(key);
		std::string input = "{\"" + key + "\":123}";
		json::jobject parsed = json::jobject::parse(input);
		input.assign(input.size(), 'x');
		TEST_TRUE(parsed.list_keys().at(0) == key && parsed.get(key) == "123");
		TEST_TRUE(json::jobject::parse(parsed.pretty()).get(key) == "123");
	}
	CheckKey(std::string("abc\0def", 7));
	CheckKey(std::string(22, 'k') + std::string("\0tail", 5));
	CheckKey("quote\"slash\\tab\t");
	CheckKey("\xE5\xAD\xA6\xE7\x94\x9F\xE5\xA7\x93\xE5\x90\x8D");

	// Exercise relocation, long/short copy assignment, and erase across both forms.
	std::vector<std::string> keys;
	json::jobject many;
	for (std::size_t index = 0; index < 160; ++index)
	{
		keys.push_back(std::string(index % 2 == 0 ? 18 : 120, 'k') + std::to_string(index));
		many.set(keys.back(), std::to_string(index));
	}
	json::jobject copied(many);
	json::jobject assigned;
	assigned.set(std::string(180, 'z'), "0");
	assigned = many;
	many.remove(static_cast<std::size_t>(0));
	many.remove(static_cast<std::size_t>(70));
	TEST_TRUE(many.size() == 158 && copied.size() == 160 && assigned.size() == 160);
	for (std::size_t index = 0; index < keys.size(); ++index)
		TEST_TRUE(copied.get(keys[index]) == std::to_string(index));
	assigned.clear();
	assigned += copied;
	TEST_TRUE(assigned.as_string() == copied.as_string());

	json::jobject binary;
	binary.set(std::string("a\0b", 3), "1");
	binary.set("a", "2");
	TEST_TRUE(binary.size() == 2 && binary.get(std::string("a\0b", 3)) == "1" && binary.get("a") == "2");
	TEST_TRUE(binary.list_keys().at(0).size() == 3);
	// Preserve the original difference: compact uses C-string escaping, pretty appends full keys.
	TEST_TRUE(binary.as_string() == R"({"a":1,"a":2})");
	TEST_TRUE(binary.pretty().find(std::string("a\0b", 3)) != std::string::npos);
	json::jobject emptyKey;
	emptyKey.set("", "1");
	TEST_TRUE(emptyKey.has_key("") && emptyKey.list_keys().at(0).empty());
	TEST_FALSE(json::jobject::tryparse(R"({"":1})", emptyKey));
	TEST_TRUE(emptyKey.get("") == "1");

	json::jobject array = json::jobject::parse("[1,2,3]");
	array += json::kvp("", "4");
	array.remove(static_cast<std::size_t>(1));
	TEST_TRUE(array.is_array() && array.list_keys().empty() && array.as_string() == "[1,3,4]");
	return 0;
}
