//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     cbor.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of the CBOR classes declared in cbor.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cbor.h"
#include "json.h"
#include "string_utils.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE LOCAL FUNCTIONS
//=============================================================================================================

namespace
{

constexpr int kMaxNesting = 1024;

/** Appends CBOR to a byte buffer. */
class Encoder
{
public:
    explicit Encoder(CborByteArray& out)
    : m_out(out)
    {
    }

    void head(unsigned major, std::uint64_t value)
    {
        const auto type = static_cast<std::byte>(major << 5);
        if (value < 24) {
            m_out.push_back(type | static_cast<std::byte>(value));
        } else if (value <= 0xff) {
            m_out.push_back(type | std::byte{24});
            big_endian(value, 1);
        } else if (value <= 0xffff) {
            m_out.push_back(type | std::byte{25});
            big_endian(value, 2);
        } else if (value <= 0xffffffffULL) {
            m_out.push_back(type | std::byte{26});
            big_endian(value, 4);
        } else {
            m_out.push_back(type | std::byte{27});
            big_endian(value, 8);
        }
    }

    void text(std::string_view value)
    {
        head(3, value.size());
        const auto* first = reinterpret_cast<const std::byte*>(value.data());
        m_out.insert(m_out.end(), first, first + value.size());
    }

    void big_endian(std::uint64_t value, int bytes)
    {
        for (int shift = (bytes - 1) * 8; shift >= 0; shift -= 8) {
            m_out.push_back(static_cast<std::byte>((value >> shift) & 0xff));
        }
    }

    void value(const CborValue& v)
    {
        switch (v.type()) {
            case CborValue::Integer: {
                const std::int64_t i = v.to_integer();
                if (i >= 0) {
                    head(0, static_cast<std::uint64_t>(i));
                } else {
                    head(1, static_cast<std::uint64_t>(-1 - i));
                }
                break;
            }
            case CborValue::ByteArray: {
                const CborByteArray& bytes = v.to_byte_array();
                head(2, bytes.size());
                m_out.insert(m_out.end(), bytes.begin(), bytes.end());
                break;
            }
            case CborValue::String:
                text(v.to_string());
                break;
            case CborValue::Array:
                head(4, v.to_array().size());
                for (const CborValue& element : v.to_array()) {
                    value(element);
                }
                break;
            case CborValue::Map:
                head(5, v.to_map().size());
                for (const auto& [key, member] : v.to_map()) {
                    text(key);
                    value(member);
                }
                break;
            case CborValue::Tag:
                head(6, v.tag());
                value(v.tagged_value());
                break;
            case CborValue::False:
                m_out.push_back(std::byte{0xf4});
                break;
            case CborValue::True:
                m_out.push_back(std::byte{0xf5});
                break;
            case CborValue::Null:
                m_out.push_back(std::byte{0xf6});
                break;
            case CborValue::Undefined:
                m_out.push_back(std::byte{0xf7});
                break;
            case CborValue::Double:
                m_out.push_back(std::byte{0xfb});
                big_endian(std::bit_cast<std::uint64_t>(v.to_double()), 8);
                break;
        }
    }

private:
    CborByteArray& m_out;
};

/** Decodes one CBOR value from a byte span. */
class Decoder
{
public:
    explicit Decoder(std::span<const std::byte> data)
    : m_data(data)
    {
    }

    CborParseError error;

    std::size_t pos() const
    {
        return m_pos;
    }

    bool failed() const
    {
        return error.error != CborParseError::NoError;
    }

    std::size_t remaining() const
    {
        return m_data.size() - m_pos;
    }

