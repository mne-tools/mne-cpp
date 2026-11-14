//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     json.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    JSON values, arrays, objects and documents in standard C++, the counterpart of Qt's JSON classes.
 *
 * JsonValue, JsonArray, JsonObject, JsonDocument and JsonParseError follow QJsonValue, QJsonArray, QJsonObject,
 * QJsonDocument and QJsonParseError call for call, so porting is mostly a rename. The differences are:
 *  - Text is UTF-8: strings are std::string and std::string_view instead of QString.
 *  - Getters that return arrays, objects and strings return references and views instead of copies; they stay
 *    valid while the value they come from is neither changed nor destroyed.
 *  - Integers are kept as std::int64_t and doubles as double, so 64-bit integers keep every digit. As in Qt 6,
 *    type() reports Double and is_double() is true for both; is_integer() tells them apart.
 *  - JsonObject keeps its members in a vector sorted by key, which gives Qt's key order, O(log n) lookup and one
 *    allocation per object instead of one per member. Its elements expose key() and value(), and they also
 *    decompose into structured bindings: for (const auto& [key, value] : object).
 *
 * Nothing throws on malformed input or missing data: parsing reports a JsonParseError, and a missing key or an
 * index out of range yields an undefined value, like Qt.
 *
 * Incremental parsing of streams, e.g. NDJSON from a socket, is in base/json_stream_parser.h.
 */

#ifndef BASE_JSON_H
#define BASE_JSON_H

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
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE BASELIB
//=============================================================================================================

namespace BASELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class JsonValue;
class JsonDocument;
struct JsonAccess;

//=============================================================================================================
/**
 * @brief Reports where and why parsing JSON failed, like QJsonParseError.
 *
 * @snippet ex_base/main.cpp json_usage
 */
struct BASESHARED_EXPORT JsonParseError
{
    /** The reasons parsing can fail, with the same names and values as QJsonParseError::ParseError. */
    enum ParseError
    {
        NoError = 0,           /**< No error occurred. */
        UnterminatedObject,    /**< An object is not closed with '}'. */
        MissingNameSeparator,  /**< A ':' is missing after a key. */
        UnterminatedArray,     /**< An array is not closed with ']'. */
        MissingValueSeparator, /**< A ',' is missing between two array elements. */
        IllegalValue,          /**< The value is not JSON, e.g. a misspelled literal. */
        TerminationByNumber,   /**< The input ended inside a number. */
        IllegalNumber,         /**< The number is malformed or out of the range of double. */
        IllegalEscapeSequence, /**< A string holds an unknown escape sequence. */
        IllegalUTF8String,     /**< A string is not valid UTF-8. */
        UnterminatedString,    /**< A string is not closed with '"'. */
        MissingObject,         /**< A value is missing after a ','. */
        DeepNesting,           /**< Arrays and objects are nested deeper than allowed. */
        DocumentTooLarge,      /**< The document or one of its tokens exceeds the allowed size. */
        GarbageAtEnd           /**< Something other than whitespace follows the document. */
    };

    //=========================================================================================================
    /**
     * @return A human-readable description of error, with Qt's wording, e.g. "unterminated string".
     */
    std::string_view error_string() const noexcept;

    std::size_t offset = 0;     /**< Byte offset in the input at which the error was found. */
    ParseError error = NoError; /**< What went wrong. */
};

//=============================================================================================================
/**
 * @brief An ordered list of JSON values, like QJsonArray.
 *
 * Indexing out of range is safe: the const accessors return an undefined value, and writes through the
 * non-const operator[] are dropped.
 *
 * @snippet ex_base/main.cpp json_usage
 */
class BASESHARED_EXPORT JsonArray
{
public:
    using value_type = JsonValue;
    using size_type = std::size_t;
    using iterator = std::vector<JsonValue>::iterator;
    using const_iterator = std::vector<JsonValue>::const_iterator;

    JsonArray() = default;

    /** @param[in] values   The elements, e.g. JsonArray{1, 2.5, "three"}. */
    JsonArray(std::initializer_list<JsonValue> values);

    /** @return The number of elements. */
    std::size_t size() const noexcept;

    /** @return The number of elements, like QJsonArray::count(). */
    std::size_t count() const noexcept;

