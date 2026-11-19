//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     json_stream_parser.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Push parser for JSON that arrives in pieces, e.g. NDJSON or concatenated documents from a socket.
 *
 * The parser is a state machine that takes input in chunks of any size, down to single bytes, and never needs
 * the whole document in memory. It either builds a JsonDocument per root object or array, or reports tokens to
 * a JsonHandler without building anything.
 */

#ifndef BASE_JSON_STREAM_PARSER_H
#define BASE_JSON_STREAM_PARSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"
#include "json.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Receives the tokens of a JSON stream from JsonStreamParser, in document order, without a DOM.
 *
 * Override the callbacks of interest; the others do nothing. Strings and keys are views that are valid only
 * during the call: they point into the chunk passed to JsonStreamParser::feed() when the token was complete and
 * unescaped in it, and into a reused buffer otherwise, so no callback allocates.
 * Callbacks must not call feed(), finish() or reset() of the parser that calls them.
 *
 * @snippet ex_base/main.cpp json_stream_parser_usage
 */
class BASESHARED_EXPORT JsonHandler
{
public:
    virtual ~JsonHandler() = default;

    /** An object starts: '{'. */
    virtual void on_start_object()
    {
    }

    /** The innermost object ends: '}'. */
    virtual void on_end_object()
    {
    }

    /** An array starts: '['. */
    virtual void on_start_array()
    {
    }

    /** The innermost array ends: ']'. */
    virtual void on_end_array()
    {
    }

    /** @param[in] key   Key of the next member of the innermost object, unescaped UTF-8. */
    virtual void on_key(std::string_view key)
    {
        static_cast<void>(key);
    }

    /** @param[in] value   A string value, unescaped UTF-8. */
    virtual void on_string(std::string_view value)
    {
        static_cast<void>(value);
    }

    /** @param[in] value   A number written without fraction and exponent that fits std::int64_t; calls on_number() unless overridden. */
    virtual void on_integer(std::int64_t value)
    {
        on_number(static_cast<double>(value));
    }

    /** @param[in] value   Any other number. */
    virtual void on_number(double value)
    {
        static_cast<void>(value);
    }

    /** @param[in] value   true or false. */
    virtual void on_bool(bool value)
    {
        static_cast<void>(value);
    }

    /** null. */
    virtual void on_null()
    {
    }

    /** The root object or array closed: one document of the stream is complete. */
    virtual void on_end_document()
    {
    }
};

//=============================================================================================================
/**
 * @brief Incremental JSON parser for real-time streams: feed bytes as they arrive, get documents as they close.
 *
 * The stream holds root objects or arrays back to back, with or without whitespace in between, which covers
 * NDJSON (one document per line) and concatenated JSON ({"event":1}{"event":2}). Every root that closes is either
 *  - queued for take_document(), the default,
 *  - passed to the callback set with set_document_callback(), or
 *  - reported token by token to the JsonHandler set with set_handler(), without building any JsonDocument.
 *
 * Memory is bounded by Options: nesting depth, the bytes of one string or number held between chunks, and the
 * bytes of one document. On malformed input feed() returns false, error() tells why and where, and the parser
 * ignores input until reset(). Nothing throws, apart from std::bad_alloc and exceptions thrown by callbacks.
 *
 * @snippet ex_base/main.cpp json_stream_parser_usage
 */
class BASESHARED_EXPORT JsonStreamParser
{
public:
    /** Limits and stream layout. */
    struct Options
    {
        std::size_t maxDepth = 64;              /**< Deepest nesting of arrays and objects; deeper input fails with DeepNesting. */
        std::size_t maxBufferSize = 1 << 20;    /**< Most bytes of one string or number the parser copies, i.e. one that spans chunks or has escapes; more fails with DocumentTooLarge. */
        std::size_t maxDocumentSize = 64 << 20; /**< Most bytes of one document from its '{' or '[' on, checked at every '{' and '[' and at the end of each chunk; more fails with DocumentTooLarge. */
        bool singleDocument = false;            /**< Whether anything but whitespace after the first document fails with GarbageAtEnd, as for JsonDocument::from_json(). */
    };

    /** Receives each document as soon as its root closes. */
    using DocumentCallback = std::function<void(JsonDocument&& document)>;

    /** Constructs a parser with the default Options. */
    JsonStreamParser();

    /** @param[in] options   Limits and stream layout. */
    explicit JsonStreamParser(const Options& options);

    ~JsonStreamParser();

    JsonStreamParser(const JsonStreamParser&) = delete;
    JsonStreamParser& operator=(const JsonStreamParser&) = delete;
    JsonStreamParser(JsonStreamParser&&) noexcept;
    JsonStreamParser& operator=(JsonStreamParser&&) noexcept;

    //=========================================================================================================
    /**
     * Passes completed documents to @p callback instead of queueing them; call it between documents.
     *
     * @param[in] callback   Receives each document; an empty function queues documents again.
     */
    void set_document_callback(DocumentCallback callback);

    //=========================================================================================================
    /**
     * Reports tokens to @p handler instead of building documents; call it between documents.
     *
     * @param[in] handler   Receives the tokens and must outlive its use; nullptr builds documents again.
     */
    void set_handler(JsonHandler* handler) noexcept;

    //=========================================================================================================
    /**
     * Parses the next piece of the stream; tokens may be split anywhere, even inside a UTF-8 sequence.
     *
     * @param[in] chunk   The next bytes of the stream, of any length; they need not stay valid after the call.
     *
     * @return Whether the stream is still valid JSON; false once an error occurred, until reset().
     */
    bool feed(std::string_view chunk);

