//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     json.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Definition of the JSON value classes and the JSON writer in base/json.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "json.h"
#include "json_stream_parser.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

/** Doubles of at most this magnitude with no fraction are written as integers, as Qt 6 does. */
constexpr double kMaxExactInteger = 9007199254740992.0; // 2^53

/** Spaces per indentation level of JsonDocument::Indented. */
constexpr std::size_t kIndent = 4;

/** Escape sequences of the control characters; empty where \\u00XX is used. */
constexpr std::array<std::string_view, 32> kControlEscapes = {
    "", "", "", "", "", "", "", "", "\\b", "\\t", "\\n", "", "\\f", "\\r", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};

//=============================================================================================================
/**
 * @brief Writes JSON text into one string, reserving its size once up front.
 */
class Writer
{
public:
    Writer(std::string& out, bool indented)
    : m_out(out)
    , m_indented(indented)
    {
    }

    //=========================================================================================================
    /**
     * @param[in] value   A value.
     * @param[in] depth   Nesting level of @p value.
     *
     * @return An upper estimate of the bytes value() writes for @p value, short only for unusual escapes.
     */
    std::size_t estimate(const JsonValue& value, std::size_t depth) const
    {
        const std::size_t indent = m_indented ? (depth + 1) * kIndent + 1 : 0;
        if (value.is_string()) {
            return value.to_string().size() + 2;
        }
        if (value.is_array()) {
            std::size_t size = 2 + indent;
            for (const JsonValue& element : value.to_array()) {
                size += 1 + indent + estimate(element, depth + 1);
            }
            return size;
        }
        if (value.is_object()) {
            std::size_t size = 2 + indent;
            for (const auto& [key, member] : value.to_object()) {
                size += key.size() + 5 + indent + estimate(member, depth + 1);
            }
            return size;
        }
        return 24; // the longest number, true, false and null
    }

    //=========================================================================================================
    /**
     * @param[in] value   The value to write.
     * @param[in] depth   Nesting level of @p value, for indentation.
     */
    void value(const JsonValue& value, std::size_t depth)
    {
        switch (value.type()) {
            case JsonValue::Bool:
                m_out += value.to_bool() ? "true" : "false";
                break;
            case JsonValue::Double:
                if (value.is_integer()) {
                    integer(value.to_integer());
                } else {
                    number(value.to_double());
                }
                break;
            case JsonValue::String:
                string(value.to_string());
                break;
            case JsonValue::Array:
                array(value.to_array(), depth);
                break;
            case JsonValue::Object:
                object(value.to_object(), depth);
                break;
            default:
                m_out += "null"; // Qt writes undefined values as null as well
                break;
        }
    }

    //=========================================================================================================
    /**
     * @param[in] array   An array.
     * @param[in] depth   Nesting level of @p array.
     */
    void array(const JsonArray& array, std::size_t depth)
    {
        m_out += '[';
        bool first = true;
        for (const JsonValue& element : array) {
            if (!first) {
                m_out += ',';
            }
            first = false;
            newline(depth + 1);
            value(element, depth + 1);
        }
        newline(depth);
        m_out += ']';
    }

    //=========================================================================================================
    /**
     * @param[in] object   An object.
     * @param[in] depth    Nesting level of @p object.
     */
    void object(const JsonObject& object, std::size_t depth)
    {
        m_out += '{';
        bool first = true;
        for (const auto& [key, member] : object) {
            if (!first) {
                m_out += ',';
            }
            first = false;
            newline(depth + 1);
            string(key);
            m_out += m_indented ? ": " : ":";
            value(member, depth + 1);
        }
        newline(depth);
        m_out += '}';
    }

private:
    /** Starts a new line indented for @p depth in indented mode. */
    void newline(std::size_t depth)
    {
        if (m_indented) {
            m_out += '\n';
            m_out.append(depth * kIndent, ' ');
        }
    }

    /** Writes @p value in decimal. */
    void integer(std::int64_t value)
    {
        char buffer[24];
        const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
        m_out.append(buffer, result.ptr);
    }

