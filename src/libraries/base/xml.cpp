//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     xml.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Definition of the XML reader: a non-recursive well-formedness checking parser.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xml.h"
#include "byte_stream.h"
#include "string_utils.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstdint>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_name_start(char c)
{
    const auto u = static_cast<unsigned char>(c);
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == ':' || u >= 0x80;
}

bool is_name_char(char c)
{
    return is_name_start(c) || (c >= '0' && c <= '9') || c == '-' || c == '.';
}

void append_utf8(std::string& out, std::uint32_t code)
{
    if (code < 0x80) {
        out += static_cast<char>(code);
    } else if (code < 0x800) {
        out += static_cast<char>(0xC0 | (code >> 6));
        out += static_cast<char>(0x80 | (code & 0x3F));
    } else if (code < 0x10000) {
        out += static_cast<char>(0xE0 | (code >> 12));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (code & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (code >> 18));
        out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (code & 0x3F));
    }
}

/** The value of pseudo-attribute @p key in an XML declaration, empty if it has none. */
std::string_view declared(std::string_view declaration, std::string_view key)
{
    std::size_t at = declaration.find(key);
    while (at != std::string_view::npos && at > 0 && !is_space(declaration[at - 1]))
        at = declaration.find(key, at + 1);
    if (at == std::string_view::npos)
        return {};
    at = declaration.find_first_of("\"'", at + key.size());
    if (at == std::string_view::npos)
        return {};
    const std::size_t end = declaration.find(declaration[at], at + 1);
    return end == std::string_view::npos ? std::string_view() : declaration.substr(at + 1, end - at - 1);
}

class Parser
{
public:
    explicit Parser(std::string_view document)
    : m_doc(document)
    {
    }

    Expected<XmlElement> run();

private:
    std::unexpected<Error> error(std::string_view what) const
    {
        const auto line = std::count(m_doc.begin(), m_doc.begin() + static_cast<std::ptrdiff_t>(std::min(m_pos, m_doc.size())), '\n') + 1;
        return fail(ErrorCode::Format, "line {}: {}", line, what);
    }

    bool at_end() const
    {
        return m_pos >= m_doc.size();
    }

    bool starts(std::string_view prefix) const
    {
        return m_doc.substr(std::min(m_pos, m_doc.size())).starts_with(prefix);
    }

    bool skip_space()
    {
        const std::size_t start = m_pos;
        while (!at_end() && is_space(m_doc[m_pos]))
            ++m_pos;
        return m_pos > start;
    }

    /** Moves past the next @p end; false if the document ends first. */
    bool skip_past(std::string_view end)
    {
        const std::size_t at = m_doc.find(end, m_pos);
        if (at == std::string_view::npos)
            return false;
        m_pos = at + end.size();
        return true;
    }

    std::string_view name()
    {
        const std::size_t start = m_pos;
        if (at_end() || !is_name_start(m_doc[m_pos]))
            return {};
        while (!at_end() && is_name_char(m_doc[m_pos]))
            ++m_pos;
        return m_doc.substr(start, m_pos - start);
    }

    std::optional<std::unexpected<Error>> reference(std::string& out);
    std::optional<std::unexpected<Error>> misc(bool prolog);
    std::optional<std::unexpected<Error>> start_tag(std::vector<XmlElement>& open, bool& empty);

    std::string_view m_doc;
    std::size_t m_pos = 0;
};