    CborValue value(int depth)
    {
        const std::size_t start = m_pos;
        if (remaining() == 0) {
            return fail(CborParseError::EndOfFile, start);
        }
        const auto initial = std::to_integer<unsigned>(m_data[m_pos++]);
        const unsigned major = initial >> 5;
        const unsigned info = initial & 31;
        if (info >= 28 && info <= 30) {
            return fail(CborParseError::UnknownType, start);
        }
        if (info == 31 && (major <= 1 || major == 6)) {
            return fail(CborParseError::IllegalNumber, start);
        }
        if (major == 7) {
            return simple(info, start);
        }
        std::uint64_t argument = 0;
        if (info != 31 && !read_argument(info, argument, start)) {
            return {};
        }
        switch (major) {
            case 0:
                return argument <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ? CborValue(static_cast<std::int64_t>(argument))
                                                                                                        : CborValue(static_cast<double>(argument));
            case 1:
                return argument <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ? CborValue(-1 - static_cast<std::int64_t>(argument))
                                                                                                        : CborValue(-1.0 - static_cast<double>(argument));
            case 2:
            case 3:
                return string(major, info == 31, argument, start);
            case 4:
            case 5:
            case 6:
                if (depth >= kMaxNesting) {
                    return fail(CborParseError::NestingTooDeep, start);
                }
                return major == 4 ? array(info == 31, argument, depth) : major == 5 ? map(info == 31, argument, depth)
                                                                                    : CborValue::tagged(argument, value(depth + 1));
            default:
                return {};
        }
    }

private:
    CborValue fail(CborParseError::Code code, std::size_t offset)
    {
        if (!failed()) {
            error.error = code;
            error.offset = offset;
        }
        return {};
    }

    bool read_argument(unsigned info, std::uint64_t& argument, std::size_t start)
    {
        if (info < 24) {
            argument = info;
            return true;
        }
        const std::size_t bytes = std::size_t{1} << (info - 24);
        if (remaining() < bytes) {
            fail(CborParseError::EndOfFile, start);
            return false;
        }
        argument = 0;
        for (std::size_t i = 0; i < bytes; ++i) {
            argument = (argument << 8) | std::to_integer<std::uint64_t>(m_data[m_pos++]);
        }
        return true;
    }

    bool at_break()
    {
        if (remaining() > 0 && m_data[m_pos] == std::byte{0xff}) {
            ++m_pos;
            return true;
        }
        return false;
    }

    CborValue simple(unsigned info, std::size_t start)
    {
        switch (info) {
            case 20:
                return false;
            case 21:
                return true;
            case 22:
                return nullptr;
            case 23:
                return {};
            case 24:
                if (remaining() < 1) {
                    return fail(CborParseError::EndOfFile, start);
                }
                ++m_pos;
                return fail(CborParseError::IllegalSimpleType, start);
            case 25:
            case 26:
            case 27: {
                std::uint64_t bits = 0;
                if (!read_argument(info, bits, start)) {
                    return {};
                }
                if (info == 27) {
                    return std::bit_cast<double>(bits);
                }
                if (info == 26) {
                    return static_cast<double>(std::bit_cast<float>(static_cast<std::uint32_t>(bits)));
                }
                return half_to_double(static_cast<std::uint16_t>(bits));
            }
            case 31:
                return fail(CborParseError::UnexpectedBreak, start);
            default:
                return fail(CborParseError::IllegalSimpleType, start);
        }
    }

    static double half_to_double(std::uint16_t half)
    {
        const int exponent = (half >> 10) & 0x1f;
        const int mantissa = half & 0x3ff;
        double value = 0.0;
        if (exponent == 0) {
            value = std::ldexp(mantissa, -24);
        } else if (exponent == 31) {
            value = mantissa == 0 ? std::numeric_limits<double>::infinity() : std::numeric_limits<double>::quiet_NaN();
        } else {
            value = std::ldexp(mantissa + 1024, exponent - 25);
        }
        return (half & 0x8000) ? -value : value;
    }