    /** @return Whether there are no elements. */
    bool is_empty() const noexcept;

    /** @return Whether there are no elements, for generic code. */
    bool empty() const noexcept;

    //=========================================================================================================
    /**
     * @param[in] i   Index of an element.
     *
     * @return The element at @p i, or an undefined value if @p i is out of range.
     */
    const JsonValue& at(std::size_t i) const noexcept;

    /** @copydoc at() */
    const JsonValue& operator[](std::size_t i) const noexcept;

    //=========================================================================================================
    /**
     * @param[in] i   Index of an element.
     *
     * @return The element at @p i to be changed in place; out of range, a scratch value whose changes are dropped.
     */
    JsonValue& operator[](std::size_t i) noexcept;

    /** @return The first element, or an undefined value if the array is empty. */
    const JsonValue& first() const noexcept;

    /** @return The last element, or an undefined value if the array is empty. */
    const JsonValue& last() const noexcept;

    /** @param[in] value   Element added at the end. */
    void append(JsonValue value);

    /** @param[in] value   Element added at the end, for generic code. */
    void push_back(JsonValue value);

    /** @param[in] value   Element added at the front. */
    void prepend(JsonValue value);

    //=========================================================================================================
    /**
     * @param[in] i       Index the new element gets; clamped to size().
     * @param[in] value   The new element.
     */
    void insert(std::size_t i, JsonValue value);

    //=========================================================================================================
    /**
     * @param[in] i       Index of the element to replace; nothing happens if it is out of range.
     * @param[in] value   The new element.
     */
    void replace(std::size_t i, JsonValue value);

    /** @param[in] i   Index of the element to remove; nothing happens if it is out of range. */
    void remove_at(std::size_t i);

    /** Removes the first element, if any. */
    void remove_first();

    /** Removes the last element, if any. */
    void remove_last();

    //=========================================================================================================
    /**
     * @param[in] i   Index of the element to remove.
     *
     * @return The removed element, or an undefined value if @p i is out of range.
     */
    JsonValue take_at(std::size_t i);

    /** @param[in] value   A value. @return Whether an element equals @p value. */
    bool contains(const JsonValue& value) const;

    /** @param[in] capacity   Number of elements to allocate room for. */
    void reserve(std::size_t capacity);

    /** Removes all elements. */
    void clear() noexcept;

    /** @param[in] value   Element added at the end. @return This array. */
    JsonArray& operator+=(JsonValue value);

    /** @param[in] value   Element added at the end. @return This array. */
    JsonArray& operator<<(JsonValue value);

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    /** @param[in] other   Another array. @return Whether both hold equal elements in the same order. */
    bool operator==(const JsonArray& other) const;

private:
    std::vector<JsonValue> m_values; /**< The elements. */
};

//=============================================================================================================
/**
 * @brief A set of JSON values keyed by unique strings, like QJsonObject.
 *
 * The members live in one vector sorted by key: lookups are binary searches without allocating, and iteration
 * visits the keys in byte order, as in Qt. Inserting a new key moves the members after it, so build large
 * objects through JsonDocument::from_json() or in key order where that matters.
 *
 * @snippet ex_base/main.cpp json_usage
 */
class BASESHARED_EXPORT JsonObject
{
public:
    class Member;

    using value_type = Member;
    using size_type = std::size_t;
    using iterator = std::vector<Member>::iterator;
    using const_iterator = std::vector<Member>::const_iterator;

    JsonObject() = default;

    /** @param[in] members   Key-value pairs, e.g. JsonObject{{"name", "MEG 0113"}, {"kind", 1}}; later duplicates win. */
    JsonObject(std::initializer_list<std::pair<std::string_view, JsonValue>> members);

    /** @return The number of members. */
    std::size_t size() const noexcept;

    /** @return The number of members, like QJsonObject::count(). */
    std::size_t count() const noexcept;

    /** @return The number of members, like QJsonObject::length(). */
    std::size_t length() const noexcept;

    /** @return Whether there are no members. */
    bool is_empty() const noexcept;

    /** @return Whether there are no members, for generic code. */
    bool empty() const noexcept;

    /** @return The keys in ascending byte order. */
    std::vector<std::string> keys() const;