    /** @copydoc feed(std::string_view) */
    bool feed(std::span<const std::byte> chunk);

    //=========================================================================================================
    /**
     * Ends the stream: input that stops inside a document is an error, e.g. UnterminatedObject.
     * The parser stays usable for a new stream if this succeeds.
     *
     * @return Whether the stream ended between documents.
     */
    bool finish();

    //=========================================================================================================
    /**
     * Clears the error and any partial document so that parsing restarts with the next byte fed, e.g. after a
     * corrupt line of NDJSON; the options, handler, callback and the queue of complete documents are kept.
     */
    void reset();

    /** @return Whether an error occurred since construction or the last reset(). */
    bool has_error() const noexcept;

    /** @return The error, or NoError; offsets count the bytes fed since construction or the last reset(). */
    const JsonParseError& error() const noexcept;

    /** @return Whether complete documents are waiting in the queue. */
    bool has_document() const noexcept;

    /** @return The number of complete documents waiting in the queue. */
    std::size_t document_count() const noexcept;

    /** @return The oldest complete document from the queue, or a null document if there is none. */
    JsonDocument take_document();

    /** @return The number of bytes fed since construction or the last reset(). */
    std::size_t bytes_fed() const noexcept;

    /** @return The current nesting depth: 0 between documents. */
    std::size_t depth() const noexcept;

    /** @return Whether the parser is between documents, i.e. no document is partially parsed. */
    bool is_idle() const noexcept;

    /** @return The limits and stream layout. */
    const Options& options() const noexcept;

private:
    class DocumentBuilder;

    /** Where the parser is in the grammar. */
    enum class State : std::uint8_t
    {
        Root,               /**< Between documents: whitespace or '{' or '['. */
        Done,               /**< After the document with Options::singleDocument: whitespace only. */
        Value,              /**< A value after ':' or after ',' in an array. */
        ValueOrEndArray,    /**< A value or ']' after '['. */
        KeyOrEndObject,     /**< A key or '}' after '{'. */
        Key,                /**< A key after ',' in an object. */
        Colon,              /**< ':' after a key. */
        CommaOrEnd,         /**< ',' or the closing bracket after a value in an array or object. */
        String,             /**< Inside a string or key. */
        Escape,             /**< After '\\' in a string. */
        Unicode,            /**< Inside the four hex digits of \\uXXXX. */
        SurrogateBackslash, /**< After a high surrogate: '\\' of the low surrogate. */
        SurrogateU,         /**< After a high surrogate and '\\': 'u' of the low surrogate. */
        Number,             /**< Inside a number. */
        Literal,            /**< Inside true, false or null. */
        Error               /**< After an error, until reset(). */
    };

    bool fail(JsonParseError::ParseError error, std::size_t offset);
    bool open_container(bool object, std::size_t offset);
    void close_container();
    void end_value();
    bool append_token(const char* begin, const char* end);
    bool take_token(const char* end, std::string_view& token);
    bool emit_number(std::string_view text);
    bool append_code_point(std::uint32_t codePoint);
    bool decode_code_unit();
    bool accept_utf8(unsigned char byte) noexcept;

    Options m_options;                          /**< Limits and stream layout. */
    std::unique_ptr<DocumentBuilder> m_builder; /**< Builds documents unless a handler is set. */
    JsonHandler* m_handler = nullptr;           /**< Receives the tokens: the user's handler or m_builder. */
    DocumentCallback m_callback;                /**< Receives completed documents if set. */
    std::vector<JsonDocument> m_documents;      /**< Completed documents not taken yet, from m_firstDocument on. */
    std::size_t m_firstDocument = 0;            /**< Index of the oldest document not taken in m_documents. */

    std::vector<bool> m_containers;     /**< Open containers from the root inward: true for objects. */
    std::string m_token;                /**< A string or number spanning chunks or unescaped, reused. */
    JsonParseError m_error;             /**< The error, if any. */
    std::size_t m_offset = 0;           /**< Bytes fed before the current chunk. */
    std::size_t m_documentStart = 0;    /**< Offset of the root bracket of the current document. */
    const char* m_tokenStart = nullptr; /**< Start of the current token's bytes in the current chunk. */
    std::string_view m_literal;         /**< The literal being matched: "true", "false" or "null". */
    std::uint32_t m_codeUnit = 0;       /**< The UTF-16 code unit of \\uXXXX read so far. */
    std::uint32_t m_highSurrogate = 0;  /**< A high surrogate waiting for its low surrogate, or 0. */
    std::uint8_t m_literalPos = 0;      /**< Characters of m_literal matched. */
    std::uint8_t m_hexDigits = 0;       /**< Hex digits of \\uXXXX read. */
    std::uint8_t m_utf8Pending = 0;     /**< Continuation bytes still due in the current UTF-8 sequence. */
    std::uint8_t m_utf8Lower = 0x80;    /**< Smallest valid next continuation byte. */
    std::uint8_t m_utf8Upper = 0xBF;    /**< Largest valid next continuation byte. */
    State m_state = State::Root;        /**< Where the parser is. */
    bool m_inKey = false;               /**< Whether the current string is a key. */
    bool m_tokenBuffered = false;       /**< Whether part of the current token is in m_token. */
};

} // namespace mne

#endif // BASE_JSON_STREAM_PARSER_H
