// Purpose: Emit deterministic observable behavior for byte-for-byte comparisons against the unoptimized source.
// Author / Copyright: Not provided.
#include "json.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
void Emit(const std::string &value)
{
	constexpr char DIGITS[] = "0123456789abcdef";
	std::cout << value.size() << ':';
	for (const unsigned char character : value)
		std::cout << DIGITS[character >> 4] << DIGITS[character & 15];
	std::cout << '\n';
}

std::string MakeObject(int count)
{
	std::string result = "{";
	for (int i = 0; i < count; ++i)
	{
		if (i != 0) result += ',';
		result += "\"long_field_name_" + std::to_string(i) + "\":\"long_value_data_" + std::to_string(i) + "\"";
	}
	return result + '}';
}

void Observe(const std::string &input)
{
	Emit(input);
	try
	{
		std::string ownedInput = input;
		json::jobject object = json::jobject::parse(ownedInput);
		ownedInput.assign(ownedInput.size(), 'x');
		std::cout << "accepted " << object.is_array() << ' ' << object.size() << '\n';
		Emit(object.as_string());
		Emit(object.pretty());
		Emit(object.pretty(2));
		const json::key_list_t keys = object.list_keys();
		for (size_t i = 0; i < object.size(); ++i)
		{
			const std::string raw = object.get(i);
			Emit(raw);
			const json::parsing::parse_results token = json::parsing::parse(raw.c_str());
			std::cout << static_cast<int>(token.type) << ':' << (token.remainder - raw.c_str()) << '\n';
			Emit(token.value);
			if (object.is_array()) Emit(object.array(i).as_string());
			else
			{
				Emit(keys.at(i));
				Emit(object.get(keys.at(i)));
				Emit(object[keys.at(i)].as_string());
			}
		}
		json::jobject copied(object);
		json::jobject assigned;
		assigned = copied;
		if (copied.size() != 0) copied.remove(static_cast<size_t>(0));
		Emit(copied.as_string());
		Emit(assigned.as_string());
		object.clear();
		Emit(object.as_string());
	}
	catch (const json::parsing_error &error)
	{
		std::cout << "parsing_error\n";
		Emit(error.what());
	}
	catch (const std::exception &error)
	{
		std::cout << "other_error\n";
		Emit(error.what());
	}
}

void ObserveMutations()
{
	json::jobject object;
	object["name"] = "Tom";
	object["age"] = 25;
	object["flag"].set_boolean(true);
	object["nothing"].set_null();
	object["numbers"] = std::vector<int>{1, 2, 3};
	std::cout << static_cast<int>(object["age"]) << ' ' << object["flag"].is_true()
		<< ' ' << object["nothing"].is_null() << '\n';
	Emit(object.as_string());
	object["age"] = 26;
	object.remove("name");
	Emit(object.pretty());
	json::kvp entry("entry", "\"long_string_for_ownership\"");
	object += std::move(entry);
	// The existing public insertion overload must still copy even for std::move callers.
	Emit(entry.first);
	Emit(entry.second);
	entry.second = "0";
	Emit(object.get("entry"));
	const std::string before = object.as_string();
	const bool accepted = json::jobject::tryparse(R"({"duplicate":1,"duplicate":2})", object);
	std::cout << accepted << ' ' << (before == object.as_string()) << '\n';
	json::jobject array = json::jobject::parse("[1,2]");
	array += array;
	Emit(array.as_string());
	// Preserve original formatting for escaped keys, including its known pretty() limitation.
	json::jobject escapedKeys;
	escapedKeys.set("quote\"back\\slash/line\n", "25");
	Emit(escapedKeys.as_string());
	Emit(escapedKeys.pretty());
}
}

int main()
{
	std::vector<std::string> inputs = {
		"{}", "[]", " { \"x\" : 1 } suffix", "[1,true,false,null,-1,2.5,1e3]",
		R"({"quote\"key":"a\\b\n\t\"c","unicode":"\u0041"})",
		R"({"nested":{"items":[{},[],{"value":"comma,brace}colon:"}]}})",
		R"({"slash":"a\/b"})", R"({"same":1,"same":2})", R"({"":1})",
		R"({"x" 1})", R"({"x":1 "y":2})", R"({"x":tru})", "[1,]", "{]", "true", "",
		"{\"utf8\":\"\xe5\xbc\xa0\xe4\xb8\x89\"}"
	};
	for (const int count : {0, 1, 2, 10, 15, 16, 17, 32, 33, 50, 100, 150})
	{
		inputs.push_back(MakeObject(count));
		std::string array = "[";
		for (int i = 0; i < count; ++i)
		{
			if (i != 0) array += ',';
			array += "\"long_item_value_" + std::to_string(i) + "\"";
		}
		inputs.push_back(array + ']');
	}
	const std::vector<std::string> values = {
		"0", "-17", "1.25e-3", "true", "false", "null", "\"\"", "\"plain long string value\"",
		R"("quotes\"backslash\\line\n")", "[]", "{}", R"([1,{"nested":[true,null]}])"
	};
	uint32_t seed = 0x1729U;
	for (int i = 0; i < 300; ++i)
	{
		seed = seed * 1664525U + 1013904223U;
		const size_t count = seed % 12 + 1;
		std::string object = "{";
		std::string array = "[";
		for (size_t position = 0; position < count; ++position)
		{
			seed = seed * 1664525U + 1013904223U;
			if (position != 0) { object += ','; array += ','; }
			const std::string &value = values.at(seed % values.size());
			object += "\"key_" + std::to_string(position) + "\" : " + value;
			array += value;
		}
		inputs.push_back(object + '}');
		inputs.push_back(array + ']');
	}
	for (const std::string &input : inputs) Observe(input);
	ObserveMutations();
	std::cout << "cases " << inputs.size() << " plus mutation suite\n";
	return 0;
}