    /** @param[in] key   A key. @return Whether the object has a member called @p key. */
    bool contains(std::string_view key) const noexcept;

    //=========================================================================================================
    /**
     * @param[in] key   A key.
     *
     * @return The value of the member called @p key, or an undefined value if there is none.
     */
    const JsonValue& value(std::string_view key) const noexcept;

    /** @copydoc value() */
    const JsonValue& operator[](std::string_view key) const noexcept;

    //=========================================================================================================
    /**
     * @param[in] key   A key.
     *
     * @return The value of the member called @p key to be changed in place; a null member is added if there is none.
     */
    JsonValue& operator[](std::string_view key);

    //=========================================================================================================
    /**
     * Sets the member called @p key, replacing an existing value.
     *
     * @param[in] key     A key.
     * @param[in] value   The value.
     *
     * @return The member.
     */
    iterator insert(std::string_view key, JsonValue value);

    /** @param[in] key   Key of the member to remove; nothing happens if there is none. */
    void remove(std::string_view key);

    //=========================================================================================================
    /**
     * @param[in] key   Key of the member to remove.
     *
     * @return The value of the removed member, or an undefined value if there is none.
     */
    JsonValue take(std::string_view key);

    /** @param[in] key   A key. @return The member called @p key, or end(). */
    iterator find(std::string_view key) noexcept;

    /** @param[in] key   A key. @return The member called @p key, or end(). */
    const_iterator find(std::string_view key) const noexcept;

    /** @param[in] key   A key. @return The member called @p key, or const_end(). */
    const_iterator const_find(std::string_view key) const noexcept;

    /** @param[in] it   A member of this object. @return The member after it. */
    iterator erase(const_iterator it);

    /** @param[in] capacity   Number of members to allocate room for. */
    void reserve(std::size_t capacity);

    /** Removes all members. */
    void clear() noexcept;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;
    const_iterator const_begin() const noexcept;
    const_iterator const_end() const noexcept;

    /** @param[in] other   Another object. @return Whether both hold the same keys with equal values. */
    bool operator==(const JsonObject& other) const;

private:
    friend struct JsonAccess;

    //=========================================================================================================
    /**
     * Sorts members appended out of order by key, keeping the last of duplicate keys, as the parser needs.
     */
    void sort_members();

    std::vector<Member> m_members; /**< The members, sorted by key. */
};

//=============================================================================================================
/**
 * @brief A JSON value: null, a boolean, a number, a string, an array or an object, like QJsonValue.
 *
 * Numbers stay integers when they are given or parsed as integers that fit std::int64_t. Strings, arrays and
 * objects are held in place, so short strings need no allocation at all.
 *
 * @snippet ex_base/main.cpp json_usage
 */
class BASESHARED_EXPORT JsonValue
{
public:
    /** The JSON type, with the names and values of QJsonValue::Type; integers report Double, as in Qt 6. */
    enum Type
    {
        Null = 0x0,      /**< null, also the default. */
        Bool = 0x1,      /**< true or false. */
        Double = 0x2,    /**< A number, integer or not. */
        String = 0x3,    /**< UTF-8 text. */
        Array = 0x4,     /**< A JsonArray. */
        Object = 0x5,    /**< A JsonObject. */
        Undefined = 0x80 /**< No value: a missing key, an index out of range or a wrong getter. */
    };

    /** @param[in] type   The type; the value is null, false, 0, empty or undefined accordingly. */
    JsonValue(Type type = Null);

    /** Constructs null. */
    JsonValue(std::nullptr_t) noexcept;

    /** @param[in] value   A boolean. */
    JsonValue(bool value) noexcept;

    /** @param[in] value   An integer; unsigned values beyond std::int64_t become doubles. */
    template<std::integral T>
    requires(!std::same_as<T, bool>)
        JsonValue(T value) noexcept;

    /** @param[in] value   A floating-point number; NaN and infinities are written as null. */
    template<std::floating_point T>
    JsonValue(T value) noexcept;

    /** @param[in] value   Null-terminated UTF-8 text. */
    JsonValue(const char* value);

    /** @param[in] value   UTF-8 text. */
    JsonValue(std::string_view value);

    /** @param[in] value   UTF-8 text. */
    JsonValue(const std::string& value);

