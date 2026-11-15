//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     cbor.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    CBOR (RFC 8949) values, arrays and maps in standard C++, the counterpart of Qt's QCborValue, QCborArray
 *           and QCborMap.
 *
 * CborValue, CborArray and CborMap follow their Qt counterparts call for call, so porting is mostly a rename. The
 * differences are:
 *  - Text is UTF-8 in std::string, byte strings are std::vector<std::byte>.
 *  - Map keys are text. Decoding turns integer keys into their decimal text, as QCborMap::toJsonObject() does,
 *    and rejects other key types.
 *  - Simple values other than false, true, null and undefined are rejected when decoding.
 *
 * Encoding uses definite lengths and the shortest integer heads, and writes doubles as 64-bit floats, which gives
 * the same bytes as QCborValue::toCbor(). Decoding also accepts indefinite lengths and 16- and 32-bit floats.
 * Nothing throws on malformed input: decoding reports a CborParseError.
 */

#ifndef BASE_CBOR_H
#define BASE_CBOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class CborValue;
class JsonArray;
class JsonObject;
class JsonValue;

/** A CBOR byte string. */
using CborByteArray = std::vector<std::byte>;

//=============================================================================================================
/**
 * @brief Reports where and why decoding CBOR failed, like QCborParserError.
 *
 * @snippet ex_base/main.cpp cbor_usage
 */
struct BASESHARED_EXPORT CborParseError
{
    /** The reasons decoding can fail, with the names and values of QCborError::Code. */
    enum Code
    {
        NoError = 0,             /**< No error occurred. */
        GarbageAtEnd = 256,      /**< Bytes follow the value. */
        EndOfFile = 257,         /**< The input ended inside a value. */
        UnexpectedBreak = 258,   /**< A break code appeared outside an indefinite-length item. */
        UnknownType = 259,       /**< The major type or additional information is reserved. */
        IllegalType = 260,       /**< A chunk of an indefinite-length string has the wrong type. */
        IllegalNumber = 261,     /**< An indefinite length was used where it is not allowed. */
        IllegalSimpleType = 262, /**< A simple value other than false, true, null or undefined. */
        InvalidUtf8String = 516, /**< A text string is not valid UTF-8. */
        DataTooLarge = 1024,     /**< A length exceeds the input. */
        NestingTooDeep = 1025,   /**< Arrays, maps and tags are nested deeper than allowed. */
        UnsupportedType = 1026   /**< A map key that is neither text nor an integer. */
    };

    /** @return A human-readable description of the error. */
    std::string_view error_string() const noexcept;

    std::size_t offset = 0; /**< Byte offset in the input at which the error was found. */
    Code error = NoError;   /**< What went wrong. */
};

//=============================================================================================================
/**
 * @brief An ordered list of CBOR values, like QCborArray.
 *
 * Indexing out of range is safe: the const accessors return an undefined value.
 *
 * @snippet ex_base/main.cpp cbor_usage
 */
class BASESHARED_EXPORT CborArray
{
public:
    using value_type = CborValue;
    using iterator = std::vector<CborValue>::iterator;
    using const_iterator = std::vector<CborValue>::const_iterator;

    CborArray() = default;

    /** @param[in] values   The elements, e.g. CborArray{1, "two", CborByteArray{}}. */
    CborArray(std::initializer_list<CborValue> values);

    /** @return The number of elements. */
    std::size_t size() const noexcept;

    /** @return Whether there are no elements. */
    bool empty() const noexcept;

    /** @param[in] i   An index. @return The element at @p i, or an undefined value if out of range. */
    const CborValue& at(std::size_t i) const noexcept;

    /** @param[in] i   An index. @return The element at @p i, or an undefined value if out of range. */
    const CborValue& operator[](std::size_t i) const noexcept;

    /** @param[in] value   Appended at the end. */
    void append(CborValue value);

    /** @param[in] capacity   Number of elements to make room for. */
    void reserve(std::size_t capacity);

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    /** @param[in] other   Another array. @return Whether both hold equal elements in the same order. */
    bool operator==(const CborArray& other) const;

    /** @param[in] array   A JSON array. @return The same elements as CBOR, like QCborArray::fromJsonArray(). */
    static CborArray from_json_array(const JsonArray& array);

    /** @return The elements as JSON, converted as CborValue::to_json_value() does. */
    JsonArray to_json_array() const;

private:
    std::vector<CborValue> m_values; /**< The elements. */
};

//=============================================================================================================
/**
 * @brief A CBOR map from text keys to values, like QCborMap; keys keep their insertion order.
 *
 * Looking up a missing key with a const accessor returns an undefined value.
 *
 * @snippet ex_base/main.cpp cbor_usage
 */
class BASESHARED_EXPORT CborMap
{
public:
    using value_type = std::pair<std::string, CborValue>;
    using iterator = std::vector<value_type>::iterator;
    using const_iterator = std::vector<value_type>::const_iterator;

