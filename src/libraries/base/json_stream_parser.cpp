//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     json_stream_parser.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the incremental JSON parser in base/json_stream_parser.h.
 *
 * One state machine serves both streams and JsonDocument::from_json(), so both accept exactly the same input.
 * Within a chunk, strings and numbers are scanned in tight loops and handed out as views of the chunk; only
 * tokens that span chunks or contain escapes are copied, into one reused buffer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "json_stream_parser.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <charconv>
#include <cmath>
#include <utility>

#if defined(_LIBCPP_VERSION)
#include <clocale>
#include <cstdlib>
#ifdef __APPLE__
#include <xlocale.h>
#endif
#endif

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Grants the parser access to the internals of the JSON classes that their public API keeps sorted and safe.
 */
struct JsonAccess
{
    static JsonArray& array(JsonValue& value) noexcept
    {
        return *std::get_if<JsonArray>(&value.m_value);
    }

    static JsonObject& object(JsonValue& value) noexcept
    {
        return *std::get_if<JsonObject>(&value.m_value);
    }

    /** Appends without keeping the order; sort_members() restores it. */
    static void append_member(JsonObject& object, std::string&& key, JsonValue&& value)
    {
        object.m_members.push_back(JsonObject::Member(std::move(key), std::move(value)));
    }

    static void sort_members(JsonObject& object)
    {
        object.sort_members();
    }
};

//=============================================================================================================
/**
 * @brief Builds a JsonDocument from the tokens of one root at a time.
 *
 * Finished values wait on one stack until their array or object closes, which then allocates its storage once at
 * the exact size, instead of growing it element by element.
 */
class JsonStreamParser::DocumentBuilder final : public JsonHandler
{
public:
    void on_start_object() override
    {
        m_frames.push_back({m_values.size(), m_keys.size()});
    }

    void on_end_object() override
    {
        const Frame frame = m_frames.back();
        m_frames.pop_back();
        JsonObject object;
        object.reserve(m_values.size() - frame.firstValue);
        for (std::size_t i = frame.firstValue, k = frame.firstKey; i < m_values.size(); ++i, ++k) {
            JsonAccess::append_member(object, std::move(m_keys[k]), std::move(m_values[i]));
        }
        m_values.resize(frame.firstValue);
        m_keys.resize(frame.firstKey);
        JsonAccess::sort_members(object);
        add(JsonValue(std::move(object)));
    }

    void on_start_array() override
    {
        m_frames.push_back({m_values.size(), m_keys.size()});
    }

    void on_end_array() override
    {
        const Frame frame = m_frames.back();
        m_frames.pop_back();
        JsonArray array;
        array.reserve(m_values.size() - frame.firstValue);
        for (std::size_t i = frame.firstValue; i < m_values.size(); ++i) {
            array.append(std::move(m_values[i]));
        }
        m_values.resize(frame.firstValue);
        add(JsonValue(std::move(array)));
    }

    void on_key(std::string_view key) override
    {
        m_keys.emplace_back(key);
    }

    void on_string(std::string_view value) override
    {
        add(value);
    }

    void on_integer(std::int64_t value) override
    {
        add(value);
    }

    void on_number(double value) override
    {
        add(value);
    }

    void on_bool(bool value) override
    {
        add(value);
    }

    void on_null() override
    {
        add(nullptr);
    }

    /** @return The document whose root just closed. */
    JsonDocument take_document()
    {
        JsonDocument document = m_root.is_array() ? JsonDocument(std::move(JsonAccess::array(m_root)))
                                                  : JsonDocument(std::move(JsonAccess::object(m_root)));
        m_root = JsonValue();
        return document;
    }

    /** Drops a partial document; the stacks keep their capacity. */
    void clear() noexcept
    {
        m_frames.clear();
        m_values.clear();
        m_keys.clear();
        m_root = JsonValue();
    }

private:
    /** An open array or object: where its values and keys start on the stacks. */
    struct Frame
    {
        std::size_t firstValue;
        std::size_t firstKey;
    };

    /** Constructs the value where it is stored; moving a JsonValue temporary trips GCC's -Wmaybe-uninitialized. */
    template<typename T>
    void add(T value)
    {
        if (m_frames.empty()) {
            m_root = JsonValue(value);
        } else {
            m_values.emplace_back(value);
        }
    }