    CborValue string(unsigned major, bool indefinite, std::uint64_t length, std::size_t start)
    {
        std::string bytes;
        if (!indefinite) {
            if (!append_chunk(bytes, length, start)) {
                return {};
            }
        } else {
            while (!at_break()) {
                const std::size_t chunkStart = m_pos;
                if (remaining() == 0) {
                    return fail(CborParseError::EndOfFile, chunkStart);
                }
                const auto initial = std::to_integer<unsigned>(m_data[m_pos++]);
                const unsigned info = initial & 31;
                if ((initial >> 5) != major || info == 31) {
                    return fail(CborParseError::IllegalType, chunkStart);
                }
                if (info >= 28) {
                    return fail(CborParseError::UnknownType, chunkStart);
                }
                std::uint64_t chunk = 0;
                if (!read_argument(info, chunk, chunkStart) || !append_chunk(bytes, chunk, chunkStart)) {
                    return {};
                }
            }
        }
        if (major == 2) {
            const auto* first = reinterpret_cast<const std::byte*>(bytes.data());
            return CborByteArray(first, first + bytes.size());
        }
        if (!is_utf8(bytes)) {
            return fail(CborParseError::InvalidUtf8String, start);
        }
        return bytes;
    }

    bool append_chunk(std::string& bytes, std::uint64_t length, std::size_t start)
    {
        if (length > remaining()) {
            fail(CborParseError::DataTooLarge, start);
            return false;
        }
        bytes.append(reinterpret_cast<const char*>(m_data.data() + m_pos), static_cast<std::size_t>(length));
        m_pos += static_cast<std::size_t>(length);
        return true;
    }

    CborValue array(bool indefinite, std::uint64_t count, int depth)
    {
        CborArray array;
        if (indefinite) {
            while (!failed() && !at_break()) {
                array.append(value(depth + 1));
            }
        } else {
            // Every element takes at least one byte
            if (count > remaining()) {
                return fail(CborParseError::DataTooLarge, m_pos);
            }
            array.reserve(static_cast<std::size_t>(count));
            for (std::uint64_t i = 0; i < count && !failed(); ++i) {
                array.append(value(depth + 1));
            }
        }
        return failed() ? CborValue() : CborValue(std::move(array));
    }

    CborValue map(bool indefinite, std::uint64_t count, int depth)
    {
        CborMap map;
        if (!indefinite) {
            // Every key and every value takes at least one byte
            if (count > remaining() / 2) {
                return fail(CborParseError::DataTooLarge, m_pos);
            }
            map.reserve(static_cast<std::size_t>(count));
        }
        for (std::uint64_t i = 0; !failed() && (indefinite ? !at_break() : i < count); ++i) {
            const std::size_t keyStart = m_pos;
            const CborValue key = value(depth + 1);
            std::string text;
            if (key.is_string()) {
                text = key.to_string();
            } else if (key.is_integer()) {
                text = std::to_string(key.to_integer());
            } else if (!failed()) {
                return fail(CborParseError::UnsupportedType, keyStart);
            }
            CborValue member = value(depth + 1);
            if (!failed()) {
                map.append(std::move(text), std::move(member));
            }
        }
        return failed() ? CborValue() : CborValue(std::move(map));
    }

    std::span<const std::byte> m_data;
    std::size_t m_pos = 0;
};

} // namespace

//=============================================================================================================
// CborParseError
//=============================================================================================================

std::string_view CborParseError::error_string() const noexcept
{
    switch (error) {
        case NoError:
            return "No error";
        case GarbageAtEnd:
            return "Data found after the end of the stream";
        case EndOfFile:
            return "Unexpected end of input data";
        case UnexpectedBreak:
            return "Invalid CBOR stream: unexpected 'break' byte";
        case UnknownType:
            return "Invalid CBOR stream: unknown type";
        case IllegalType:
            return "Invalid CBOR stream: illegal type found";
        case IllegalNumber:
            return "Invalid CBOR stream: illegal number encoding (future extension)";
        case IllegalSimpleType:
            return "Invalid CBOR stream: illegal simple type";
        case InvalidUtf8String:
            return "Invalid CBOR stream: invalid UTF-8 text string";
        case DataTooLarge:
            return "Internal limitation: data set too large";
        case NestingTooDeep:
            return "Internal limitation: data nesting too deep";
        case UnsupportedType:
            return "Unsupported CBOR type: map key is neither text nor an integer";
    }
    return "Unknown error";
}

