/*! \file json.cpp
 * \brief Simpleson source file
 */

#include "json.h"
#include <string.h>
#include <assert.h>

namespace
{
// Optimization 3: allocation hints only. These helpers never validate/reject an
// input or change the parser's cursor; the original parser still decides that.
std::size_t CountEntries(const char *input)
{
	if (*input != '{' && *input != '[') return 0;
	const char closing = *input == '{' ? '}' : ']';
	std::size_t depth = 0;
	std::size_t count = 0;
	bool quoted = false;
	bool escaped = false;
	bool hasEntry = false;
	for (const char *cursor = input + 1; *cursor != '\0'; ++cursor)
	{
		const char next = *cursor;
		if (quoted)
		{
			if (escaped) escaped = false;
			else if (next == '\\') escaped = true;
			else if (next == '"') quoted = false;
			continue;
		}
		if (next == '"')
		{
			quoted = true;
			if (depth == 0) hasEntry = true;
		}
		else if (next == '{' || next == '[')
		{
			if (depth == 0) hasEntry = true;
			++depth;
		}
		else if (next == '}' || next == ']')
		{
			if (depth == 0) return next == closing ? count + (hasEntry ? 1 : 0) : 0;
			--depth;
		}
		else if (depth == 0 && next == ',')
		{
			if (!hasEntry) return 0;
			++count;
			hasEntry = false;
		}
		else if (depth == 0 && !std::isspace(static_cast<unsigned char>(next)))
			hasEntry = true;
	}
	return 0;
}

bool AddCapacity(std::size_t amount, std::size_t limit, std::size_t &capacity)
{
	if (amount > limit - capacity) return false;
	capacity += amount;
	return true;
}

std::size_t EncodedSize(const char *input, std::size_t limit)
{
	std::size_t capacity = 2; // Opening and closing quotes.
	for (const char *cursor = input; *cursor != '\0'; ++cursor)
	{
		const char next = *cursor;
		const bool escaped = next == '"' || next == '\\' || next == '/'
			|| next == '\b' || next == '\f' || next == '\n' || next == '\r' || next == '\t';
		if (!AddCapacity(escaped ? 2 : 1, limit, capacity)) return 0;
	}
	return capacity;
}

// Optimization 4: share the original escape rules with callers that already
// own an output buffer. Returning a temporary encoded key allocated per field.
void AppendEncodedString(const char *input, std::string &output)
{
	output.push_back('"');
	while (*input != '\0')
	{
		switch (*input)
		{
		case '"':
		case '\\':
		case '/':
			output.push_back('\\');
			output.push_back(*input);
			break;
		case '\b': output.append("\\b"); break;
		case '\f': output.append("\\f"); break;
		case '\n': output.append("\\n"); break;
		case '\r': output.append("\\r"); break;
		case '\t': output.append("\\t"); break;
		default: output.push_back(*input); break;
		}
		++input;
	}
	output.push_back('"');
}

std::size_t DecodedSize(const char *input)
{
	std::size_t capacity = 0;
	for (const char *cursor = input + 1; *cursor != '\0'; ++cursor)
	{
		if (*cursor == '"') return capacity;
		if (*cursor == '\\')
		{
			++cursor;
			if (*cursor == '\0') return 0;
			if (*cursor == 'u')
			{
				// Preserve the original decoder's TODO: \u and four bytes emit
				// nothing. Reserving raw length here wastes memory on escapes.
				for (int digit = 0; digit < 4; ++digit)
					if (*++cursor == '\0') return 0;
				continue;
			}
			if (*cursor != '"' && *cursor != '\\' && *cursor != '/' && *cursor != 'b'
				&& *cursor != 'f' && *cursor != 'n' && *cursor != 'r' && *cursor != 't') return 0;
		}
		++capacity;
	}
	return 0;
}

// Optimization 2: only parsing's local reader can give up its buffer. Previously
// readout() copied it just before destruction, keeping two buffers alive at once.
// The public reader API retains its original non-destructive value semantics.
class ParseReader : public json::reader
{
public:
	// Optimization 7: the completed key token is disposable. Swap its buffer
	// into a fresh reader, then reset state and length while retaining capacity.
	void reuseBuffer(std::string &previous)
	{
		static_cast<std::string &>(*this).swap(previous);
		this->clear();
	}
	std::size_t bufferCapacity() const { return this->capacity(); }
	void takeValue(std::string &output)
	{
		output = std::move(static_cast<std::string &>(*this));
	}

private:
	virtual void appendReadout(std::string &output) const override
	{
		output.append(static_cast<const std::string &>(*this));
	}
};

class ParsePairReader : public json::kvp_reader
{
private:
	virtual void appendReadout(std::string &output) const override
	{
		this->appendPairReadout(output);
	}
};


// Optimization 7: public and internal parsing share the same reader driver.
void ReadTokenContents(const char *input, const char *index, ParseReader &stream,
	json::parsing::parse_results &result)
{
	// Keep the original push/validation/cursor order, including the input test.
	while (*input != '\0' && stream.push(*index) != json::reader::REJECTED) ++index;
	if (stream.is_valid())
	{
		result.type = stream.type();
		stream.takeValue(result.value);
	}
	result.remainder = index;
}
}

