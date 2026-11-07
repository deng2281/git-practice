// Purpose: Preserve token ownership and avoid retaining oversized key buffers in short values.
// Author / Copyright: Not provided. Created: 2026-11-06.
#include "json.h"

#include <string>
#include <utility>

#include "test.h"

namespace
{
class RawValue : public json::jobject::const_proxy
{
public:
	RawValue(const json::jobject &source, const std::string &key)
		: json::jobject::const_proxy(source, key) { }
	const std::string &raw() const { return ref(); }
};

void CheckCapacity(const std::string &key, const std::string &rawValue)
{
	const json::jobject object = json::jobject::parse("{\"" + key + "\":" + rawValue + "}");
	RawValue value(object, key);
	TEST_TRUE(value.raw() == rawValue);
	// Compare with the library's own capacity for a fresh value, not an ABI constant.
	const std::string fresh(rawValue);
	TEST_TRUE(value.raw().capacity() <= fresh.capacity());
}
}

int main()
{
	CheckCapacity(std::string(4096, 'k'), "\"x\"");
	CheckCapacity(std::string(4096, 'k'), "1");
	CheckCapacity("long_field_name_10", "\"x\"");
	CheckCapacity("long_field_name_10", "25");
	CheckCapacity(std::string(50, 'k'), "\"" + std::string(36, 'v') + "\"");
	CheckCapacity("long_field_name_10", "\"long_value_data_10\"");

	std::string input = R"({"long_field_name_10":"long_value_data_10","tiny":1,"long_field_name_11":"long_value_data_11","flag":false,"items":[null,{"nested":"ok"}]})";
	json::jobject object = json::jobject::parse(input);
	input.assign(input.size(), 'x');
	TEST_TRUE(object.get("long_field_name_10") == "\"long_value_data_10\"");
	TEST_TRUE(object.get("long_field_name_11") == "\"long_value_data_11\"");
	TEST_TRUE(object.get("tiny") == "1" && object.get("flag") == "false");
	TEST_TRUE(object.get("items") == "[null,{\"nested\":\"ok\"}]");
	json::jobject copied(std::move(object));
	const std::string original = object.as_string();
	TEST_TRUE(copied.as_string() == original);
	object.set("long_field_name_10", "\"changed\"");
	TEST_TRUE(copied.as_string() == original);
	TEST_TRUE(json::jobject::parse(copied.pretty()).as_string() == original);

	json::jobject array = json::jobject::parse(R"(["long_value_data_10",1,true,null,"long_value_data_11",{"x":[1,2]}])");
	TEST_TRUE(array.get(static_cast<std::size_t>(0)) == "\"long_value_data_10\"");
	TEST_TRUE(array.get(static_cast<std::size_t>(4)) == "\"long_value_data_11\"");
	TEST_TRUE(json::jobject::parse(array.pretty()).as_string() == array.as_string());

	// A later invalid value or duplicate key still fails without changing tryparse output.
	json::jobject output = json::jobject::parse(R"({"keep":1})");
	for (const char *invalid : {R"({"long_field_name_10":"long_value_data_10","bad":})",
		R"({"duplicate":"long_value_data_10","duplicate":1})", R"(["long_value_data_10",1e+])"})
	{
		TEST_FALSE(json::jobject::tryparse(invalid, output));
		TEST_TRUE(output.as_string() == "{\"keep\":1}");
	}
	const std::string tokenInput = "  \"long_value_data_10\" tail";
	const auto token = json::parsing::parse(tokenInput.c_str());
	TEST_TRUE(token.type == json::jtype::jstring && token.value == "\"long_value_data_10\"");
	TEST_TRUE(std::string(token.remainder) == " tail");
	const auto invalid = json::parsing::parse("1e+");
	TEST_TRUE(invalid.type == json::jtype::not_valid && invalid.value.empty());
	return 0;
}
