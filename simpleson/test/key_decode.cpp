// File: key_decode.cpp
// Author / Copyright: Not provided. Created: 2026-11-06.
// Purpose: Preserve decoding, ownership and error order when decoding private tokens in place.
#include "json.h"

#include <string>
#include <utility>

#include "test.h"

namespace
{
struct EscapeCase
{
	const char *encoded;
	const char *decoded;
};

void CheckKey(const std::string &encoded, const std::string &decoded)
{
	const std::string token = "\"" + encoded + "\"";
	TEST_TRUE(json::parsing::decode_string(token.c_str()) == decoded);
	std::string input = "{" + token + ":\"long_value_data_10\",\"other\":false}";
	const std::string original = input;
	json::jobject object = json::jobject::parse(input);
	TEST_TRUE(input == original);
	input.assign(input.size(), '?');
	const auto keys = object.list_keys();
	TEST_TRUE(keys.size() == 2 && keys[0] == decoded && keys[1] == "other");
	TEST_TRUE(object.get(decoded) == "\"long_value_data_10\"");
	TEST_TRUE(object.get("other") == "false");
	json::jobject copied(std::move(object));
	const std::string before = copied.as_string();
	TEST_TRUE(object.as_string() == before);
	object.set(decoded, "25");
	TEST_TRUE(copied.as_string() == before && object.get(decoded) == "25");
}

void CheckError(const char *input, const char *expected)
{
	bool failed = false;
	try { static_cast<void>(json::jobject::parse(input)); }
	catch (const json::parsing_error &error)
	{
		failed = true;
		TEST_TRUE(std::string(error.what()) == expected);
	}
	TEST_TRUE(failed);
	json::jobject output = json::jobject::parse("{\"keep\":1}");
	TEST_FALSE(json::jobject::tryparse(input, output));
	TEST_TRUE(output.as_string() == "{\"keep\":1}");
}
} // namespace

int main()
{
	const EscapeCase escapes[] = {
		{"tail", "tail"}, {"a\\\"b", "a\"b"}, {"a\\\\b", "a\\b"},
		{"a\\tb", "a\tb"}, {"a\\nb", "a\nb"}, {"a\\rb", "a\rb"},
		{"a\\fb", "a\fb"}, {"a\\bb", "a\bb"},
		// Preserve the original Unicode TODO: four following digits emit no bytes.
		{"a\\u0041b", "ab"}, {"a\\u0000b", "ab"}
	};
	for (std::size_t length : {0U, 1U, 15U, 16U, 20U, 21U, 22U, 23U, 24U, 50U, 128U, 4096U})
		for (const auto &escape : escapes)
		{
			const std::string prefix(length, 'k');
			CheckKey(prefix + escape.encoded, prefix + escape.decoded);
		}
	// These fully terminated malformed inputs have defined original error paths.
	CheckError(R"({"":1})", "Missing key");
	CheckError(R"({"\u0041":1})", "Missing key");
	CheckError(R"({"a\u0041":1,"a":2})", "Key conflict");
	CheckError(R"({"a\u0041":1,"a":})", "Input is not a valid object");
	CheckError(R"({"abc" 1})", "Input is not a valid object");
	CheckError(R"({"bad\q":1})", "Input is not a valid object");
	// Public decoder and JSON reader intentionally have different old slash rules.
	TEST_TRUE(json::parsing::decode_string(R"("a\/b")") == "a/b");
	CheckError(R"({"a\/b":1})", "Input is not a valid object");
	const auto array = json::jobject::parse(R"(["x",1e+02,-0,true,null,{"a":"b"}])");
	TEST_TRUE(array.as_string() == R"(["x",1e+02,-0,true,null,{"a":"b"}])");
	TEST_TRUE(array.get(static_cast<std::size_t>(1)) == "1e+02");
	return 0;
}