    CborMap() = default;

    /** @param[in] members   The members in order, e.g. CborMap{{"name", "x"}, {"size", 3}}. */
    CborMap(std::initializer_list<value_type> members);

    /** @return The number of members. */
    std::size_t size() const noexcept;

    /** @return Whether there are no members. */
    bool empty() const noexcept;

    /** @param[in] key   A key. @return Whether a member has that key. */
    bool contains(std::string_view key) const noexcept;

    /** @param[in] key   A key. @return The value of the first member with that key, or an undefined value. */
    const CborValue& value(std::string_view key) const noexcept;

    /** @param[in] key   A key. @return The value of the first member with that key, or an undefined value. */
    const CborValue& operator[](std::string_view key) const noexcept;

    /** @param[in] key   A key. @return The value of the member with that key, appended as undefined if missing. */
    CborValue& operator[](std::string_view key);

    /**
     * @param[in] key     A key.
     * @param[in] value   Replaces the value of the member with that key, or is appended with it.
     */
    void insert(std::string_view key, CborValue value);

    //=========================================================================================================
    /**
     * Appends a member without looking for one with the same key, which makes building a map of distinct keys
     * linear instead of quadratic.
     *
     * @param[in] key     A key not yet in the map.
     * @param[in] value   Its value.
     */
    void append(std::string key, CborValue value);

    /** @param[in] key   Every member with that key is removed. */
    void remove(std::string_view key);

    /** @return The keys in order. */
    std::vector<std::string> keys() const;

    /** @param[in] capacity   Number of members to make room for. */
    void reserve(std::size_t capacity);

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    /** @param[in] other   Another map. @return Whether both hold equal members in the same order. */
    bool operator==(const CborMap& other) const;

    /** @param[in] object   A JSON object. @return The same members as CBOR, like QCborMap::fromJsonObject(). */
    static CborMap from_json_object(const JsonObject& object);

    /** @return The members as JSON, converted as CborValue::to_json_value() does; later duplicate keys win. */
    JsonObject to_json_object() const;

private:
    std::vector<value_type> m_members; /**< The members in insertion order. */
};

//=============================================================================================================
/**
 * @brief A CBOR value: an integer, a byte string, text, an array, a map, a tagged value, a boolean, null,
 *        undefined or a double, like QCborValue.
 *
 * @snippet ex_base/main.cpp cbor_usage
 */
class BASESHARED_EXPORT CborValue
{
public:
    /** The CBOR type, with the names and values of QCborValue::Type. */
    enum Type
    {
        Integer = 0x00,    /**< A signed 64-bit integer. */
        ByteArray = 0x40,  /**< A byte string. */
        String = 0x60,     /**< UTF-8 text. */
        Array = 0x80,      /**< A CborArray. */
        Map = 0xa0,        /**< A CborMap. */
        Tag = 0xc0,        /**< A tag number and the value it applies to. */
        False = 0x114,     /**< false. */
        True = 0x115,      /**< true. */
        Null = 0x116,      /**< null. */
        Undefined = 0x117, /**< undefined, also the default and the result of failed lookups. */
        Double = 0x202     /**< A 64-bit float. */
    };

    /** Constructs undefined. */
    CborValue() noexcept = default;

    /** Constructs null. */
    CborValue(std::nullptr_t) noexcept;

    /** @param[in] value   A boolean. */
    CborValue(bool value) noexcept;

    /** @param[in] value   An integer; unsigned values beyond std::int64_t become doubles. */
    template<std::integral T>
    requires(!std::same_as<T, bool>)
        CborValue(T value) noexcept;

    /** @param[in] value   A floating-point number. */
    template<std::floating_point T>
    CborValue(T value) noexcept
    : m_value(static_cast<double>(value))
    {
    }

    /** @param[in] value   Null-terminated UTF-8 text. */
    CborValue(const char* value);

    /** @param[in] value   UTF-8 text. */
    CborValue(std::string_view value);

    /** @param[in] value   UTF-8 text, moved in. */
    CborValue(std::string value) noexcept;

    /** @param[in] value   A byte string, moved in. */
    CborValue(CborByteArray value) noexcept;

    /** @param[in] value   An array, moved in. */
    CborValue(CborArray value) noexcept;

    /** @param[in] value   A map, moved in. */
    CborValue(CborMap value) noexcept;

    /** Pointers other than text would silently become booleans. */
    CborValue(const void*) = delete;

    //=========================================================================================================
    /**
     * @param[in] tag     The tag number, e.g. 0 for a date-time string.
     * @param[in] value   The tagged value.
     *
     * @return The tagged value, like QCborValue(QCborTag, QCborValue).
     */
    static CborValue tagged(std::uint64_t tag, CborValue value);

    /** @return The type. */
    Type type() const noexcept;