    /** Writes @p value in its shortest round-trip form, like Qt: whole numbers up to 2^53 without exponent, NaN as null. */
    void number(double value)
    {
        if (!std::isfinite(value)) {
            m_out += "null";
            return;
        }
        if (std::abs(value) <= kMaxExactInteger && std::trunc(value) == value) {
            integer(static_cast<std::int64_t>(value));
            return;
        }
        // The shortest round-trip digits, written in plain or exponent notation, whichever is shorter (plain on a tie).
        // std::to_chars without a format would print every digit of large numbers in plain notation, Qt pads zeros.
        char scientific[32];
        const auto result = std::to_chars(scientific, scientific + sizeof(scientific), value, std::chars_format::scientific);
        const std::string_view text(scientific, static_cast<std::size_t>(result.ptr - scientific));
        const std::size_t e = text.find('e');
        const bool negative = text.front() == '-';
        char digits[24];
        std::size_t digitCount = 0;
        for (const char c : text.substr(negative ? 1 : 0, e - (negative ? 1 : 0))) {
            if (c != '.') {
                digits[digitCount++] = c;
            }
        }
        int exponent = 0;
        const std::string_view exponentText = text.substr(e + (text[e + 1] == '+' ? 2 : 1));
        std::from_chars(exponentText.data(), exponentText.data() + exponentText.size(), exponent);

        const std::size_t plainSize = (negative ? 1 : 0) + (exponent < 0 ? 2 + static_cast<std::size_t>(-exponent - 1) + digitCount : std::max(static_cast<std::size_t>(exponent) + 1, digitCount) + (digitCount > static_cast<std::size_t>(exponent) + 1 ? 1 : 0));
        if (plainSize > text.size()) {
            m_out += text;
            return;
        }
        char plain[32]; // plainSize <= text.size() < 32
        std::size_t size = 0;
        if (negative) {
            plain[size++] = '-';
        }
        if (exponent < 0) {
            plain[size++] = '0';
            plain[size++] = '.';
            for (int i = -1; i > exponent; --i) {
                plain[size++] = '0';
            }
            for (std::size_t i = 0; i < digitCount; ++i) {
                plain[size++] = digits[i];
            }
        } else {
            const std::size_t integerDigits = static_cast<std::size_t>(exponent) + 1;
            for (std::size_t i = 0; i < std::max(integerDigits, digitCount); ++i) {
                if (i == integerDigits) {
                    plain[size++] = '.';
                }
                plain[size++] = i < digitCount ? digits[i] : '0';
            }
        }
        m_out.append(plain, size);
    }

    /** Writes @p text quoted, copying runs that need no escape in one go. */
    void string(std::string_view text)
    {
        m_out += '"';
        std::size_t run = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(text[i]);
            if (c >= 0x20 && c != '"' && c != '\\') {
                continue;
            }
            m_out.append(text.data() + run, i - run);
            run = i + 1;
            if (c == '"') {
                m_out += "\\\"";
            } else if (c == '\\') {
                m_out += "\\\\";
            } else if (!kControlEscapes[c].empty()) {
                m_out += kControlEscapes[c];
            } else {
                constexpr char kHex[] = "0123456789abcdef";
                const char escape[6] = {'\\', 'u', '0', '0', kHex[c >> 4], kHex[c & 0xF]};
                m_out.append(escape, sizeof(escape));
            }
        }
        m_out.append(text.data() + run, text.size() - run);
        m_out += '"';
    }

    std::string& m_out; /**< Destination. */
    bool m_indented;    /**< Whether to write JsonDocument::Indented. */
};

//=============================================================================================================
/**
 * @param[in] members   Members sorted by key.
 * @param[in] key       A key.
 *
 * @return The first member whose key is not less than @p key.
 */