    std::vector<Frame> m_frames;     /**< Arrays and objects not closed yet, from the root inward. */
    std::vector<JsonValue> m_values; /**< Values of the open arrays and objects, innermost last. */
    std::vector<std::string> m_keys; /**< Keys of the values of the open objects, innermost last. */
    JsonValue m_root;                /**< The last root closed. */
};

} // namespace mne

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

bool is_whitespace(char c) noexcept
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

const char* skip_whitespace(const char* p, const char* end) noexcept
{
    while (p < end && is_whitespace(*p)) {
        ++p;
    }
    return p;
}

bool is_number_char(char c) noexcept
{
    return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E';
}

int hex_value(char c) noexcept
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

//=============================================================================================================
/**
 * @param[in]  text       A number token.
 * @param[out] integral   Whether it has neither fraction nor exponent.
 *
 * @return Whether @p text follows the JSON number grammar: -?(0|[1-9][0-9]*)(.[0-9]+)?([eE][+-]?[0-9]+)?
 */
bool is_json_number(std::string_view text, bool& integral) noexcept
{
    std::size_t i = 0;
    const std::size_t n = text.size();
    const auto digits = [&] {
        const std::size_t start = i;
        while (i < n && text[i] >= '0' && text[i] <= '9') {
            ++i;
        }
        return i - start;
    };
    if (i < n && text[i] == '-') {
        ++i;
    }
    if (i < n && text[i] == '0') {
        ++i;
    } else if (digits() == 0) {
        return false;
    }
    integral = true;
    if (i < n && text[i] == '.') {
        ++i;
        integral = false;
        if (digits() == 0) {
            return false;
        }
    }
    if (i < n && (text[i] == 'e' || text[i] == 'E')) {
        ++i;
        integral = false;
        if (i < n && (text[i] == '+' || text[i] == '-')) {
            ++i;
        }
        if (digits() == 0) {
            return false;
        }
    }
    return i == n;
}

//=============================================================================================================
/**
 * @param[in]  text    A valid JSON number.
 * @param[out] value   The closest double.
 *
 * @return Whether @p text is within the range of double; like Qt, overflow and underflow to zero are errors.
 */
bool parse_double(std::string_view text, double& value) noexcept
{
#if defined(_LIBCPP_VERSION)
    // Floating-point std::from_chars needs macOS 26 with libc++, so the C library parses in the "C" locale.
    char buffer[128];
    if (text.size() >= sizeof(buffer)) {
        return false; // no double needs that many digits; reject rather than allocate
    }
    text.copy(buffer, text.size());
    buffer[text.size()] = '\0';
#ifdef _WIN32
    static _locale_t c = _create_locale(LC_NUMERIC, "C");
    value = _strtod_l(buffer, nullptr, c);
#else
    static locale_t c = newlocale(LC_NUMERIC_MASK, "C", nullptr);
    value = strtod_l(buffer, nullptr, c);
#endif
#else
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec == std::errc::result_out_of_range) {
        return false;
    }
#endif
    if (std::isinf(value)) {
        return false;
    }
    if (value == 0.0) {
        const std::string_view mantissa = text.substr(0, text.find_first_of("eE"));
        return mantissa.find_first_of("123456789") == std::string_view::npos;
    }
    return true;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

JsonStreamParser::JsonStreamParser()
: JsonStreamParser(Options())
{
}

//=============================================================================================================

JsonStreamParser::JsonStreamParser(const Options& options)
: m_options(options)
, m_builder(std::make_unique<DocumentBuilder>())
, m_handler(m_builder.get())
{
    m_containers.reserve(std::min<std::size_t>(m_options.maxDepth, 1024));
}

//=============================================================================================================

JsonStreamParser::~JsonStreamParser() = default;

JsonStreamParser::JsonStreamParser(JsonStreamParser&&) noexcept = default;

JsonStreamParser& JsonStreamParser::operator=(JsonStreamParser&&) noexcept = default;

//=============================================================================================================

void JsonStreamParser::set_document_callback(DocumentCallback callback)
{
    m_callback = std::move(callback);
}

//=============================================================================================================

void JsonStreamParser::set_handler(JsonHandler* handler) noexcept
{
    m_handler = handler ? handler : m_builder.get();
}

//=============================================================================================================

bool JsonStreamParser::feed(std::span<const std::byte> chunk)
{
    return feed(std::string_view(reinterpret_cast<const char*>(chunk.data()), chunk.size()));
}

//=============================================================================================================