    bool is_integer() const noexcept;    /**< @return Whether the value is an integer. */
    bool is_byte_array() const noexcept; /**< @return Whether the value is a byte string. */
    bool is_string() const noexcept;     /**< @return Whether the value is text. */
    bool is_array() const noexcept;      /**< @return Whether the value is an array. */
    bool is_map() const noexcept;        /**< @return Whether the value is a map. */
    bool is_tag() const noexcept;        /**< @return Whether the value is tagged. */
    bool is_bool() const noexcept;       /**< @return Whether the value is false or true. */
    bool is_null() const noexcept;       /**< @return Whether the value is null. */
    bool is_undefined() const noexcept;  /**< @return Whether the value is undefined. */
    bool is_double() const noexcept;     /**< @return Whether the value is a double. */

    /** @param[in] defaultValue   Fallback. @return The boolean, or @p defaultValue if the value is not one. */
    bool to_bool(bool defaultValue = false) const noexcept;

    /** @param[in] defaultValue   Fallback. @return The integer, or @p defaultValue if the value is not one. */
    std::int64_t to_integer(std::int64_t defaultValue = 0) const noexcept;

    /** @param[in] defaultValue   Fallback. @return The number as double, or @p defaultValue if it is not a number. */
    double to_double(double defaultValue = 0.0) const noexcept;

    /** @param[in] defaultValue   Fallback. @return A view of the text, or @p defaultValue if the value is not text. */
    std::string_view to_string(std::string_view defaultValue = {}) const noexcept;

    /** @return The byte string, or an empty one if the value is not a byte string. */
    const CborByteArray& to_byte_array() const noexcept;

    /** @return The array, or an empty array if the value is not one. */
    const CborArray& to_array() const noexcept;

    /** @return The map, or an empty map if the value is not one. */
    const CborMap& to_map() const noexcept;

    /** @param[in] defaultValue   Fallback. @return The tag number, or @p defaultValue if the value is not tagged. */
    std::uint64_t tag(std::uint64_t defaultValue = 0) const noexcept;

    /** @return The tagged value, or an undefined value if the value is not tagged. */
    const CborValue& tagged_value() const noexcept;

    /** @param[in] key   A key. @return The member called @p key if the value is a map with one, else undefined. */
    const CborValue& operator[](std::string_view key) const noexcept;

    /** @param[in] i   An index. @return The element at @p i if the value is an array that long, else undefined. */
    const CborValue& operator[](std::size_t i) const noexcept;

    /** @param[in] other   Another value. @return Whether both have the same type and content. */
    bool operator==(const CborValue& other) const;

    //=========================================================================================================
    /**
     * @param[in] value   A JSON value.
     *
     * @return The same value as CBOR, like QCborValue::fromJsonValue(): JSON integers become integers, other
     *         numbers doubles, and an undefined JSON value becomes undefined.
     */
    static CborValue from_json_value(const JsonValue& value);

    //=========================================================================================================
    /**
     * @return The value as JSON, like QCborValue::toJsonValue(): byte strings become base64url text without
     *         padding (base64 for tag 22, hex for tag 23), other tagged values the value they tag, undefined and
     *         non-finite doubles null.
     */
    JsonValue to_json_value() const;

    /** @return The value encoded as CBOR, with the same bytes as QCborValue::toCbor(). */
    CborByteArray to_cbor() const;

    //=========================================================================================================
    /**
     * @param[in]  data    Exactly one encoded value.
     * @param[out] error   Receives the outcome if not null.
     *
     * @return The decoded value, or undefined on error.
     */
    static CborValue from_cbor(std::span<const std::byte> data, CborParseError* error = nullptr);

    /** @return The shared undefined value that lookups return when nothing is found. */
    static const CborValue& undefined_value() noexcept;

private:
    friend struct CborAccess;

    /** Alternative of undefined. */
    struct UndefinedTag
    {
        bool operator==(const UndefinedTag&) const = default;
    };

    /** A tag number with its value, held in a one-element array so the value stays copyable. */
    struct Tagged
    {
        std::uint64_t tag = 0;
        CborArray value;
        bool operator==(const Tagged&) const = default;
    };

    using Storage = std::variant<UndefinedTag, std::nullptr_t, bool, std::int64_t, double, std::string, CborByteArray,
                                 CborArray, CborMap, Tagged>;

    Storage m_value; /**< The value. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template<std::integral T>
requires(!std::same_as<T, bool>)
    CborValue::CborValue(T value) noexcept
{
    if constexpr (std::is_unsigned_v<T> && sizeof(T) >= sizeof(std::int64_t)) {
        if (value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
            m_value.template emplace<double>(static_cast<double>(value));
            return;
        }
    }
    m_value.template emplace<std::int64_t>(static_cast<std::int64_t>(value));
}

} // namespace mne

#endif // BASE_CBOR_H