    /** @param[in] value   UTF-8 text, moved in. */
    JsonValue(std::string&& value) noexcept;

    /** @param[in] value   An array. */
    JsonValue(const JsonArray& value);

    /** @param[in] value   An array, moved in. */
    JsonValue(JsonArray&& value) noexcept;

    /** @param[in] value   An object. */
    JsonValue(const JsonObject& value);

    /** @param[in] value   An object, moved in. */
    JsonValue(JsonObject&& value) noexcept;

    /** Pointers other than text would silently become booleans. */
    JsonValue(const void*) = delete;

    /** @return The type; integers report Double, as in Qt 6. */
    Type type() const noexcept;

    bool is_null() const noexcept;      /**< @return Whether the value is null. */
    bool is_bool() const noexcept;      /**< @return Whether the value is a boolean. */
    bool is_double() const noexcept;    /**< @return Whether the value is a number, integer or not, like QJsonValue::isDouble(). */
    bool is_number() const noexcept;    /**< @return Whether the value is a number, integer or not. */
    bool is_integer() const noexcept;   /**< @return Whether the value is a number held as std::int64_t. */
    bool is_string() const noexcept;    /**< @return Whether the value is a string. */
    bool is_array() const noexcept;     /**< @return Whether the value is an array. */
    bool is_object() const noexcept;    /**< @return Whether the value is an object. */
    bool is_undefined() const noexcept; /**< @return Whether the value is undefined. */

    /** @param[in] defaultValue   Fallback. @return The boolean, or @p defaultValue if the value is not one. */
    bool to_bool(bool defaultValue = false) const noexcept;

    //=========================================================================================================
    /**
     * @param[in] defaultValue   Fallback.
     *
     * @return The number if it is a whole number within the range of int, else @p defaultValue, like QJsonValue::toInt().
     */
    int to_int(int defaultValue = 0) const noexcept;

    //=========================================================================================================
    /**
     * @param[in] defaultValue   Fallback.
     *
     * @return The number if it is a whole number within the range of std::int64_t, else @p defaultValue,
     *         like QJsonValue::toInteger().
     */
    std::int64_t to_integer(std::int64_t defaultValue = 0) const noexcept;

    /** @param[in] defaultValue   Fallback. @return The number as double, or @p defaultValue if the value is not a number. */
    double to_double(double defaultValue = 0.0) const noexcept;

    //=========================================================================================================
    /**
     * @param[in] defaultValue   Fallback.
     *
     * @return A view of the string, valid while this value lives unchanged, or @p defaultValue if the value is
     *         not a string; numbers are not converted, as in Qt.
     */
    std::string_view to_string(std::string_view defaultValue = {}) const noexcept;

    /** @return A copy of the string, or empty text if the value is not a string. */
    std::string to_std_string() const;

    /** @return The array, or an empty array if the value is not one. */
    const JsonArray& to_array() const noexcept;

    /** @param[in] defaultValue   Fallback. @return A copy of the array, or @p defaultValue if the value is not one. */
    JsonArray to_array(const JsonArray& defaultValue) const;

    /** @return The object, or an empty object if the value is not one. */
    const JsonObject& to_object() const noexcept;

    /** @param[in] defaultValue   Fallback. @return A copy of the object, or @p defaultValue if the value is not one. */
    JsonObject to_object(const JsonObject& defaultValue) const;

    //=========================================================================================================
    /**
     * @param[in] key   A key.
     *
     * @return The member called @p key if the value is an object with one, else an undefined value.
     */
    const JsonValue& operator[](std::string_view key) const noexcept;

    //=========================================================================================================
    /**
     * @param[in] i   An index.
     *
     * @return The element at @p i if the value is an array that long, else an undefined value.
     */
    const JsonValue& operator[](std::size_t i) const noexcept;

    //=========================================================================================================
    /**
     * @param[in] other   Another value.
     *
     * @return Whether both have the same type and content; numbers compare by value, so 1 equals 1.0, as in Qt.
     */
    bool operator==(const JsonValue& other) const;

    /** @return The shared undefined value that lookups return when nothing is found. */
    static const JsonValue& undefined_value() noexcept;

private:
    friend struct JsonAccess;

    /** Alternative of an undefined value. */
    struct UndefinedTag
    {
        bool operator==(const UndefinedTag&) const = default;
    };