//=============================================================================================================
// CborArray
//=============================================================================================================

CborArray::CborArray(std::initializer_list<CborValue> values)
: m_values(values)
{
}

std::size_t CborArray::size() const noexcept
{
    return m_values.size();
}

bool CborArray::empty() const noexcept
{
    return m_values.empty();
}

const CborValue& CborArray::at(std::size_t i) const noexcept
{
    return i < m_values.size() ? m_values[i] : CborValue::undefined_value();
}

const CborValue& CborArray::operator[](std::size_t i) const noexcept
{
    return at(i);
}

void CborArray::append(CborValue value)
{
    m_values.push_back(std::move(value));
}

void CborArray::reserve(std::size_t capacity)
{
    m_values.reserve(capacity);
}

CborArray::iterator CborArray::begin() noexcept
{
    return m_values.begin();
}

CborArray::iterator CborArray::end() noexcept
{
    return m_values.end();
}

CborArray::const_iterator CborArray::begin() const noexcept
{
    return m_values.begin();
}

CborArray::const_iterator CborArray::end() const noexcept
{
    return m_values.end();
}

bool CborArray::operator==(const CborArray& other) const
{
    return m_values == other.m_values;
}

CborArray CborArray::from_json_array(const JsonArray& array)
{
    CborArray result;
    result.reserve(array.size());
    for (const JsonValue& value : array) {
        result.append(CborValue::from_json_value(value));
    }
    return result;
}

CborArray CborArray::from_string_list(std::span<const std::string> list)
{
    CborArray result;
    result.reserve(list.size());
    for (const std::string& item : list) {
        result.append(item);
    }
    return result;
}

JsonArray CborArray::to_json_array() const
{
    JsonArray result;
    result.reserve(m_values.size());
    for (const CborValue& value : m_values) {
        result.append(value.to_json_value());
    }
    return result;
}

//=============================================================================================================
// CborMap
//=============================================================================================================

CborMap::CborMap(std::initializer_list<value_type> members)
: m_members(members)
{
}

std::size_t CborMap::size() const noexcept
{
    return m_members.size();
}

bool CborMap::empty() const noexcept
{
    return m_members.empty();
}

bool CborMap::contains(std::string_view key) const noexcept
{
    return std::ranges::any_of(m_members, [key](const value_type& member) { return member.first == key; });
}

const CborValue& CborMap::value(std::string_view key) const noexcept
{
    const auto it = std::ranges::find(m_members, key, &value_type::first);
    return it == m_members.end() ? CborValue::undefined_value() : it->second;
}

const CborValue& CborMap::operator[](std::string_view key) const noexcept
{
    return value(key);
}

CborValue& CborMap::operator[](std::string_view key)
{
    const auto it = std::ranges::find(m_members, key, &value_type::first);
    if (it != m_members.end()) {
        return it->second;
    }
    return m_members.emplace_back(std::string(key), CborValue()).second;
}

void CborMap::insert(std::string_view key, CborValue value)
{
    (*this)[key] = std::move(value);
}

void CborMap::append(std::string key, CborValue value)
{
    m_members.emplace_back(std::move(key), std::move(value));
}

void CborMap::remove(std::string_view key)
{
    std::erase_if(m_members, [key](const value_type& member) { return member.first == key; });
}

std::vector<std::string> CborMap::keys() const
{
    std::vector<std::string> keys;
    keys.reserve(m_members.size());
    for (const value_type& member : m_members) {
        keys.push_back(member.first);
    }
    return keys;
}