/*! \brief Checks for an empty string
 * 
 * @param str The string to check
 * @return True if the string is empty, false if the string is not empty
 * @warning The string must be null-terminated for this macro to work
 */
#define EMPTY_STRING(str) (*str == '\0')

/*! \brief Moves a pointer to the first character that is not white space
 *
 * @param str The pointer to move
 */
#define SKIP_WHITE_SPACE(str) { const char *next = json::parsing::tlws(str); str = next; }

/*! \brief Determines if the end character of serialized JSON is encountered
 * 
 * @param obj The JSON object or array that is being written to
 * @param index The pointer to the character to be checked
 */
#define END_CHARACTER_ENCOUNTERED(obj, index) (obj.is_array() ? *index == ']' : *index == '}')

/*! \brief Determines if the supplied character is a digit
 *
 * @param input The character to be tested
 */
#define IS_DIGIT(input) (input >= '0' && input <= '9')

/*! \brief Format used for integer to string conversion */
const char * INT_FORMAT = "%i";

/*! \brief Format used for unsigned integer to string conversion */
const char * UINT_FORMAT = "%u";

/*! \brief Format used for long integer to string conversion */
const char * LONG_FORMAT = "%li";

/*! \brief Format used for unsigned long integer to string conversion */
const char * ULONG_FORMAT = "%lu";

/*! \brief Format used for character to string conversion */
const char * CHAR_FORMAT = "%c";

/*! \brief Format used for floating-point number to string conversion */
const char * FLOAT_FORMAT = "%f";

/*! \brief Format used for double floating-opint number to string conversion */
const char * DOUBLE_FORMAT = "%lf";

const char* json::parsing::tlws(const char *input)
{
    const char *output = input;
    while(!EMPTY_STRING(output) && std::isspace(*output)) output++;
    return output;
}

json::jtype::jtype json::jtype::peek(const char input)
{
    switch (input)
    {
    case '[':
        return json::jtype::jarray;
    case '"':
        return json::jtype::jstring;
    case '{':
        return json::jtype::jobject;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return json::jtype::jnumber;
    case 't':
    case 'f':
        return json::jtype::jbool;
    case 'n':
        return json::jtype::jnull;
    default:
        return json::jtype::not_valid;
    }
}

json::jtype::jtype json::jtype::detect(const char *input)
{
    const char *start = json::parsing::tlws(input);
    return json::jtype::peek(*start);
}

void json::reader::clear()
{
    std::string::clear(); 
    if(this->sub_reader != NULL) {
        delete this->sub_reader;
        this->sub_reader = NULL;
    }
    this->read_state = 0;
}

void json::reader::appendReadout(std::string &output) const
{
	// Preserve virtual readout() for user-defined child readers. Only our known
	// internal readers bypass the temporary through their private overrides.
	output.append(this->readout());
}

json::reader::push_result json::reader::push(const char next)
{
    // Check for opening whitespace
    if(this->length() == 0 && std::isspace(next)) return reader::ACCEPTED;

    // Get the type
    const json::jtype::jtype type = json::jtype::peek(this->length() > 0 ? this->front() : next);

    // Store the return
    reader::push_result result = reader::REJECTED;

    #if DEBUG
    const size_t start_length = this->length();
    #endif

    switch(type)
    {
    case json::jtype::jarray:
        result = this->push_array(next);
        break;
    case json::jtype::jbool:
        result = this->push_boolean(next);
        assert(result != WHITESPACE);
        break;
    case json::jtype::jnull:
        result = this->push_null(next);
        assert(result != WHITESPACE);
        break;
    case json::jtype::jnumber:
        result = this->push_number(next);
        assert(result != WHITESPACE);
        break;
    case json::jtype::jobject:
        result = this->push_object(next);
        break;
    case json::jtype::jstring:
        result = this->push_string(next);
        assert(result != WHITESPACE);
        break;
    case json::jtype::not_valid:
        result = reader::REJECTED;
        break;
    }

    // Verify the expected length change
    #if DEBUG
    if(result == ACCEPTED) assert(this->length() - start_length == 1);
    else assert(this->length() == start_length);
    #endif

    // Return the result
    return result;
}

bool json::reader::is_valid() const
{
    switch (this->type())
    {
    case jtype::jarray:
        return this->get_state<array_reader_enum>() == ARRAY_CLOSED;
    case jtype::jbool:
        if(this->length() < 4) return false;
        if(this->length() == 4 && *this == "true") return true;
        if(this->length() == 5 && *this == "false") return true;
        return false;
    case jtype::jnull:
        return (this->length() == 4 && *this == "null");
    case jtype::jnumber:
        switch (this->get_state<number_reader_enum>())
        {
        case NUMBER_ZERO:
        case NUMBER_INTEGER_DIGITS:
        case NUMBER_FRACTION_DIGITS:
        case NUMBER_EXPONENT_DIGITS:
            return true;
        default:
            return false;
        }
    case jtype::jobject:
        return this->get_state<object_reader_enum>() == OBJECT_CLOSED;
    case jtype::jstring:
        return this->get_state<string_reader_enum>() == STRING_CLOSED;
    case jtype::not_valid:
        return false;
    }
    throw std::logic_error("Unexpected return");
}