    /** The alternatives in the order of their indices. */
    using Storage = std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, JsonArray, JsonObject, UndefinedTag>;

    /** Indices of the alternatives of Storage. */
    enum Index : std::size_t
    {
        NullIndex,
        BoolIndex,
        IntegerIndex,
        DoubleIndex,
        StringIndex,
        ArrayIndex,
        ObjectIndex,
        UndefinedIndex
    };

    Storage m_value; /**< The value. */
};

//=============================================================================================================
/**
 * @brief A member of a JsonObject: a key that cannot change in place and a value that can.
 *
 * Members decompose into structured bindings: for (const auto& [key, value] : object).
 */
class BASESHARED_EXPORT JsonObject::Member
{
public:
    /** @return The key. */
    const std::string& key() const noexcept
    {
        return m_key;
    }

    /** @return The value. */
    const JsonValue& value() const noexcept
    {
        return m_value;
    }

    /** @return The value, to be changed in place. */
    JsonValue& value() noexcept
    {
        return m_value;
    }

    /** @return key() for I == 0 and value() for I == 1, for structured bindings. */
    template<std::size_t I>
    decltype(auto) get() const noexcept
    {
        static_assert(I < 2);
        if constexpr (I == 0) {
            return key();
        } else {
            return value();
        }
    }

    /** @return key() for I == 0 and value() for I == 1, for structured bindings. */
    template<std::size_t I>
    decltype(auto) get() noexcept
    {
        static_assert(I < 2);
        if constexpr (I == 0) {
            return key();
        } else {
            return value();
        }
    }

    /** @param[in] other   Another member. @return Whether key and value are equal. */
    bool operator==(const Member& other) const
    {
        return m_key == other.m_key && m_value == other.m_value;
    }

private:
    friend class JsonObject;
    friend struct JsonAccess;

    Member(std::string key, JsonValue value) noexcept
    : m_key(std::move(key))
    , m_value(std::move(value))
    {
    }

    std::string m_key; /**< The key. */
    JsonValue m_value; /**< The value. */
};

//=============================================================================================================
/**
 * @brief A JSON document whose root is an array or an object, parsed from or written as UTF-8 text, like QJsonDocument.
 *
 * @snippet ex_base/main.cpp json_usage
 */
class BASESHARED_EXPORT JsonDocument
{
public:
    /** The layouts of to_json(), like QJsonDocument::JsonFormat. */
    enum JsonFormat
    {
        Indented, /**< One value per line, indented by four spaces, ending in a newline, as Qt writes it. */
        Compact   /**< No whitespace at all. */
    };

    /** Constructs a null document. */
    JsonDocument() = default;

    /** @param[in] array   The root array. */
    explicit JsonDocument(JsonArray array) noexcept;

    /** @param[in] object   The root object. */
    explicit JsonDocument(JsonObject object) noexcept;

    //=========================================================================================================
    /**
     * Parses a whole document like QJsonDocument::fromJson(): the root must be an object or an array and only
     * whitespace may follow it. A UTF-8 byte order mark is skipped. Nesting is limited to 1024 levels, as in Qt.
     *
     * @param[in]  json    UTF-8 text.
     * @param[out] error   Receives the outcome if not null; error is NoError on success.
     *
     * @return The document, or a null document if @p json is not valid JSON.
     */
    static JsonDocument from_json(std::string_view json, JsonParseError* error = nullptr);

    //=========================================================================================================
    /**
     * @param[in] format   Layout; Indented, the default as in Qt, matches QJsonDocument::toJson() byte for byte.
     *
     * @return The document as UTF-8 text, or empty text for a null document.
     */
    std::string to_json(JsonFormat format = Indented) const;

    /** @return Whether the document has neither an array nor an object, e.g. after a parse error. */
    bool is_null() const noexcept;

    /** @return Whether the document has neither an array nor an object, like QJsonDocument::isEmpty(). */
    bool is_empty() const noexcept;

    /** @return Whether the root is an array. */
    bool is_array() const noexcept;

    /** @return Whether the root is an object. */
    bool is_object() const noexcept;

    /** @return The root array, or an empty array if the root is not one. */
    const JsonArray& array() const noexcept;