void CborMap::reserve(std::size_t capacity)
{
    m_members.reserve(capacity);
}

CborMap::iterator CborMap::begin() noexcept
{
    return m_members.begin();
}

CborMap::iterator CborMap::end() noexcept
{
    return m_members.end();
}

CborMap::const_iterator CborMap::begin() const noexcept
{
    return m_members.begin();
}

CborMap::const_iterator CborMap::end() const noexcept
{
    return m_members.end();
}

bool CborMap::operator==(const CborMap& other) const
{
    return m_members == other.m_members;
}

CborMap CborMap::from_json_object(const JsonObject& object)
{
    CborMap result;
    result.reserve(object.size());
    for (const auto& [key, value] : object) {
        result.append(key, CborValue::from_json_value(value));
    }
    return result;
}

JsonObject CborMap::to_json_object(std::initializer_list<std::string_view> except) const
{
    JsonObject result;
    result.reserve(m_members.size());
    for (const auto& [key, value] : m_members) {
        if (std::ranges::find(except, std::string_view(key)) == except.end()) {
            result.insert(key, value.to_json_value());
        }
    }
    return result;
}

//=============================================================================================================
// CborValue
//=============================================================================================================

CborValue::CborValue(std::nullptr_t) noexcept
: m_value(nullptr)
{
}

CborValue::CborValue(bool value) noexcept
: m_value(value)
{
}

CborValue::CborValue(const char* value)
: m_value(std::string(value))
{
}

CborValue::CborValue(std::string_view value)
: m_value(std::string(value))
{
}

CborValue::CborValue(std::string value) noexcept
: m_value(std::move(value))
{
}

CborValue::CborValue(CborByteArray value) noexcept
: m_value(std::move(value))
{
}

CborValue::CborValue(CborArray value) noexcept
: m_value(std::move(value))
{
}

CborValue::CborValue(CborMap value) noexcept
: m_value(std::move(value))
{
}

CborValue CborValue::tagged(std::uint64_t tag, CborValue value)
{
    CborValue result;
    result.m_value = Tagged{tag, CborArray{std::move(value)}};
    return result;
}

CborValue::Type CborValue::type() const noexcept
{
    static constexpr Type types[] = {Undefined, Null, False, Integer, Double, String, ByteArray, Array, Map, Tag};
    if (const bool* b = std::get_if<bool>(&m_value)) {
        return *b ? True : False;
    }
    return types[m_value.index()];
}

bool CborValue::is_integer() const noexcept
{
    return std::holds_alternative<std::int64_t>(m_value);
}

bool CborValue::is_byte_array() const noexcept
{
    return std::holds_alternative<CborByteArray>(m_value);
}

bool CborValue::is_string() const noexcept
{
    return std::holds_alternative<std::string>(m_value);
}

bool CborValue::is_array() const noexcept
{
    return std::holds_alternative<CborArray>(m_value);
}

bool CborValue::is_map() const noexcept
{
    return std::holds_alternative<CborMap>(m_value);
}

bool CborValue::is_tag() const noexcept
{
    return std::holds_alternative<Tagged>(m_value);
}

bool CborValue::is_bool() const noexcept
{
    return std::holds_alternative<bool>(m_value);
}

bool CborValue::is_null() const noexcept
{
    return std::holds_alternative<std::nullptr_t>(m_value);
}

bool CborValue::is_undefined() const noexcept
{
    return std::holds_alternative<UndefinedTag>(m_value);
}

bool CborValue::is_double() const noexcept
{
    return std::holds_alternative<double>(m_value);
}

bool CborValue::to_bool(bool defaultValue) const noexcept
{
    const bool* value = std::get_if<bool>(&m_value);
    return value ? *value : defaultValue;
}

std::int64_t CborValue::to_integer(std::int64_t defaultValue) const noexcept
{
    const std::int64_t* value = std::get_if<std::int64_t>(&m_value);
    return value ? *value : defaultValue;
}