bool JsonStreamParser::feed(std::string_view chunk)
{
    if (m_state == State::Error) {
        return false;
    }
    const char* const begin = chunk.data();
    const char* const end = begin + chunk.size();
    const char* p = begin;
    const auto offsetOf = [&](const char* at) {
        return m_offset + static_cast<std::size_t>(at - begin);
    };

    // A string or number that began in an earlier chunk continues at the start of this one.
    m_tokenStart = begin;

    while (p < end) {
        switch (m_state) {
            case State::Root:
            case State::Done:
                p = skip_whitespace(p, end);
                if (p == end) {
                    break;
                }
                if (m_state == State::Done) {
                    return fail(JsonParseError::GarbageAtEnd, offsetOf(p));
                }
                if (*p != '{' && *p != '[') {
                    return fail(JsonParseError::IllegalValue, offsetOf(p));
                }
                m_documentStart = offsetOf(p);
                if (!open_container(*p == '{', offsetOf(p))) {
                    return false;
                }
                ++p;
                break;

            case State::Value:
            case State::ValueOrEndArray:
                p = skip_whitespace(p, end);
                if (p == end) {
                    break;
                }
                switch (*p) {
                    case '{':
                    case '[':
                        if (!open_container(*p == '{', offsetOf(p))) {
                            return false;
                        }
                        ++p;
                        break;
                    case '"':
                        m_inKey = false;
                        m_tokenBuffered = false;
                        m_state = State::String;
                        m_tokenStart = ++p;
                        break;
                    case 't':
                    case 'f':
                    case 'n':
                        m_literal = *p == 't' ? "true" : *p == 'f' ? "false"
                                                                   : "null";
                        m_literalPos = 0;
                        m_state = State::Literal;
                        break;
                    case ']':
                        if (m_state == State::ValueOrEndArray) {
                            close_container();
                            ++p;
                            break;
                        }
                        return fail(JsonParseError::MissingObject, offsetOf(p));
                    case '}':
                        return fail(JsonParseError::MissingObject, offsetOf(p));
                    default:
                        if (*p != '-' && (*p < '0' || *p > '9')) {
                            return fail(JsonParseError::IllegalValue, offsetOf(p));
                        }
                        m_tokenBuffered = false;
                        m_tokenStart = p;
                        m_state = State::Number;
                        break;
                }
                break;

            case State::KeyOrEndObject:
            case State::Key:
                p = skip_whitespace(p, end);
                if (p == end) {
                    break;
                }
                if (*p == '"') {
                    m_inKey = true;
                    m_tokenBuffered = false;
                    m_state = State::String;
                    m_tokenStart = ++p;
                    break;
                }
                if (*p == '}' && m_state == State::KeyOrEndObject) {
                    close_container();
                    ++p;
                    break;
                }
                return fail(m_state == State::Key ? JsonParseError::MissingObject : JsonParseError::UnterminatedObject, offsetOf(p));

            case State::Colon:
                p = skip_whitespace(p, end);
                if (p == end) {
                    break;
                }
                if (*p != ':') {
                    return fail(JsonParseError::MissingNameSeparator, offsetOf(p));
                }
                m_state = State::Value;
                ++p;
                break;

            case State::CommaOrEnd: {
                p = skip_whitespace(p, end);
                if (p == end) {
                    break;
                }
                const bool inObject = m_containers.back();
                if (*p == ',') {
                    m_state = inObject ? State::Key : State::Value;
                    ++p;
                } else if (*p == (inObject ? '}' : ']')) {
                    close_container();
                    ++p;
                } else {
                    return fail(inObject ? JsonParseError::UnterminatedObject : JsonParseError::MissingValueSeparator, offsetOf(p));
                }
                break;
            }

            case State::String: {
                const char* q = p;
                for (; q < end; ++q) {
                    const unsigned char c = static_cast<unsigned char>(*q);
                    if (c < 0x80) {
                        if (m_utf8Pending != 0) {
                            return fail(JsonParseError::IllegalUTF8String, offsetOf(q));
                        }
                        if (c == '"' || c == '\\') {
                            break;
                        }
                    } else if (!accept_utf8(c)) {
                        return fail(JsonParseError::IllegalUTF8String, offsetOf(q));
                    }
                }
                if (q == end) {
                    p = end; // the rest of the string comes with the next chunk
                    break;
                }
                if (*q == '\\') {
                    if (!append_token(m_tokenStart, q)) {
                        return fail(JsonParseError::DocumentTooLarge, offsetOf(q));
                    }
                    m_state = State::Escape;
                    p = q + 1;
                    break;
                }
                std::string_view text;
                if (!take_token(q, text)) {
                    return fail(JsonParseError::DocumentTooLarge, offsetOf(q));
                }
                p = q + 1;
                if (m_inKey) {
                    m_handler->on_key(text);
                    m_state = State::Colon;
                } else {
                    m_handler->on_string(text);
                    end_value();
                }
                m_token.clear();
                m_tokenBuffered = false;
                break;
            }

            case State::Escape: {
                char decoded = 0;
                switch (*p) {
                    case '"':
                        decoded = '"';
                        break;
                    case '\\':
                        decoded = '\\';
                        break;
                    case '/':
                        decoded = '/';
                        break;
                    case 'b':
                        decoded = '\b';
                        break;
                    case 'f':
                        decoded = '\f';
                        break;
                    case 'n':
                        decoded = '\n';
                        break;
                    case 'r':
                        decoded = '\r';
                        break;
                    case 't':
                        decoded = '\t';
                        break;
                    case 'u':
                        m_codeUnit = 0;
                        m_hexDigits = 0;
                        m_state = State::Unicode;
                        ++p;
                        continue;
                    default:
                        return fail(JsonParseError::IllegalEscapeSequence, offsetOf(p));
                }
                if (!append_token(&decoded, &decoded + 1)) {
                    return fail(JsonParseError::DocumentTooLarge, offsetOf(p));
                }
                m_state = State::String;
                m_tokenStart = ++p;
                break;
            }

            case State::Unicode: {
                const int digit = hex_value(*p);
                if (digit < 0) {
                    return fail(JsonParseError::IllegalEscapeSequence, offsetOf(p));
                }
                m_codeUnit = m_codeUnit * 16 + static_cast<std::uint32_t>(digit);
                ++p;
                if (++m_hexDigits == 4) {
                    if (!decode_code_unit()) {
                        return fail(JsonParseError::DocumentTooLarge, offsetOf(p));
                    }
                    m_tokenStart = p;
                }
                break;
            }

            case State::SurrogateBackslash:
                if (*p == '\\') {
                    m_state = State::SurrogateU;
                    ++p;
                    break;
                }
                // A high surrogate without its low half becomes U+FFFD so that the string stays valid UTF-8.
                m_highSurrogate = 0;
                if (!append_code_point(0xFFFD)) {
                    return fail(JsonParseError::DocumentTooLarge, offsetOf(p));
                }
                m_state = State::String;
                m_tokenStart = p;
                break;

            case State::SurrogateU:
                if (*p == 'u') {
                    m_codeUnit = 0;
                    m_hexDigits = 0;
                    m_state = State::Unicode;
                    ++p;
                    break;
                }
                m_highSurrogate = 0;
                if (!append_code_point(0xFFFD)) {
                    return fail(JsonParseError::DocumentTooLarge, offsetOf(p));
                }
                m_state = State::Escape; // the backslash already read starts another escape
                break;

            case State::Number: {
                const char* q = p;
                while (q < end && is_number_char(*q)) {
                    ++q;
                }
                if (q == end) {
                    p = end; // the number may go on in the next chunk
                    break;
                }
                std::string_view text;
                if (!take_token(q, text)) {
                    return fail(JsonParseError::DocumentTooLarge, offsetOf(q));
                }
                if (!emit_number(text)) {
                    return fail(JsonParseError::IllegalNumber, offsetOf(q));
                }
                m_token.clear();
                m_tokenBuffered = false;
                p = q;
                end_value();
                break;
            }

            case State::Literal:
                while (p < end && m_literalPos < m_literal.size()) {
                    if (*p != m_literal[m_literalPos]) {
                        return fail(JsonParseError::IllegalValue, offsetOf(p));
                    }
                    ++p;
                    ++m_literalPos;
                }
                if (m_literalPos == m_literal.size()) {
                    if (m_literal == "null") {
                        m_handler->on_null();
                    } else {
                        m_handler->on_bool(m_literal == "true");
                    }
                    end_value();
                }
                break;

            case State::Error:
                return false;
        }
    }

    if (m_state == State::String || m_state == State::Number) {
        if (!append_token(m_tokenStart, end)) {
            return fail(JsonParseError::DocumentTooLarge, offsetOf(end));
        }
        m_tokenBuffered = true;
    }
    m_offset += chunk.size();
    if (!is_idle() && m_offset - m_documentStart > m_options.maxDocumentSize) {
        return fail(JsonParseError::DocumentTooLarge, m_offset);
    }
    return true;
}