bool is_control_character(const char input)
{
    switch (input)
    {
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
    case '"':
    case '\\':
        return true;
    default:
        return false;
    }
}

bool is_hex_digit(const char input)
{
    return IS_DIGIT(input) || (input >= 'a' && input <= 'f') || (input >= 'A' && input <= 'F');
}

json::reader::push_result json::reader::push_string(const char next)
{
    const string_reader_enum state = this->get_state<string_reader_enum>();
    switch (state)
    {
    case STRING_EMPTY:
        assert(this->length() == 0);
        if(next == '"') {
            assert(this->length() == 0);
            this->push_back(next);
            this->set_state(STRING_OPENING_QUOTE);
            return ACCEPTED;
        }
        return REJECTED;
    case STRING_OPENING_QUOTE:
        assert(this->length() == 1);
        this->set_state(STRING_OPEN);
        // Fall through deliberate
    case STRING_OPEN:
        assert(this->length() > 0);
        switch (next)
        {
        case '\\':
            this->set_state(STRING_ESCAPED);
            break;
        case '"':
            this->set_state(STRING_CLOSED);
            break;
        default:
            // No state change
            break;
        }
        this->push_back(next);
        return ACCEPTED;
    case STRING_ESCAPED:
        if(is_control_character(next)) {
            this->set_state(STRING_OPEN);
            this->push_back(next);
            return ACCEPTED;
        } else if(next == 'u') {
            this->set_state(STRING_CODE_POINT_START);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case STRING_CODE_POINT_START:
        assert(this->back() == 'u');
        if(!is_hex_digit(next)) return REJECTED;
        this->push_back(next);
        this->set_state(STRING_CODE_POINT_1);
        return ACCEPTED;
    case STRING_CODE_POINT_1:
        assert(is_hex_digit(this->back()));
        if(!is_hex_digit(next)) return REJECTED;
        this->push_back(next);
        this->set_state(STRING_CODE_POINT_2);
        return ACCEPTED;
    case STRING_CODE_POINT_2:
        assert(is_hex_digit(this->back()));
        if(!is_hex_digit(next)) return REJECTED;
        this->push_back(next);
        this->set_state(STRING_CODE_POINT_3);
        return ACCEPTED;
    case STRING_CODE_POINT_3:
        assert(is_hex_digit(this->back()));
        if(!is_hex_digit(next)) return REJECTED;
        this->push_back(next);
        this->set_state(STRING_OPEN);
        return ACCEPTED;
    case STRING_CLOSED:
        return REJECTED;
    }
    throw std::logic_error("Unexpected return");
}

json::reader::push_result json::reader::push_array(const char next)
{
    const array_reader_enum state = this->get_state<array_reader_enum>();

    switch (state)
    {
    case ARRAY_EMPTY:
        assert(this->sub_reader == NULL);
        if(next == '[') {
            this->set_state(ARRAY_OPEN_BRACKET);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case ARRAY_OPEN_BRACKET:
        assert(this->sub_reader == NULL);
        if(std::isspace(next)) return WHITESPACE;
        if(next == ']') {
            this->set_state(ARRAY_CLOSED);
            this->push_back(next);
            return ACCEPTED;
        }
        begin_reading_value:
        if(json::jtype::peek(next) == json::jtype::not_valid) return REJECTED;
        this->sub_reader = new ParseReader();
        this->set_state(ARRAY_READING_VALUE);
        // Fall-through deliberate
    case ARRAY_READING_VALUE:
        assert(this->sub_reader != NULL);
        if(this->sub_reader->is_valid() && std::isspace(next)) return WHITESPACE;
        switch (this->sub_reader->push(next))
        {
        case ACCEPTED:
            return ACCEPTED;
        case WHITESPACE:
            return WHITESPACE;
        case REJECTED:
            switch (next)
            {
            case ']':
                if(!this->sub_reader->is_valid()) return REJECTED;
                this->sub_reader->appendReadout(*this);
                delete this->sub_reader;
                this->sub_reader = NULL;
                this->push_back(next);
                this->set_state(ARRAY_CLOSED);
                return ACCEPTED;
            case ',':
                if(!this->sub_reader->is_valid()) return REJECTED;
                this->sub_reader->appendReadout(*this);
                delete this->sub_reader;
                this->sub_reader = NULL;
                this->push_back(next);
                this->set_state(ARRAY_AWAITING_NEXT_LINE);
                return ACCEPTED;
            default:
                return REJECTED;
            }
        }
        // This point should not be reached
        break;
    case ARRAY_AWAITING_NEXT_LINE:
        if(std::isspace(next)) return WHITESPACE;
        goto begin_reading_value;
    case ARRAY_CLOSED:
        return REJECTED;
    }
    throw std::logic_error("Unexpected return");
}

json::reader::push_result json::reader::push_object(const char next)
{
    const object_reader_enum state = this->get_state<object_reader_enum>();

    switch (state)
    {
    case OBJECT_EMPTY:
        assert(this->sub_reader == NULL);
        if(next == '{') {
            this->set_state(OBJECT_OPEN_BRACE);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case OBJECT_OPEN_BRACE:
        assert(this->sub_reader == NULL);
        if(next == '}') {
            this->set_state(OBJECT_CLOSED);
            this->push_back(next);
            return ACCEPTED;
        }
        // Fall-through deliberate
    case OBJECT_AWAITING_NEXT_LINE:
        if(std::isspace(next)) return WHITESPACE;
        if(next != '"') return REJECTED;
        this->sub_reader = new ParsePairReader();
        #if DEBUG
        assert(
        #endif
        this->sub_reader->push(next)
        #if DEBUG
        == ACCEPTED);
        #else
        ;
        #endif
        this->set_state(OBJECT_READING_ENTRY);
        return ACCEPTED;
    case OBJECT_READING_ENTRY:
        assert(this->sub_reader != NULL);
        switch (this->sub_reader->push(next))
        {
        case ACCEPTED:
            return ACCEPTED;
        case WHITESPACE:
            return WHITESPACE;
        case REJECTED:
            if(!this->sub_reader->is_valid()) return REJECTED;
            if(std::isspace(next)) return WHITESPACE;
            switch (next)
            {
            case '}':
                this->sub_reader->appendReadout(*this);
                delete this->sub_reader;
                this->sub_reader = NULL;
                this->push_back(next);
                this->set_state(OBJECT_CLOSED);
                return ACCEPTED;
            case ',':
                this->sub_reader->appendReadout(*this);
                delete this->sub_reader;
                this->sub_reader = NULL;
                this->push_back(next);
                this->set_state(OBJECT_AWAITING_NEXT_LINE);
                return ACCEPTED;
            default:
                return REJECTED;
            }
        }
        // This point should never be reached
        break;
    case OBJECT_CLOSED:
        return REJECTED;
    }
    throw std::logic_error("Unexpected return");
}

json::reader::push_result json::reader::push_number(const char next)
{
    const number_reader_enum state = this->get_state<number_reader_enum>();
    switch (state)
    {
    case NUMBER_EMPTY:
        assert(this->length() == 0);
        if(next == '-') {
            this->set_state(NUMBER_OPEN_NEGATIVE);
            this->push_back(next);
            return ACCEPTED;
        } else if(IS_DIGIT(next)) {
            this->set_state(next == '0' ? NUMBER_ZERO : NUMBER_INTEGER_DIGITS);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case NUMBER_OPEN_NEGATIVE:
        if(IS_DIGIT(next)) {
            this->set_state(next == '0' ? NUMBER_ZERO : NUMBER_INTEGER_DIGITS);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case NUMBER_INTEGER_DIGITS:
        assert(IS_DIGIT(this->back()));
        if(IS_DIGIT(next)) {
            this->push_back(next);
            return ACCEPTED;
        }
        // Fall-through deliberate
    case NUMBER_ZERO:
        switch (next)
        {
        case '.':
            this->set_state(NUMBER_DECIMAL);
            this->push_back(next);
            return ACCEPTED;
        case 'e':
        case 'E':
            this->set_state(NUMBER_EXPONENT);
            this->push_back(next);
            return ACCEPTED;
        default:
            return REJECTED;
        }
    case NUMBER_DECIMAL:
        assert(this->back() == '.');
        if(IS_DIGIT(next)) {
            this->set_state(NUMBER_FRACTION_DIGITS);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case NUMBER_FRACTION_DIGITS:
        assert(IS_DIGIT(this->back()));
        if(IS_DIGIT(next)) {
            this->push_back(next);
            return ACCEPTED;
        } else if(next == 'e' || next == 'E') {
            this->set_state(NUMBER_EXPONENT);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case NUMBER_EXPONENT:
        assert(this->back() == 'e' || this->back() == 'E');
        if(next == '+' || next == '-') {
            this->set_state(NUMBER_EXPONENT_SIGN);
            this->push_back(next);
            return ACCEPTED;
        }
        // Fall-through deliberate
    case NUMBER_EXPONENT_SIGN:
    case NUMBER_EXPONENT_DIGITS:
        if(IS_DIGIT(next)) {
            this->set_state(NUMBER_EXPONENT_DIGITS);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    }
    throw std::logic_error("Unexpected return");
}

json::reader::push_result json::reader::push_boolean(const char next)
{
    const char *str_true = "true";
    const char *str_false = "false";
    const char *str = NULL;

    if(this->length() == 0) {
        switch (next)
        {
        case 't':
        case 'f':
            this->push_back(next);
            return ACCEPTED;
        default:
            return REJECTED;
        }
    }

    // Determine which string to use
    switch (this->at(0))
    {
    case 't':
        str = str_true;
        break;
    case 'f':
        str = str_false;
        break;
    default:
        throw json::parsing_error("Unexpected state");
    }
    assert(str == str_true || str == str_false);

    // Push the value
    if(this->length() < strlen(str) && str[this->length()] == next) {
        this->push_back(next);
        return ACCEPTED;
    }
    return REJECTED;
}

json::reader::push_result json::reader::push_null(const char next)
{    
    switch (this->length())
    {
    case 0:
        if(next == 'n') {
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case 1:
        if(next == 'u') {
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case 2:
    case 3:
        if(next == 'l') {
            this->push_back(next);
            return ACCEPTED;
        }
        // Fall through
    case 4:
        return REJECTED;
    default:
        throw json::parsing_error("Unexpected state");
    }
}

json::reader::push_result json::kvp_reader::push(const char next)
{
    if(this->_key.length() == 0) {
        if(std::isspace(next)) return WHITESPACE;
        if(next == '"') {
            this->_key.push(next);
            assert(this->_key.type() == json::jtype::jstring);
            assert(this->_key.length() == 1);
            return ACCEPTED;
        }
        return REJECTED;
    } else if (!this->_key.is_valid()) {
        return this->_key.push(next);
    }

    // At this point the key should be valid
    assert(this->_key.is_valid());

    if(!this->_colon_read) {
        if(std::isspace(next)) return WHITESPACE;
        if(next == ':') {
            this->_colon_read = true;
            return ACCEPTED;
        }
        return REJECTED;
    }

    // At this point the colon should be read
    assert(this->_colon_read);

    // Check for a fresh start
    if(reader::length() == 0 && std::isspace(next))
    {
        assert(reader::get_state<char>() == 0);
        return WHITESPACE;
    }
    return reader::push(next);
}

std::string json::kvp_reader::readout() const
{
    return this->_key.readout() + ":" + reader::readout();
}

void json::kvp_reader::appendPairReadout(std::string &output) const
{
	// Optimization 2: the old readout() built key/value copies and a joined
	// temporary before the parent copied it again. Preserve the same bytes.
	output.append(static_cast<const std::string &>(this->_key));
	output.push_back(':');
	// Read only this value buffer; virtual readout() here would repeat the key.
	output.append(static_cast<const std::string &>(*this));
}

std::string json::parsing::read_digits(const char *input)
{
    // Trim leading white space
    const char *index = json::parsing::tlws(input);

    // Initialize the result
    std::string result;

    // Loop until all digits are read
    while (
        !EMPTY_STRING(index) &&
        (
            *index == '0' ||
            *index == '1' ||
            *index == '2' ||
            *index == '3' ||
            *index == '4' ||
            *index == '5' ||
            *index == '6' ||
            *index == '7' ||
            *index == '8' ||
            *index == '9'
            )
        )
    {
        result += *index;
        index++;
    }

    // Return the result
    return result;
}

namespace
{
// Optimization 8: decoding previously allocated an intermediate key string.
// A shared emitter keeps public decoding and the parser's owned-token decoding
// on the original state machine, including Unicode TODO/errors/stopping rules.
template<typename Emit>
void DecodeBytes(const char *input, Emit &emit)
{
	const char *index = input;
	if (*index != '"') throw json::parsing_error("Expecting opening quote");
	index++;
	bool escaped = false;
	// Loop until the end quote is found
	while(!(!escaped && *index == '"'))
	{
		if(escaped)
		{
			switch (*index)
			{
			case '"':
			case '\\':
			case '/':
				emit(*index);
				break;
			case 'b':
				emit('\b');
				break;
			case 'f':
				emit('\f');
				break;
			case 'n':
				emit('\n');
				break;
			case 'r':
				emit('\r');
				break;
			case 't':
				emit('\t');
				break;
			case 'u':
				// #todo Unicode support
				index += 4;
				break;
			default:
				throw json::parsing_error("Expected control character");
			}
			escaped = false;
		} else if(*index == '\\') {
			escaped = true;
		} else {
			emit(*index);
		}
		index++;
	}
}

void DecodeKeyInPlace(std::string &token)
{
	std::size_t written = 0;
	auto emit = [&](char next) { token[written++] = next; };
	// Optimization 8: this validated token is private, never the caller input.
	// Every write precedes the unread input. Resize only after decoding, so
	// neither character storage nor the reader cursor changes during the loop.
	DecodeBytes(token.c_str(), emit);
	token.resize(written);
}
} // namespace

std::string json::parsing::decode_string(const char *input)
{
	std::string result;
	if (*input != '"') throw json::parsing_error("Expecting opening quote");
	const std::size_t capacity = DecodedSize(input);
	if (capacity > result.capacity() && capacity <= result.max_size()) result.reserve(capacity);
	auto emit = [&](char next) { result += next; };
	DecodeBytes(input, emit);
	return result;
}

std::string json::parsing::encode_string(const char *input)
{
	std::string result;
	// Optimization 3: count the original escape rules before allocating output.
	const std::size_t capacity = EncodedSize(input, result.max_size());
	if (capacity > result.capacity()) result.reserve(capacity);

	AppendEncodedString(input, result);
    return result;
}

json::parsing::parse_results json::parsing::parse(const char *input)
{
    // Strip white space
    const char *index = json::parsing::tlws(input);

    // Validate input
    if (EMPTY_STRING(index)) throw json::parsing_error("Input was only whitespace");

    // Initialize the output
    json::parsing::parse_results result;
    result.type = json::jtype::not_valid;

    // Initialize the reader
    ParseReader stream;

	ReadTokenContents(input, index, stream, result);

    return result;
}

std::vector<std::string> json::parsing::parse_array(const char *input)
{
    // Initalize the result
    std::vector<std::string> result;

    const char *index = json::parsing::tlws(input);
    if (*index != '[') throw json::parsing_error("Input was not an array");
	// Optimization 3: avoid repeated vector growth; nested/string commas do not
	// count as entries, and any text following this array is excluded.
	const std::size_t entryCount = CountEntries(index);
	if (entryCount <= result.max_size()) result.reserve(entryCount);
    index++;
    SKIP_WHITE_SPACE(index);
    if (*index == ']')
    {
        return result;
    }
    const char error[] = "Input was not properly formated";
    while (!EMPTY_STRING(index))
    {
        SKIP_WHITE_SPACE(index);
        json::parsing::parse_results parse_results = json::parsing::parse(index);
        if (parse_results.type == json::jtype::not_valid) throw json::parsing_error(error);
        if(parse_results.type == json::jtype::jstring) {
            result.push_back(json::parsing::decode_string(parse_results.value.c_str()));
        } else {
			// Optimization 2: this internal result is not read again after insertion.
            result.push_back(std::move(parse_results.value));
        }
        index = json::parsing::tlws(parse_results.remainder);
        if (*index == ']') break;
        if (*index == ',') index++;
    }
    if (*index != ']') throw json::parsing_error(error);
    index++;
    return result;
}

json::jobject::entry::operator int() const { return this->get_number<int>(INT_FORMAT); }
json::jobject::entry::operator unsigned int() const { return this->get_number<unsigned int>(UINT_FORMAT); }
json::jobject::entry::operator long() const { return this->get_number<long>(LONG_FORMAT); }
json::jobject::entry::operator unsigned long() const { return this->get_number<unsigned long>(ULONG_FORMAT); }
json::jobject::entry::operator char() const { return this->get_number<char>(CHAR_FORMAT); }
json::jobject::entry::operator float() const { return this->get_number<float>(FLOAT_FORMAT); }
json::jobject::entry::operator double() const { return this->get_number<double>(DOUBLE_FORMAT); }

json::jobject::entry::operator std::vector<int>() const { return this->get_number_array<int>(INT_FORMAT); }
json::jobject::entry::operator std::vector<unsigned int>() const { return this->get_number_array<unsigned int>(UINT_FORMAT); }
json::jobject::entry::operator std::vector<long>() const { return this->get_number_array<long>(LONG_FORMAT); }
json::jobject::entry::operator std::vector<unsigned long>() const { return this->get_number_array<unsigned long>(ULONG_FORMAT); }
json::jobject::entry::operator std::vector<char>() const { return this->get_number_array<char>(CHAR_FORMAT); }
json::jobject::entry::operator std::vector<float>() const { return this->get_number_array<float>(FLOAT_FORMAT); }
json::jobject::entry::operator std::vector<double>() const { return this->get_number_array<double>(DOUBLE_FORMAT); }

void json::jobject::proxy::set_array(const std::vector<std::string> &values, const bool wrap)
{
    std::string value = "[";
    for (size_t i = 0; i < values.size(); i++)
    {
		// Optimization 4: append each component, avoiding encoded/comma copies.
		if (i != 0) value.push_back(',');
		if (wrap) AppendEncodedString(values[i].c_str(), value);
		else value.append(values[i]);
    }
    value += "]";
    this->sink.set(key, value);
}

json::jobject::StoredKey::StoredKey(const std::string &source)
{
	if (source.size() <= INLINE_CAPACITY) assignSmall(source);
	else m_large = std::make_unique<std::string>(source);
}

json::jobject::StoredKey::StoredKey(std::string &&source)
{
	if (source.size() <= INLINE_CAPACITY) assignSmall(source);
	else m_large = std::make_unique<std::string>(std::move(source));
}

json::jobject::StoredKey::StoredKey(const StoredKey &other)
	: m_inline(other.m_inline)
	, m_length(other.m_length)
	, m_large(other.m_large ? std::make_unique<std::string>(*other.m_large) : nullptr)
{ }

json::jobject::StoredKey &json::jobject::StoredKey::operator=(const StoredKey &other)
{
	// Allocate the independent long-key copy before replacing existing storage.
	StoredKey replacement(other);
	*this = std::move(replacement);
	return *this;
}

void json::jobject::StoredKey::assignSmall(std::string_view source)
{
	std::char_traits<char>::copy(m_inline.data(), source.data(), source.size());
	m_inline[source.size()] = '\0';
	m_length = static_cast<unsigned char>(source.size());
}

void json::jobject::validateEntry(std::string_view key) const
{
	// Optimization 6: compare the original byte length without constructing a
	// temporary std::string. Preserve duplicate/array/empty-key error order.
	if (!this->array_flag)
		for (const StoredEntry &entry : this->data)
			if (entry.first == key) throw json::parsing_error("Key conflict");
	if (this->array_flag && !key.empty()) throw json::parsing_error("Array cannot have key");
	if (!this->array_flag && key.empty()) throw json::parsing_error("Missing key");
}

namespace
{
// Optimization 7: reuse only a complete string token with a compatible size.
// A long key must not donate an oversized buffer to an inline/short value.
// This scan is only a capacity hint; the original reader validates all bytes.
bool CanReuseTokenBuffer(const char *input, std::size_t capacity, std::size_t inlineCapacity)
{
	if (*input != '"' || capacity <= inlineCapacity) return false;
	bool escaped = false;
	for (const char *cursor = input + 1; *cursor != '\0'; ++cursor)
	{
		if (escaped) escaped = false;
		else if (*cursor == '\\') escaped = true;
		else if (*cursor == '"')
		{
			const std::size_t length = static_cast<std::size_t>(cursor - input) + 1;
			return length > inlineCapacity && capacity >= length && capacity - length <= inlineCapacity;
		}
	}
	return false;
}

// Optimization 7: write into a reusable internal token instead of constructing
// and returning another parse_results for every key/value. No borrowed input.
void ReadTokenInto(const char *input, json::parsing::parse_results &result)
{
	const char *index = json::parsing::tlws(input);
	if (EMPTY_STRING(index)) throw json::parsing_error("Input was only whitespace");
	result.type = json::jtype::not_valid;
	ParseReader stream;
	if (CanReuseTokenBuffer(index, result.value.capacity(), stream.bufferCapacity()))
		stream.reuseBuffer(result.value);
	else result.value.clear();
	ReadTokenContents(input, index, stream, result);
}
}

json::jobject json::jobject::parse(const char *input)
{
    const char error[] = "Input is not a valid object";
    const char *index = json::parsing::tlws(input);
    json::jobject result;
	// Optimization 2: the unused reader here allocated a Debug string proxy.
    switch (*index)
    {
    case '{':
        // Result is already an object
        break;
    case '[':
        result = json::jobject(true);
        break;
    default:
        throw json::parsing_error(error);
        break;
    }
	// Optimization 3: the old growing vector repeatedly moved all existing
	// entries and briefly held old/new buffers. This scan is only a size hint.
	const std::size_t entryCount = CountEntries(index);
	if (entryCount <= result.data.max_size()) result.data.reserve(entryCount);
    index++;
    SKIP_WHITE_SPACE(index);
    if (EMPTY_STRING(index)) throw json::parsing_error(error);

	// Optimization 7: one owned token for this call; each stored value takes
	// its buffer before the token is reused for the next key.
    json::parsing::parse_results token;
    while (!EMPTY_STRING(index) && !END_CHARACTER_ENCOUNTERED(result, index))
    {
		// Optimization 8: the old kvp created two disposable strings per entry.
		// Decode into the owned token and construct only the final stored value.
		StoredKey key;

        if(!result.is_array()) {
            ReadTokenInto(index, token);
            if (token.type != json::jtype::jstring || token.value == "") throw json::parsing_error(error);
            DecodeKeyInPlace(token.value);
			// Copy only long keys into right-sized independent ownership; do not
			// retain oversized escaped-token capacity in the stored key.
			key = StoredKey(token.value);
            index = token.remainder;

            // Get value
            SKIP_WHITE_SPACE(index);
            if (*index != ':') throw json::parsing_error(error);
            index++;
        }

        SKIP_WHITE_SPACE(index);
        ReadTokenInto(index, token);
        if (token.type == json::jtype::not_valid) throw json::parsing_error(error);
        index = token.remainder;

        // Clean up
        SKIP_WHITE_SPACE(index);
        if (*index != ',' && !END_CHARACTER_ENCOUNTERED(result, index)) throw json::parsing_error(error);
        if (*index == ',') index++;
		// Keep validation at the original point, including error order/text.
		// The key/token are private temporaries; public += still copies its input.
		result.validateEntry(key.view());
		result.data.emplace_back(std::move(key), std::move(token.value));

    }
    if (EMPTY_STRING(index) || !END_CHARACTER_ENCOUNTERED(result, index)) throw json::parsing_error(error);
    index++;
	// Optimization 5: the named result was deep-copied here in Debug when NRVO
	// was not applied, briefly keeping two full objects alive. Return a prvalue
	// with the owned vector transferred through a private constructor instead.
	// C++17 constructs this return value directly; std::move(result) alone would
	// still select the unchanged public copy constructor, so do not use that.
	return jobject(std::move(result.data), result.array_flag);
}

json::key_list_t json::jobject::list_keys() const
{
    // Initialize the result
    key_list_t result;

    // Return an empty list if the object is an array
    if(this->is_array()) return result;
	// Optimization 3: this result's complete element count is already known.
	result.reserve(this->data.size());

    for(size_t i = 0; i < this->data.size(); i++)
    {
        result.emplace_back(this->data.at(i).first.c_str(), this->data.at(i).first.size());
    }
    return result;
}

void json::jobject::set(const std::string &key, const std::string &value)
{
    if(this->array_flag) throw json::invalid_key(key);
    for (size_t i = 0; i < this->size(); i++)
    {
        if (this->data.at(i).first == key)
        {
            this->data.at(i).second = value;
            return;
        }
    }
    kvp entry;
    entry.first = key;
    entry.second = value;
    this->data.emplace_back(entry.first, entry.second);
}

void json::jobject::remove(const std::string &key)
{
    for (size_t i = 0; i < this->size(); i++)
    {
        if (this->data.at(i).first == key)
        {
            this->remove(i);
        }
    }
}

std::size_t json::jobject::outputCapacity(bool formatted, unsigned int indent, std::size_t limit) const
{
	std::size_t capacity = 2;
	if (formatted)
	{
		if (!AddCapacity(indent, limit, capacity)) return 0;
		if (!this->data.empty() && !AddCapacity(indent, limit, capacity)) return 0;
	}
	for (std::size_t index = 0; index < this->data.size(); ++index)
	{
		const StoredEntry &entry = this->data[index];
		if (formatted)
		{
			// Include indentation, separators and the original unescaped key.
			if (!AddCapacity(indent, limit, capacity)
				|| !AddCapacity(this->array_flag ? 3 : 7, limit, capacity)) return 0;
			if (!this->array_flag && !AddCapacity(entry.first.size(), limit, capacity)) return 0;
			const jtype::jtype type = jtype::peek(*entry.second.c_str());
			const bool nested = type == jtype::jarray || type == jtype::jobject;
			// A raw nested value may contain whitespace that pretty() removes.
			// Reserve only two brackets here; raw length is not a safe lower bound.
			if (!AddCapacity(nested ? 2 : entry.second.size(), limit, capacity)) return 0;
		}
		else
		{
			if (index != 0 && !AddCapacity(1, limit, capacity)) return 0;
			if (!this->array_flag)
			{
				const std::size_t keySize = EncodedSize(entry.first.c_str(), limit);
				if (keySize == 0 || !AddCapacity(keySize, limit, capacity)
					|| !AddCapacity(1, limit, capacity)) return 0;
			}
			if (!AddCapacity(entry.second.size(), limit, capacity)) return 0;
		}
	}
	return capacity;
}

json::jobject::operator std::string() const
{
	if (this->data.empty()) return this->array_flag ? "[]" : "{}";
	std::string result;
	const std::size_t capacity = this->outputCapacity(false, 0, result.max_size());
	if (capacity > result.capacity()) result.reserve(capacity);
	result.push_back(this->array_flag ? '[' : '{');
	for (std::size_t index = 0; index < this->data.size(); ++index)
	{
		// Optimization 4: encode the key and append the stored value directly;
		// the previous + chain allocated copies of keys and entire field values.
		if (index != 0) result.push_back(',');
		const StoredEntry &entry = this->data[index];
		if (!this->array_flag)
		{
			AppendEncodedString(entry.first.c_str(), result);
			result.push_back(':');
		}
		result.append(entry.second);
	}
	result.push_back(this->array_flag ? ']' : '}');
	return result;
}

std::string json::jobject::pretty(unsigned int indent_level) const
{
    std::string result = "";
	// Optimization 3: reserve flat output exactly, nested output conservatively;
	// retain the same capacity hint for the direct writer below.
	const std::size_t capacity = this->outputCapacity(true, indent_level, result.max_size());
	if (capacity > result.capacity()) result.reserve(capacity);
	this->appendPretty(indent_level, true, result);
    return result;
}

void json::jobject::appendPretty(unsigned int indent, bool leadingIndent, std::string &output) const
{
	if (leadingIndent) output.append(indent, '\t');
	if (this->data.empty())
	{
		output.append(this->array_flag ? "[]" : "{}");
		return;
	}
	output.append(this->array_flag ? "[\n" : "{\n");
	for (std::size_t index = 0; index < this->data.size(); ++index)
	{
		const StoredEntry &entry = this->data[index];
		if (!this->array_flag)
		{
			output.append(indent + 1, '\t');
			output.push_back('"');
			// Keep pretty()'s original raw-key behavior, including embedded NUL.
			// Reusing the compact encoder here would silently change its output.
			output.append(entry.first.c_str(), entry.first.size());
			output.append("\": ");
		}
		const jtype::jtype type = jtype::peek(*entry.second.c_str());
		if (type == jtype::jarray || type == jtype::jobject)
		{
			// Optimization 4: retain the original nested parse, but write the
			// child's formatting into the same output rather than a full temporary.
			const json::jobject child = json::jobject::parse(entry.second);
			const std::size_t childStart = output.size();
			child.appendPretty(indent + 1, this->array_flag, output);
			if (!this->array_flag)
			{
				// The old object branch appended std::string(tlws(child.c_str())).
				// Preserve its first-NUL truncation as well as its removed prefix tabs.
				const std::size_t terminator = output.find('\0', childStart);
				if (terminator != std::string::npos) output.erase(terminator);
			}
		}
		else
		{
			if (this->array_flag) output.append(indent + 1, '\t');
			output.append(entry.second);
		}
		if (index + 1 < this->data.size()) output.push_back(',');
		output.push_back('\n');
	}
	output.append(indent, '\t');
	output.push_back(this->array_flag ? ']' : '}');
}
