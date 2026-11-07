// Purpose: Verify internal handoff preserves bytes, ownership and public readers.
// Author / Copyright: Not provided. Created: 2026-11-06.
#include "json.h"

#include <string>
#include <utility>
#include <vector>

#include "test.h"

namespace
{
void Feed(json::reader &stream, const std::string &input)
{
	for (const char next : input)
		TEST_TRUE(stream.push(next) != json::reader::REJECTED);
	TEST_TRUE(stream.is_valid());
}

class CustomChild : public json::reader
{
public:
	virtual std::string readout() const override
	{
		return "99";
	}
};

class CustomPairChild : public json::kvp_reader
{
public:
	virtual std::string readout() const override
	{
		return "88";
	}
};

// Existing subclasses can replace their protected child reader. The new
// internal append path must still respect that child's virtual readout().
class CustomParent : public json::reader
{
public:
	void replaceChild()
	{
		delete this->sub_reader;
		this->sub_reader = new CustomChild();
		this->sub_reader->push('2');
	}

	void replacePairChild()
	{
		delete this->sub_reader;
		this->sub_reader = new CustomPairChild();
		Feed(*this->sub_reader, R"("k":2)");
	}
};
}

int main()
{
	const std::string longKey(100, 'k');
	const std::string longValue(400, 'v');
	const std::string nested = "{\"" + longKey + "\":\"" + longValue
		+ R"(","nested":[{"value":"line\nquote\"slash\\"},[true,false,null,-12.5e2],{}]})";
	json::reader stream;
	Feed(stream, nested);
	TEST_TRUE(stream.readout() == nested);
	std::string copy = stream.readout();
	copy.assign("changed");
	TEST_TRUE(stream.readout() == nested);
	TEST_TRUE(stream.type() == json::jtype::jobject);
	stream.clear();
	Feed(stream, "[1,2,{}]");
	TEST_TRUE(stream.readout() == "[1,2,{}]");

	json::kvp_reader pairReader;
	const std::string pair = "\"" + longKey + "\":" + nested;
	Feed(pairReader, pair);
	json::reader &polymorphic = pairReader;
	TEST_TRUE(polymorphic.readout() == pair);
	TEST_TRUE(polymorphic.readout() == pair);
	pairReader.clear();
	Feed(pairReader, R"("again":42)");
	TEST_TRUE(pairReader.readout() == R"("again":42)");

	CustomParent parent;
	parent.push('[');
	parent.push('1');
	parent.replaceChild();
	TEST_TRUE(parent.push(']') == json::reader::ACCEPTED);
	TEST_TRUE(parent.readout() == "[99]");
	parent.clear();
	parent.push('[');
	parent.push('1');
	parent.replacePairChild();
	TEST_TRUE(parent.push(']') == json::reader::ACCEPTED);
	TEST_TRUE(parent.readout() == "[88]");

	std::string input = nested + " trailing";
	const auto raw = json::parsing::parse(input.c_str());
	TEST_TRUE(raw.type == json::jtype::jobject && raw.value == nested);
	TEST_TRUE(std::string(raw.remainder) == " trailing");
	const json::jobject parsed = json::jobject::parse(input);
	input.assign(input.size(), 'x');
	TEST_TRUE(parsed.as_string() == nested);
	TEST_TRUE(parsed[longKey].as_string() == longValue);
	TEST_TRUE(json::jobject::parse(parsed.as_string()).pretty(2) == parsed.pretty(2));

	const std::string arrayInput = "[" + nested + R"(,"decoded\nvalue",[1,2],true,null])";
	const std::vector<std::string> array = json::parsing::parse_array(arrayInput.c_str());
	TEST_TRUE(array.size() == 5);
	TEST_TRUE(array[0] == nested && array[1] == "decoded\nvalue");
	TEST_TRUE(array[2] == "[1,2]" && array[3] == "true" && array[4] == "null");

	// Only private parser temporaries may be consumed; even std::move at the
	// public insertion call still binds to the original const-reference API.
	json::kvp entry{longKey, "\"" + longValue + "\""};
	const json::kvp originalEntry = entry;
	json::jobject object;
	object += std::move(entry);
	TEST_TRUE(entry == originalEntry);
	TEST_TRUE(object[longKey].as_string() == longValue);
	bool duplicateRejected = false;
	try
	{
		json::jobject::parse(R"({"same":1,"same":2})");
	}
	catch (const json::parsing_error &error)
	{
		duplicateRejected = std::string(error.what()) == "Key conflict";
	}
	TEST_TRUE(duplicateRejected);
	return 0;
}