double CborValue::to_double(double defaultValue) const noexcept
{
    if (const double* value = std::get_if<double>(&m_value)) {
        return *value;
    }
    if (const std::int64_t* value = std::get_if<std::int64_t>(&m_value)) {
        return static_cast<double>(*value);
    }
    return defaultValue;
}

std::string_view CborValue::to_string(std::string_view defaultValue) const noexcept
{
    const std::string* value = std::get_if<std::string>(&m_value);
    return value ? std::string_view(*value) : defaultValue;
}

const CborByteArray& CborValue::to_byte_array() const noexcept
{
    static const CborByteArray empty;
    const CborByteArray* value = std::get_if<CborByteArray>(&m_value);
    return value ? *value : empty;
}

const CborArray& CborValue::to_array() const noexcept
{
    static const CborArray empty;
    const CborArray* value = std::get_if<CborArray>(&m_value);
    return value ? *value : empty;
}

const CborMap& CborValue::to_map() const noexcept
{
    static const CborMap empty;
    const CborMap* value = std::get_if<CborMap>(&m_value);
    return value ? *value : empty;
}

std::uint64_t CborValue::tag(std::uint64_t defaultValue) const noexcept
{
    const Tagged* value = std::get_if<Tagged>(&m_value);
    return value ? value->tag : defaultValue;
}

const CborValue& CborValue::tagged_value() const noexcept
{
    const Tagged* value = std::get_if<Tagged>(&m_value);
    return value ? value->value.at(0) : undefined_value();
}

const CborValue& CborValue::operator[](std::string_view key) const noexcept
{
    return to_map().value(key);
}

const CborValue& CborValue::operator[](std::size_t i) const noexcept
{
    return to_array().at(i);
}

bool CborValue::operator==(const CborValue& other) const
{
    return m_value == other.m_value;
}

CborValue CborValue::from_json_value(const JsonValue& value)
{
    switch (value.type()) {
        case JsonValue::Null:
            return nullptr;
        case JsonValue::Bool:
            return value.to_bool();
        case JsonValue::Double:
            return value.is_integer() ? CborValue(value.to_integer()) : CborValue(value.to_double());
        case JsonValue::String:
            return value.to_string();
        case JsonValue::Array:
            return CborArray::from_json_array(value.to_array());
        case JsonValue::Object:
            return CborMap::from_json_object(value.to_object());
        case JsonValue::Undefined:
            break;
    }
    return {};
}

JsonValue CborValue::to_json_value() const
{
    switch (type()) {
        case Integer:
            return to_integer();
        case ByteArray:
            return to_base64(to_byte_array(), true);
        case String:
            return to_string();
        case Array:
            return to_array().to_json_array();
        case Map:
            return to_map().to_json_object();
        case Tag: {
            const CborValue& content = tagged_value();
            if (content.is_byte_array() && (tag() == 22 || tag() == 23)) {
                return tag() == 22 ? to_base64(content.to_byte_array()) : to_hex(content.to_byte_array());
            }
            return content.to_json_value();
        }
        case False:
            return false;
        case True:
            return true;
        case Null:
        case Undefined:
            return nullptr;
        case Double:
            return to_double();
    }
    return nullptr;
}

CborByteArray CborValue::to_cbor() const
{
    CborByteArray out;
    Encoder(out).value(*this);
    return out;
}

CborValue CborValue::from_cbor(std::span<const std::byte> data, CborParseError* error)
{
    Decoder decoder(data);
    CborValue value = decoder.value(0);
    if (!decoder.failed() && decoder.pos() != data.size()) {
        decoder.error = {decoder.pos(), CborParseError::GarbageAtEnd};
    }
    if (error) {
        *error = decoder.error;
    }
    return decoder.failed() ? CborValue() : value;
}

const CborValue& CborValue::undefined_value() noexcept
{
    static const CborValue undefined;
    return undefined;
}