// At '&': appends the character the reference stands for
std::optional<std::unexpected<Error>> Parser::reference(std::string& out)
{
    const std::size_t end = m_doc.find(';', m_pos);
    if (end == std::string_view::npos || end - m_pos > 16)
        return error("unterminated entity reference");
    const std::string_view ref = m_doc.substr(m_pos + 1, end - m_pos - 1);
    if (ref.starts_with('#')) {
        const bool hex = ref.size() > 1 && ref[1] == 'x';
        const std::string_view digits = ref.substr(hex ? 2 : 1);
        std::uint32_t code = 0;
        if (digits.empty())
            return error("invalid character reference");
        for (const char c : digits) {
            int digit = -1;
            if (c >= '0' && c <= '9')
                digit = c - '0';
            else if (hex && c >= 'a' && c <= 'f')
                digit = c - 'a' + 10;
            else if (hex && c >= 'A' && c <= 'F')
                digit = c - 'A' + 10;
            if (digit < 0 || code > 0x10FFFF)
                return error("invalid character reference");
            code = code * (hex ? 16U : 10U) + static_cast<std::uint32_t>(digit);
        }
        const bool allowed = code == 0x9 || code == 0xA || code == 0xD || (code >= 0x20 && code <= 0xD7FF) || (code >= 0xE000 && code <= 0xFFFD) || (code >= 0x10000 && code <= 0x10FFFF);
        if (!allowed)
            return error("invalid character reference");
        append_utf8(out, code);
    } else if (ref == "lt") {
        out += '<';
    } else if (ref == "gt") {
        out += '>';
    } else if (ref == "amp") {
        out += '&';
    } else if (ref == "apos") {
        out += '\'';
    } else if (ref == "quot") {
        out += '"';
    } else {
        return error("undeclared entity");
    }
    m_pos = end + 1;
    return std::nullopt;
}

// Skips comments, processing instructions, white space and, before the root, the document type declaration
std::optional<std::unexpected<Error>> Parser::misc(bool prolog)
{
    while (true) {
        skip_space();
        if (starts("<!--")) {
            if (!skip_past("-->"))
                return error("unterminated comment");
        } else if (starts("<?")) {
            if (!skip_past("?>"))
                return error("unterminated processing instruction");
        } else if (prolog && starts("<!DOCTYPE")) {
            int depth = 0;
            char quote = 0;
            for (; !at_end(); ++m_pos) {
                const char c = m_doc[m_pos];
                if (quote) {
                    quote = c == quote ? 0 : quote;
                } else if (c == '"' || c == '\'') {
                    quote = c;
                } else if (c == '[') {
                    ++depth;
                } else if (c == ']') {
                    --depth;
                } else if (c == '>' && depth == 0) {
                    break;
                }
            }
            if (at_end())
                return error("unterminated document type declaration");
            ++m_pos;
        } else {
            return std::nullopt;
        }
    }
}

// At '<' of a start tag: pushes the element, with its attributes, onto the open elements
std::optional<std::unexpected<Error>> Parser::start_tag(std::vector<XmlElement>& open, bool& empty)
{
    ++m_pos;
    XmlElement element;
    element.name = name();
    if (element.name.empty())
        return error("expected an element name");
    while (true) {
        const bool spaced = skip_space();
        if (at_end())
            return error("unexpected end of document");
        if (starts("/>")) {
            m_pos += 2;
            empty = true;
            break;
        }
        if (m_doc[m_pos] == '>') {
            ++m_pos;
            empty = false;
            break;
        }
        const std::string_view key = name();
        if (key.empty() || !spaced)
            return error("expected an attribute name");
        if (element.attribute(key))
            return error("attribute redefined");
        skip_space();
        if (at_end() || m_doc[m_pos] != '=')
            return error("expected '='");
        ++m_pos;
        skip_space();
        if (at_end() || (m_doc[m_pos] != '"' && m_doc[m_pos] != '\''))
            return error("expected a quoted attribute value");
        const char quote = m_doc[m_pos++];
        std::string value;
        while (!at_end() && m_doc[m_pos] != quote) {
            const char c = m_doc[m_pos];
            if (c == '<')
                return error("'<' in an attribute value");
            if (c == '&') {
                if (auto failed = reference(value))
                    return failed;
                continue;
            }
            value += is_space(c) ? ' ' : c;
            ++m_pos;
        }
        if (at_end())
            return error("unexpected end of document");
        ++m_pos;
        element.attributes.emplace_back(std::string(key), std::move(value));
    }
    open.push_back(std::move(element));
    return std::nullopt;
}