//=============================================================================================================

bool JsonStreamParser::finish()
{
    switch (m_state) {
        case State::Error:
            return false;
        case State::Root:
        case State::Done:
            return true;
        case State::String:
        case State::Escape:
        case State::Unicode:
        case State::SurrogateBackslash:
        case State::SurrogateU:
            return fail(JsonParseError::UnterminatedString, m_offset);
        case State::Colon:
            return fail(JsonParseError::MissingNameSeparator, m_offset);
        case State::Literal:
            return fail(JsonParseError::IllegalValue, m_offset);
        default:
            return fail(m_containers.back() ? JsonParseError::UnterminatedObject : JsonParseError::UnterminatedArray, m_offset);
    }
}

//=============================================================================================================

void JsonStreamParser::reset()
{
    m_builder->clear();
    m_containers.clear();
    m_token.clear();
    m_error = JsonParseError();
    m_offset = 0;
    m_documentStart = 0;
    m_highSurrogate = 0;
    m_utf8Pending = 0;
    m_tokenBuffered = false;
    m_state = State::Root;
}

//=============================================================================================================

bool JsonStreamParser::has_error() const noexcept
{
    return m_state == State::Error;
}

//=============================================================================================================

const JsonParseError& JsonStreamParser::error() const noexcept
{
    return m_error;
}