template<typename Members>
auto lower_bound(Members& members, std::string_view key)
{
    return std::lower_bound(members.begin(), members.end(), key,
                            [](const JsonObject::Member& member, std::string_view k) { return member.key() < k; });
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::string_view JsonParseError::error_string() const noexcept
{
    switch (error) {
        case NoError:
            return "no error occurred";
        case UnterminatedObject:
            return "unterminated object";
        case MissingNameSeparator:
            return "missing name separator";
        case UnterminatedArray:
            return "unterminated array";
        case MissingValueSeparator:
            return "missing value separator";
        case IllegalValue:
            return "illegal value";
        case TerminationByNumber:
            return "invalid termination by number";
        case IllegalNumber:
            return "illegal number";
        case IllegalEscapeSequence:
            return "invalid escape sequence";
        case IllegalUTF8String:
            return "invalid UTF8 string";
        case UnterminatedString:
            return "unterminated string";
        case MissingObject:
            return "object is missing after a comma";
        case DeepNesting:
            return "too deeply nested document";
        case DocumentTooLarge:
            return "too large document";
        case GarbageAtEnd:
            return "garbage at the end of the document";
    }
    return "unknown error";
}

//=============================================================================================================

JsonValue::JsonValue(Type type)
{
    switch (type) {
        case Bool:
            m_value.emplace<bool>(false);
            break;
        case Double:
            m_value.emplace<double>(0.0);
            break;
        case String:
            m_value.emplace<std::string>();
            break;
        case Array:
            m_value.emplace<JsonArray>();
            break;
        case Object:
            m_value.emplace<JsonObject>();
            break;
        case Undefined:
            m_value.emplace<UndefinedTag>();
            break;
        default:
            break;
    }
}

//=============================================================================================================

const JsonValue& JsonValue::undefined_value() noexcept
{
    static const JsonValue undefined(Undefined);
    return undefined;
}

//=============================================================================================================

int JsonValue::to_int(int defaultValue) const noexcept
{
    if (const std::int64_t* value = std::get_if<std::int64_t>(&m_value)) {
        return std::in_range<int>(*value) ? static_cast<int>(*value) : defaultValue;
    }
    if (const double* value = std::get_if<double>(&m_value)) {
        if (std::trunc(*value) == *value && *value >= std::numeric_limits<int>::min() && *value <= std::numeric_limits<int>::max()) {
            return static_cast<int>(*value);
        }
    }
    return defaultValue;
}

//=============================================================================================================

std::int64_t JsonValue::to_integer(std::int64_t defaultValue) const noexcept
{
    if (const std::int64_t* value = std::get_if<std::int64_t>(&m_value)) {
        return *value;
    }
    if (const double* value = std::get_if<double>(&m_value)) {
        // [-2^63, 2^63) holds exactly the doubles that convert to std::int64_t without overflow.
        constexpr double kLimit = 9223372036854775808.0;
        if (std::trunc(*value) == *value && *value >= -kLimit && *value < kLimit) {
            return static_cast<std::int64_t>(*value);
        }
    }
    return defaultValue;
}

//=============================================================================================================

const JsonArray& JsonValue::to_array() const noexcept
{
    static const JsonArray empty;
    const JsonArray* value = std::get_if<JsonArray>(&m_value);
    return value ? *value : empty;
}

//=============================================================================================================

const JsonObject& JsonValue::to_object() const noexcept
{
    static const JsonObject empty;
    const JsonObject* value = std::get_if<JsonObject>(&m_value);
    return value ? *value : empty;
}

//=============================================================================================================

const JsonValue& JsonValue::operator[](std::string_view key) const noexcept
{
    const JsonObject* object = std::get_if<JsonObject>(&m_value);
    return object ? object->value(key) : undefined_value();
}

//=============================================================================================================

const JsonValue& JsonValue::operator[](std::size_t i) const noexcept
{
    const JsonArray* array = std::get_if<JsonArray>(&m_value);
    return array ? array->at(i) : undefined_value();
}

//=============================================================================================================

bool JsonValue::operator==(const JsonValue& other) const
{
    if (is_number() && other.is_number()) {
        if (is_integer() && other.is_integer()) {
            return to_integer() == other.to_integer();
        }
        return to_double() == other.to_double();
    }
    return m_value == other.m_value;
}

//=============================================================================================================

JsonValue& JsonArray::operator[](std::size_t i) noexcept
{
    if (i < m_values.size()) {
        return m_values[i];
    }
    thread_local JsonValue scratch;
    scratch = JsonValue(JsonValue::Undefined);
    return scratch;
}

//=============================================================================================================

JsonValue JsonArray::take_at(std::size_t i)
{
    if (i >= m_values.size()) {
        return JsonValue(JsonValue::Undefined);
    }
    JsonValue value = std::move(m_values[i]);
    m_values.erase(m_values.begin() + static_cast<std::ptrdiff_t>(i));
    return value;
}

//=============================================================================================================

bool JsonArray::contains(const JsonValue& value) const
{
    return std::find(m_values.begin(), m_values.end(), value) != m_values.end();
}

//=============================================================================================================

bool JsonArray::operator==(const JsonArray& other) const
{
    return m_values == other.m_values;
}

//=============================================================================================================

JsonArray JsonArray::from_string_list(std::span<const std::string> list)
{
    JsonArray array;
    array.reserve(list.size());
    for (const std::string& item : list) {
        array.append(item);
    }
    return array;
}

//=============================================================================================================

std::vector<std::string> JsonArray::to_string_list() const
{
    std::vector<std::string> list;
    list.reserve(size());
    for (const JsonValue& value : *this) {
        list.emplace_back(value.to_string());
    }
    return list;
}

//=============================================================================================================

JsonObject::JsonObject(std::initializer_list<std::pair<std::string_view, JsonValue>> members)
{
    m_members.reserve(members.size());
    for (const auto& [key, value] : members) {
        m_members.push_back(Member(std::string(key), value));
    }
    sort_members();
}

//=============================================================================================================

std::vector<std::string> JsonObject::keys() const
{
    std::vector<std::string> keys;
    keys.reserve(m_members.size());
    for (const Member& member : m_members) {
        keys.push_back(member.m_key);
    }
    return keys;
}

//=============================================================================================================

JsonValue& JsonObject::operator[](std::string_view key)
{
    auto it = lower_bound(m_members, key);
    if (it == m_members.end() || it->key() != key) {
        it = m_members.insert(it, Member(std::string(key), JsonValue()));
    }
    return it->m_value;
}

//=============================================================================================================

JsonObject::iterator JsonObject::insert(std::string_view key, JsonValue value)
{
    const auto it = lower_bound(m_members, key);
    if (it != m_members.end() && it->m_key == key) {
        it->m_value = std::move(value);
        return it;
    }
    return m_members.insert(it, Member(std::string(key), std::move(value)));
}

//=============================================================================================================

void JsonObject::remove(std::string_view key)
{
    const const_iterator it = find(key);
    if (it != m_members.cend()) {
        m_members.erase(it);
    }
}

//=============================================================================================================

JsonValue JsonObject::take(std::string_view key)
{
    const iterator it = find(key);
    if (it == m_members.end()) {
        return JsonValue(JsonValue::Undefined);
    }
    JsonValue value = std::move(it->m_value);
    m_members.erase(it);
    return value;
}

//=============================================================================================================

JsonObject JsonObject::without(std::initializer_list<std::string_view> keys) const
{
    JsonObject result;
    result.m_members.reserve(m_members.size());
    for (const Member& member : m_members) {
        if (std::ranges::find(keys, std::string_view(member.key())) == keys.end()) {
            result.m_members.push_back(member); // already sorted
        }
    }
    return result;
}

//=============================================================================================================

JsonObject::iterator JsonObject::find(std::string_view key) noexcept
{
    const auto it = lower_bound(m_members, key);
    return it != m_members.end() && it->m_key == key ? it : m_members.end();
}

//=============================================================================================================

JsonObject::const_iterator JsonObject::find(std::string_view key) const noexcept
{
    const auto it = lower_bound(m_members, key);
    return it != m_members.end() && it->m_key == key ? it : m_members.end();
}

//=============================================================================================================

JsonObject::iterator JsonObject::erase(const_iterator it)
{
    return m_members.erase(it);
}

//=============================================================================================================

bool JsonObject::operator==(const JsonObject& other) const
{
    return m_members == other.m_members;
}

//=============================================================================================================

void JsonObject::sort_members()
{
    const auto byKey = [](const Member& a, const Member& b) {
        return a.m_key < b.m_key;
    };
    if (std::is_sorted(m_members.begin(), m_members.end(), byKey) && std::adjacent_find(m_members.begin(), m_members.end(), [](const Member& a, const Member& b) { return a.m_key == b.m_key; }) == m_members.end()) {
        return;
    }
    if (m_members.size() <= 32) {
        // Insertion sort: stable and allocation-free, and fastest for the small objects that dominate real data.
        for (auto it = std::next(m_members.begin()); it != m_members.end(); ++it) {
            for (auto hole = it; hole != m_members.begin() && byKey(*hole, *std::prev(hole)); --hole) {
                std::iter_swap(hole, std::prev(hole));
            }
        }
    } else {
        std::stable_sort(m_members.begin(), m_members.end(), byKey);
    }
    // Of equal keys the stable sort keeps input order, so the last one is the one to keep, as in Qt.
    auto out = m_members.begin();
    for (auto it = m_members.begin(); it != m_members.end(); ++it) {
        const auto next = std::next(it);
        if (next != m_members.end() && next->m_key == it->m_key) {
            continue;
        }
        if (out != it) {
            *out = std::move(*it);
        }
        ++out;
    }
    m_members.erase(out, m_members.end());
}

//=============================================================================================================

JsonDocument JsonDocument::from_json(std::string_view json, JsonParseError* error)
{
    constexpr std::string_view kByteOrderMark = "\xEF\xBB\xBF";
    const std::size_t skipped = json.starts_with(kByteOrderMark) ? kByteOrderMark.size() : 0;

    JsonStreamParser::Options options;
    options.maxDepth = 1024;
    options.maxBufferSize = std::numeric_limits<std::size_t>::max();
    options.maxDocumentSize = std::numeric_limits<std::size_t>::max();
    options.singleDocument = true;
    JsonStreamParser parser(options);
    parser.feed(json.substr(skipped));
    parser.finish();

    JsonParseError result = parser.error();
    JsonDocument document;
    if (result.error == JsonParseError::NoError) {
        if (parser.has_document()) {
            document = parser.take_document();
        } else {
            result = {json.size() - skipped, JsonParseError::IllegalValue}; // nothing but whitespace
        }
    }
    if (error) {
        *error = result;
        if (result.error != JsonParseError::NoError) {
            error->offset += skipped;
        }
    }
    return document;
}

//=============================================================================================================

std::string JsonDocument::to_json(JsonFormat format) const
{
    std::string out;
    if (is_null()) {
        return out;
    }
    Writer writer(out, format == Indented);
    out.reserve(writer.estimate(m_root, 0) + 1);
    writer.value(m_root, 0);
    if (format == Indented) {
        out += '\n';
    }
    return out;
}