Expected<XmlElement> Parser::run()
{
    if (auto failed = misc(true))
        return *failed;
    if (at_end() || m_doc[m_pos] != '<')
        return error("expected the root element");

    std::vector<XmlElement> open;
    std::optional<XmlElement> root;
    const auto close = [&open, &root]() {
        XmlElement done = std::move(open.back());
        open.pop_back();
        if (open.empty())
            root = std::move(done);
        else
            open.back().children.push_back(std::move(done));
    };

    bool empty = false;
    if (auto failed = start_tag(open, empty))
        return *failed;
    if (empty)
        close();
    while (!open.empty()) {
        if (at_end())
            return error("unexpected end of document");
        const char c = m_doc[m_pos];
        if (c == '&') {
            if (auto failed = reference(open.back().text))
                return *failed;
        } else if (c != '<') {
            const std::size_t end = std::min(m_doc.find_first_of("<&", m_pos), m_doc.size());
            const std::string_view text = m_doc.substr(m_pos, end - m_pos);
            if (text.find("]]>") != std::string_view::npos)
                return error("']]>' in content");
            open.back().text += text;
            m_pos = end;
        } else if (starts("<![CDATA[")) {
            const std::size_t start = m_pos + 9;
            if (!skip_past("]]>"))
                return error("unterminated CDATA section");
            open.back().text += m_doc.substr(start, m_pos - 3 - start);
        } else if (starts("<!--")) {
            if (!skip_past("-->"))
                return error("unterminated comment");
        } else if (starts("<?")) {
            if (!skip_past("?>"))
                return error("unterminated processing instruction");
        } else if (starts("</")) {
            m_pos += 2;
            if (name() != open.back().name)
                return error("opening and ending tag mismatch");
            skip_space();
            if (at_end() || m_doc[m_pos] != '>')
                return error("expected '>'");
            ++m_pos;
            close();
        } else {
            if (auto failed = start_tag(open, empty))
                return *failed;
            if (empty)
                close();
        }
    }
    if (auto failed = misc(false))
        return *failed;
    if (!at_end())
        return error("extra content at the end of the document");
    return std::move(*root);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::string_view XmlElement::local_name() const
{
    const std::string_view qualified = name;
    const std::size_t colon = qualified.find(':');
    return colon == std::string_view::npos ? qualified : qualified.substr(colon + 1);
}

//=============================================================================================================

const std::string* XmlElement::attribute(std::string_view key) const
{
    const auto it = std::ranges::find(attributes, key, [](const auto& attr) { return std::string_view(attr.first); });
    return it == attributes.end() ? nullptr : &it->second;
}

//=============================================================================================================

const XmlElement* XmlElement::child(std::string_view local) const
{
    const auto it = std::ranges::find(children, local, &XmlElement::local_name);
    return it == children.end() ? nullptr : &*it;
}

//=============================================================================================================

Expected<XmlElement> mne::parse_xml(std::string_view document)
{
    if (document.starts_with("\xEF\xBB\xBF"))
        document.remove_prefix(3);

    std::string text;
    if (document.starts_with("<?xml")) {
        const std::string_view declaration = document.substr(0, document.find("?>"));
        const std::string_view encoding = declared(declaration, "encoding");
        if (iequals(encoding, "ISO-8859-1") || iequals(encoding, "latin1") || iequals(encoding, "latin-1")) {
            text = latin1_to_utf8(document);
        } else if (!encoding.empty() && !iequals(encoding, "UTF-8") && !iequals(encoding, "US-ASCII") && !iequals(encoding, "ASCII")) {
            return fail(ErrorCode::Format, "line 1: unsupported encoding {}", encoding);
        }
    }
    if (text.empty())
        text = document;
    if (!is_utf8(text))
        return fail(ErrorCode::Format, "the document is not UTF-8");

    // Line ends are read as a single line feed
    std::string normalized;
    normalized.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            normalized += '\n';
            if (i + 1 < text.size() && text[i + 1] == '\n')
                ++i;
        } else {
            normalized += text[i];
        }
    }
    return Parser(normalized).run();
}

//=============================================================================================================

Expected<XmlElement> mne::read_xml_file(const std::filesystem::path& path)
{
    Expected<std::string> text = read_text_file(path);
    if (!text)
        return std::unexpected(text.error());
    Expected<XmlElement> root = parse_xml(*text);
    if (!root)
        return fail(ErrorCode::Format, "{}: {}", utf8_from_path(path), root.error().what());
    return root;
}