//=============================================================================================================

bool JsonStreamParser::has_document() const noexcept
{
    return m_firstDocument < m_documents.size();
}

//=============================================================================================================

std::size_t JsonStreamParser::document_count() const noexcept
{
    return m_documents.size() - m_firstDocument;
}

//=============================================================================================================

JsonDocument JsonStreamParser::take_document()
{
    if (!has_document()) {
        return JsonDocument();
    }
    JsonDocument document = std::move(m_documents[m_firstDocument++]);
    if (m_firstDocument == m_documents.size()) {
        m_documents.clear(); // keeps the capacity for the next documents
        m_firstDocument = 0;
    }
    return document;
}

//=============================================================================================================

std::size_t JsonStreamParser::bytes_fed() const noexcept
{
    return m_offset;
}

//=============================================================================================================

std::size_t JsonStreamParser::depth() const noexcept
{
    return m_containers.size();
}

//=============================================================================================================

bool JsonStreamParser::is_idle() const noexcept
{
    return m_state == State::Root || m_state == State::Done;
}

//=============================================================================================================

const JsonStreamParser::Options& JsonStreamParser::options() const noexcept
{
    return m_options;
}

//=============================================================================================================

bool JsonStreamParser::fail(JsonParseError::ParseError error, std::size_t offset)
{
    m_error = {offset, error};
    m_state = State::Error;
    m_builder->clear();
    return false;
}

//=============================================================================================================

bool JsonStreamParser::open_container(bool object, std::size_t offset)
{
    if (m_containers.size() >= m_options.maxDepth) {
        return fail(JsonParseError::DeepNesting, offset);
    }
    if (!m_containers.empty() && offset - m_documentStart > m_options.maxDocumentSize) {
        return fail(JsonParseError::DocumentTooLarge, offset);
    }
    m_containers.push_back(object);
    if (object) {
        m_handler->on_start_object();
        m_state = State::KeyOrEndObject;
    } else {
        m_handler->on_start_array();
        m_state = State::ValueOrEndArray;
    }
    return true;
}

//=============================================================================================================

void JsonStreamParser::close_container()
{
    const bool object = m_containers.back();
    m_containers.pop_back();
    if (object) {
        m_handler->on_end_object();
    } else {
        m_handler->on_end_array();
    }
    end_value();
}

//=============================================================================================================

void JsonStreamParser::end_value()
{
    if (!m_containers.empty()) {
        m_state = State::CommaOrEnd;
        return;
    }
    m_state = m_options.singleDocument ? State::Done : State::Root;
    if (m_handler != m_builder.get()) {
        m_handler->on_end_document();
    } else if (m_callback) {
        m_callback(m_builder->take_document());
    } else {
        m_documents.push_back(m_builder->take_document());
    }
}

//=============================================================================================================