    /** @return The root object, or an empty object if the root is not one. */
    const JsonObject& object() const noexcept;

    /** @param[in] array   The new root array. */
    void set_array(JsonArray array) noexcept;

    /** @param[in] object   The new root object. */
    void set_object(JsonObject object) noexcept;

    /** @param[in] key   A key. @return The member of the root object called @p key, or an undefined value. */
    const JsonValue& operator[](std::string_view key) const noexcept;

    /** @param[in] i   An index. @return The element @p i of the root array, or an undefined value. */
    const JsonValue& operator[](std::size_t i) const noexcept;

    /** @param[in] other   Another document. @return Whether both have equal roots. */
    bool operator==(const JsonDocument& other) const;

private:
    JsonValue m_root; /**< The root: an array, an object, or null for a null document. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template<std::integral T>
requires(!std::same_as<T, bool>) inline JsonValue::JsonValue(T value) noexcept
{
    if constexpr (std::is_unsigned_v<T> && sizeof(T) >= sizeof(std::int64_t)) {
        if (value > static_cast<T>(std::numeric_limits<std::int64_t>::max())) {
            m_value.template emplace<double>(static_cast<double>(value));
            return;
        }
    }
    m_value.template emplace<std::int64_t>(static_cast<std::int64_t>(value));
}

//=============================================================================================================

template<std::floating_point T>
inline JsonValue::JsonValue(T value) noexcept
: m_value(static_cast<double>(value))
{
}

//=============================================================================================================

inline JsonValue::JsonValue(std::nullptr_t) noexcept
{
}

//=============================================================================================================

inline JsonValue::JsonValue(bool value) noexcept
: m_value(value)
{
}

//=============================================================================================================

inline JsonValue::JsonValue(const char* value)
: m_value(std::in_place_type<std::string>, value ? value : "")
{
}

//=============================================================================================================

inline JsonValue::JsonValue(std::string_view value)
: m_value(std::in_place_type<std::string>, value)
{
}

//=============================================================================================================

inline JsonValue::JsonValue(const std::string& value)
: m_value(std::in_place_type<std::string>, value)
{
}

//=============================================================================================================

inline JsonValue::JsonValue(std::string&& value) noexcept
: m_value(std::in_place_type<std::string>, std::move(value))
{
}

//=============================================================================================================

inline JsonValue::JsonValue(const JsonArray& value)
: m_value(std::in_place_type<JsonArray>, value)
{
}

//=============================================================================================================

inline JsonValue::JsonValue(JsonArray&& value) noexcept
: m_value(std::in_place_type<JsonArray>, std::move(value))
{
}

//=============================================================================================================

inline JsonValue::JsonValue(const JsonObject& value)
: m_value(std::in_place_type<JsonObject>, value)
{
}

//=============================================================================================================

inline JsonValue::JsonValue(JsonObject&& value) noexcept
: m_value(std::in_place_type<JsonObject>, std::move(value))
{
}

//=============================================================================================================

inline JsonValue::Type JsonValue::type() const noexcept
{
    switch (m_value.index()) {
        case NullIndex:
            return Null;
        case BoolIndex:
            return Bool;
        case IntegerIndex:
        case DoubleIndex:
            return Double;
        case StringIndex:
            return String;
        case ArrayIndex:
            return Array;
        case ObjectIndex:
            return Object;
        default:
            return Undefined;
    }
}

//=============================================================================================================

inline bool JsonValue::is_null() const noexcept
{
    return m_value.index() == NullIndex;
}

inline bool JsonValue::is_bool() const noexcept
{
    return m_value.index() == BoolIndex;
}

inline bool JsonValue::is_double() const noexcept
{
    return is_number();
}

inline bool JsonValue::is_number() const noexcept
{
    return m_value.index() == IntegerIndex || m_value.index() == DoubleIndex;
}

inline bool JsonValue::is_integer() const noexcept
{
    return m_value.index() == IntegerIndex;
}

inline bool JsonValue::is_string() const noexcept
{
    return m_value.index() == StringIndex;
}

inline bool JsonValue::is_array() const noexcept
{
    return m_value.index() == ArrayIndex;
}

inline bool JsonValue::is_object() const noexcept
{
    return m_value.index() == ObjectIndex;
}

inline bool JsonValue::is_undefined() const noexcept
{
    return m_value.index() == UndefinedIndex;
}

//=============================================================================================================

inline bool JsonValue::to_bool(bool defaultValue) const noexcept
{
    const bool* value = std::get_if<bool>(&m_value);
    return value ? *value : defaultValue;
}

//=============================================================================================================

inline double JsonValue::to_double(double defaultValue) const noexcept
{
    if (const double* value = std::get_if<double>(&m_value)) {
        return *value;
    }
    if (const std::int64_t* value = std::get_if<std::int64_t>(&m_value)) {
        return static_cast<double>(*value);
    }
    return defaultValue;
}

//=============================================================================================================

inline std::string_view JsonValue::to_string(std::string_view defaultValue) const noexcept
{
    const std::string* value = std::get_if<std::string>(&m_value);
    return value ? std::string_view(*value) : defaultValue;
}

//=============================================================================================================

inline std::string JsonValue::to_std_string() const
{
    return std::string(to_string());
}

//=============================================================================================================

inline JsonArray JsonValue::to_array(const JsonArray& defaultValue) const
{
    return is_array() ? to_array() : defaultValue;
}

//=============================================================================================================

inline JsonObject JsonValue::to_object(const JsonObject& defaultValue) const
{
    return is_object() ? to_object() : defaultValue;
}

//=============================================================================================================

inline JsonArray::JsonArray(std::initializer_list<JsonValue> values)
: m_values(values)
{
}

inline std::size_t JsonArray::size() const noexcept
{
    return m_values.size();
}

inline std::size_t JsonArray::count() const noexcept
{
    return m_values.size();
}

inline bool JsonArray::is_empty() const noexcept
{
    return m_values.empty();
}

inline bool JsonArray::empty() const noexcept
{
    return m_values.empty();
}

inline const JsonValue& JsonArray::at(std::size_t i) const noexcept
{
    return i < m_values.size() ? m_values[i] : JsonValue::undefined_value();
}

inline const JsonValue& JsonArray::operator[](std::size_t i) const noexcept
{
    return at(i);
}

inline const JsonValue& JsonArray::first() const noexcept
{
    return at(0);
}

inline const JsonValue& JsonArray::last() const noexcept
{
    return m_values.empty() ? JsonValue::undefined_value() : m_values.back();
}

inline void JsonArray::append(JsonValue value)
{
    m_values.push_back(std::move(value));
}

inline void JsonArray::push_back(JsonValue value)
{
    m_values.push_back(std::move(value));
}

inline void JsonArray::prepend(JsonValue value)
{
    m_values.insert(m_values.begin(), std::move(value));
}

inline void JsonArray::insert(std::size_t i, JsonValue value)
{
    m_values.insert(m_values.begin() + static_cast<std::ptrdiff_t>(i < m_values.size() ? i : m_values.size()), std::move(value));
}

inline void JsonArray::replace(std::size_t i, JsonValue value)
{
    if (i < m_values.size()) {
        m_values[i] = std::move(value);
    }
}

inline void JsonArray::remove_at(std::size_t i)
{
    if (i < m_values.size()) {
        m_values.erase(m_values.begin() + static_cast<std::ptrdiff_t>(i));
    }
}

inline void JsonArray::remove_first()
{
    remove_at(0);
}

inline void JsonArray::remove_last()
{
    if (!m_values.empty()) {
        m_values.pop_back();
    }
}

inline void JsonArray::reserve(std::size_t capacity)
{
    m_values.reserve(capacity);
}

inline void JsonArray::clear() noexcept
{
    m_values.clear();
}

inline JsonArray& JsonArray::operator+=(JsonValue value)
{
    m_values.push_back(std::move(value));
    return *this;
}

inline JsonArray& JsonArray::operator<<(JsonValue value)
{
    m_values.push_back(std::move(value));
    return *this;
}

inline JsonArray::iterator JsonArray::begin() noexcept
{
    return m_values.begin();
}

inline JsonArray::iterator JsonArray::end() noexcept
{
    return m_values.end();
}

inline JsonArray::const_iterator JsonArray::begin() const noexcept
{
    return m_values.begin();
}

inline JsonArray::const_iterator JsonArray::end() const noexcept
{
    return m_values.end();
}

inline JsonArray::const_iterator JsonArray::cbegin() const noexcept
{
    return m_values.cbegin();
}

inline JsonArray::const_iterator JsonArray::cend() const noexcept
{
    return m_values.cend();
}

//=============================================================================================================

inline std::size_t JsonObject::size() const noexcept
{
    return m_members.size();
}

inline std::size_t JsonObject::count() const noexcept
{
    return m_members.size();
}

inline std::size_t JsonObject::length() const noexcept
{
    return m_members.size();
}

inline bool JsonObject::is_empty() const noexcept
{
    return m_members.empty();
}

inline bool JsonObject::empty() const noexcept
{
    return m_members.empty();
}

inline bool JsonObject::contains(std::string_view key) const noexcept
{
    return find(key) != m_members.end();
}

inline const JsonValue& JsonObject::value(std::string_view key) const noexcept
{
    const const_iterator it = find(key);
    return it != m_members.end() ? it->value() : JsonValue::undefined_value();
}

inline const JsonValue& JsonObject::operator[](std::string_view key) const noexcept
{
    return value(key);
}

inline JsonObject::const_iterator JsonObject::const_find(std::string_view key) const noexcept
{
    return find(key);
}

inline void JsonObject::reserve(std::size_t capacity)
{
    m_members.reserve(capacity);
}

inline void JsonObject::clear() noexcept
{
    m_members.clear();
}

inline JsonObject::iterator JsonObject::begin() noexcept
{
    return m_members.begin();
}

inline JsonObject::iterator JsonObject::end() noexcept
{
    return m_members.end();
}

inline JsonObject::const_iterator JsonObject::begin() const noexcept
{
    return m_members.begin();
}

inline JsonObject::const_iterator JsonObject::end() const noexcept
{
    return m_members.end();
}

inline JsonObject::const_iterator JsonObject::cbegin() const noexcept
{
    return m_members.cbegin();
}

inline JsonObject::const_iterator JsonObject::cend() const noexcept
{
    return m_members.cend();
}

inline JsonObject::const_iterator JsonObject::const_begin() const noexcept
{
    return m_members.cbegin();
}

inline JsonObject::const_iterator JsonObject::const_end() const noexcept
{
    return m_members.cend();
}

//=============================================================================================================

inline JsonDocument::JsonDocument(JsonArray array) noexcept
: m_root(std::move(array))
{
}

inline JsonDocument::JsonDocument(JsonObject object) noexcept
: m_root(std::move(object))
{
}

inline bool JsonDocument::is_null() const noexcept
{
    return !m_root.is_array() && !m_root.is_object();
}

inline bool JsonDocument::is_empty() const noexcept
{
    return is_null();
}

inline bool JsonDocument::is_array() const noexcept
{
    return m_root.is_array();
}

inline bool JsonDocument::is_object() const noexcept
{
    return m_root.is_object();
}

inline const JsonArray& JsonDocument::array() const noexcept
{
    return m_root.to_array();
}

inline const JsonObject& JsonDocument::object() const noexcept
{
    return m_root.to_object();
}

inline void JsonDocument::set_array(JsonArray array) noexcept
{
    m_root = JsonValue(std::move(array));
}

inline void JsonDocument::set_object(JsonObject object) noexcept
{
    m_root = JsonValue(std::move(object));
}

inline const JsonValue& JsonDocument::operator[](std::string_view key) const noexcept
{
    return m_root.to_object().value(key);
}

inline const JsonValue& JsonDocument::operator[](std::size_t i) const noexcept
{
    return m_root.to_array().at(i);
}

inline bool JsonDocument::operator==(const JsonDocument& other) const
{
    return m_root == other.m_root;
}

} // namespace BASELIB

//=============================================================================================================
// STRUCTURED BINDINGS
//=============================================================================================================

template<>
struct std::tuple_size<BASELIB::JsonObject::Member> : std::integral_constant<std::size_t, 2>
{
};

template<>
struct std::tuple_element<0, BASELIB::JsonObject::Member>
{
    using type = const std::string;
};

template<>
struct std::tuple_element<1, BASELIB::JsonObject::Member>
{
    using type = BASELIB::JsonValue;
};

#endif // BASE_JSON_H