bool JsonStreamParser::append_token(const char* begin, const char* end)
{
    const std::size_t size = static_cast<std::size_t>(end - begin);
    if (size > m_options.maxBufferSize - std::min(m_token.size(), m_options.maxBufferSize)) {
        return false;
    }
    m_token.append(begin, size);
    return true;
}

//=============================================================================================================

bool JsonStreamParser::take_token(const char* end, std::string_view& token)
{
    if (!m_tokenBuffered && m_token.empty()) {
        token = std::string_view(m_tokenStart, static_cast<std::size_t>(end - m_tokenStart)); // no copy
        return true;
    }
    if (!append_token(m_tokenStart, end)) {
        return false;
    }
    token = m_token;
    return true;
}

//=============================================================================================================

bool JsonStreamParser::emit_number(std::string_view text)
{
    bool integral = false;
    if (!is_json_number(text, integral)) {
        return false;
    }
    if (integral) {
        std::int64_t value = 0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec == std::errc()) {
            m_handler->on_integer(value);
            return true;
        }
        // Beyond std::int64_t the number is kept as double, like Qt.
    }
    double value = 0.0;
    if (!parse_double(text, value)) {
        return false;
    }
    m_handler->on_number(value);
    return true;
}

//=============================================================================================================

bool JsonStreamParser::append_code_point(std::uint32_t codePoint)
{
    char utf8[4];
    std::size_t size = 0;
    if (codePoint < 0x80) {
        utf8[size++] = static_cast<char>(codePoint);
    } else if (codePoint < 0x800) {
        utf8[size++] = static_cast<char>(0xC0 | (codePoint >> 6));
        utf8[size++] = static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
        utf8[size++] = static_cast<char>(0xE0 | (codePoint >> 12));
        utf8[size++] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        utf8[size++] = static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
        utf8[size++] = static_cast<char>(0xF0 | (codePoint >> 18));
        utf8[size++] = static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        utf8[size++] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        utf8[size++] = static_cast<char>(0x80 | (codePoint & 0x3F));
    }
    return append_token(utf8, utf8 + size);
}

//=============================================================================================================

bool JsonStreamParser::decode_code_unit()
{
    std::uint32_t unit = m_codeUnit;
    if (m_highSurrogate != 0) {
        const std::uint32_t high = std::exchange(m_highSurrogate, 0);
        if (unit >= 0xDC00 && unit <= 0xDFFF) {
            m_state = State::String;
            return append_code_point(0x10000 + ((high - 0xD800) << 10) + (unit - 0xDC00));
        }
        if (!append_code_point(0xFFFD)) {
            return false;
        }
    }
    if (unit >= 0xD800 && unit <= 0xDBFF) {
        m_highSurrogate = unit;
        m_state = State::SurrogateBackslash;
        return true;
    }
    if (unit >= 0xDC00 && unit <= 0xDFFF) {
        unit = 0xFFFD; // a low surrogate on its own
    }
    m_state = State::String;
    return append_code_point(unit);
}

//=============================================================================================================

bool JsonStreamParser::accept_utf8(unsigned char byte) noexcept
{
    if (m_utf8Pending != 0) {
        if (byte < m_utf8Lower || byte > m_utf8Upper) {
            return false;
        }
        --m_utf8Pending;
        m_utf8Lower = 0x80;
        m_utf8Upper = 0xBF;
        return true;
    }
    // Lead bytes per RFC 3629: no overlong forms, no surrogates, nothing beyond U+10FFFF.
    m_utf8Lower = 0x80;
    m_utf8Upper = 0xBF;
    if (byte >= 0xC2 && byte <= 0xDF) {
        m_utf8Pending = 1;
    } else if (byte == 0xE0) {
        m_utf8Pending = 2;
        m_utf8Lower = 0xA0;
    } else if (byte == 0xED) {
        m_utf8Pending = 2;
        m_utf8Upper = 0x9F;
    } else if (byte >= 0xE1 && byte <= 0xEF) {
        m_utf8Pending = 2;
    } else if (byte == 0xF0) {
        m_utf8Pending = 3;
        m_utf8Lower = 0x90;
    } else if (byte >= 0xF1 && byte <= 0xF3) {
        m_utf8Pending = 3;
    } else if (byte == 0xF4) {
        m_utf8Pending = 3;
        m_utf8Upper = 0x8F;
    } else {
        return false;
    }
    return true;
}
