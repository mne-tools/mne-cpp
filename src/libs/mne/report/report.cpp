//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     report.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of @ref mne::Report.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "report.h"

#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstdint>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

constexpr std::string_view kStyle =
    "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; "
    "max-width: 960px; margin: 0 auto; padding: 20px; color: #333; }\n"
    "h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }\n"
    "h2 { color: #2980b9; margin-top: 30px; }\n"
    "table { border-collapse: collapse; width: 100%; margin: 10px 0; }\n"
    "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n"
    "th { background-color: #3498db; color: white; }\n"
    "tr:nth-child(even) { background-color: #f2f2f2; }\n"
    "dl { margin: 10px 0; }\n"
    "dt { font-weight: bold; margin-top: 8px; }\n"
    "dd { margin-left: 20px; color: #555; }\n"
    "pre { background-color: #f8f8f8; border: 1px solid #ddd; padding: 12px; border-radius: 4px; overflow-x: auto; }\n"
    "code { font-family: 'SF Mono', Menlo, Monaco, monospace; font-size: 0.9em; }\n"
    "figure { margin: 10px 0; }\n"
    "figure img { max-width: 100%; height: auto; border: 1px solid #ddd; }\n"
    "figcaption { color: #555; font-size: 0.9em; margin-top: 4px; }\n"
    ".toc { background-color: #f8f8f8; border: 1px solid #ddd; padding: 8px 16px; }\n"
    ".toc ul { margin: 0; padding-left: 20px; }\n"
    ".footer { margin-top: 40px; padding-top: 10px; border-top: 1px solid #ddd; color: #999; font-size: 0.85em; }\n";

std::string_view mimeType(ImageFormat format)
{
    switch (format) {
        case ImageFormat::Png:
            return "image/png";
        case ImageFormat::Jpeg:
            return "image/jpeg";
        case ImageFormat::Svg:
            return "image/svg+xml";
    }
    return "application/octet-stream";
}

std::optional<ImageFormat> formatOfExtension(const std::filesystem::path& path)
{
    const std::string extension = to_lower(utf8_from_path(path.extension()));
    if (extension == ".png") {
        return ImageFormat::Png;
    }
    if (extension == ".jpg" || extension == ".jpeg") {
        return ImageFormat::Jpeg;
    }
    if (extension == ".svg") {
        return ImageFormat::Svg;
    }
    return std::nullopt;
}

/** Appends an element whose text content is escaped, e.g. <td>text</td>. */
void appendElement(std::string& html, std::string_view tag, std::string_view text, std::string_view indent = "")
{
    std::format_to(std::back_inserter(html), "{}<{}>{}</{}>\n", indent, tag, html_escape(text), tag);
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::string mne::html_escape(std::string_view text)
{
    std::string escaped;
    escaped.reserve(text.size());
    for (const char c : text) {
        switch (c) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#39;";
                break;
            default:
                escaped += c;
        }
    }
    return escaped;
}

//=============================================================================================================

std::string mne::base64_encode(std::string_view data)
{
    constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    encoded.reserve((data.size() + 2) / 3 * 4);
    std::size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        const auto chunk = static_cast<std::uint32_t>(static_cast<unsigned char>(data[i]) << 16U |
                                                      static_cast<unsigned char>(data[i + 1]) << 8U |
                                                      static_cast<unsigned char>(data[i + 2]));
        encoded += alphabet[chunk >> 18U & 0x3FU];
        encoded += alphabet[chunk >> 12U & 0x3FU];
        encoded += alphabet[chunk >> 6U & 0x3FU];
        encoded += alphabet[chunk & 0x3FU];
    }
    if (const std::size_t rest = data.size() - i; rest > 0) {
        std::uint32_t chunk = static_cast<std::uint32_t>(static_cast<unsigned char>(data[i])) << 16U;
        if (rest == 2) {
            chunk |= static_cast<std::uint32_t>(static_cast<unsigned char>(data[i + 1])) << 8U;
        }
        encoded += alphabet[chunk >> 18U & 0x3FU];
        encoded += alphabet[chunk >> 12U & 0x3FU];
        encoded += rest == 2 ? alphabet[chunk >> 6U & 0x3FU] : '=';
        encoded += '=';
    }
    return encoded;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Report::Report(std::string title)
: m_title(std::move(title))
{
}

//=============================================================================================================

void Report::add_section(std::string_view title, std::string html)
{
    m_sections.push_back({std::string(title), std::move(html)});
}

//=============================================================================================================

void Report::add_text(std::string_view title, std::string_view text)
{
    add_section(title, "<p>" + html_escape(text) + "</p>");
}

//=============================================================================================================

void Report::add_table(std::string_view title, const StringList& headers, const std::vector<StringList>& rows)
{
    std::string html = "<table>\n<thead><tr>\n";
    for (const std::string& header : headers) {
        appendElement(html, "th", header, "  ");
    }
    html += "</tr></thead>\n<tbody>\n";
    for (const StringList& row : rows) {
        html += "<tr>\n";
        for (const std::string& cell : row) {
            appendElement(html, "td", cell, "  ");
        }
        html += "</tr>\n";
    }
    html += "</tbody>\n</table>";
    add_section(title, std::move(html));
}

//=============================================================================================================

void Report::add_key_value(std::string_view title, const std::vector<std::pair<std::string, std::string>>& pairs)
{
    std::string html = "<dl>\n";
    for (const auto& [key, value] : pairs) {
        appendElement(html, "dt", key, "  ");
        appendElement(html, "dd", value, "  ");
    }
    html += "</dl>";
    add_section(title, std::move(html));
}

//=============================================================================================================

void Report::add_code(std::string_view title, std::string_view code)
{
    add_section(title, "<pre><code>" + html_escape(code) + "</code></pre>");
}

//=============================================================================================================

void Report::add_image(std::string_view title, std::string_view data, ImageFormat format, std::string_view caption)
{
    std::string html = std::format("<figure>\n<img src=\"data:{};base64,{}\" alt=\"{}\">\n", mimeType(format),
                                   base64_encode(data), html_escape(title));
    if (!caption.empty()) {
        appendElement(html, "figcaption", caption);
    }
    html += "</figure>";
    add_section(title, std::move(html));
}

//=============================================================================================================

bool Report::add_image_file(std::string_view title, const std::filesystem::path& path, std::string_view caption)
{
    const std::optional<ImageFormat> format = formatOfExtension(path);
    if (!format) {
        Log::warning("[Report::add_image_file] Unsupported image format: {}", utf8_from_path(path));
        return false;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        Log::warning("[Report::add_image_file] Cannot open: {}", utf8_from_path(path));
        return false;
    }
    const std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    add_image(title, data, *format, caption);
    return true;
}

//=============================================================================================================

std::size_t Report::section_count() const
{
    return m_sections.size();
}

//=============================================================================================================

const std::string& Report::title() const
{
    return m_title;
}

//=============================================================================================================

std::string Report::to_html() const
{
    std::string html = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
                       "<meta charset=\"UTF-8\">\n"
                       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    appendElement(html, "title", m_title);
    html += "<style>\n";
    html += kStyle;
    html += "</style>\n</head>\n<body>\n";
    appendElement(html, "h1", m_title);
    if (m_sections.size() > 1) {
        html += "<nav class=\"toc\">\n<ul>\n";
        for (std::size_t i = 0; i < m_sections.size(); ++i) {
            std::format_to(std::back_inserter(html), "<li><a href=\"#section-{}\">{}</a></li>\n", i + 1,
                           html_escape(m_sections[i].title));
        }
        html += "</ul>\n</nav>\n";
    }
    for (std::size_t i = 0; i < m_sections.size(); ++i) {
        std::format_to(std::back_inserter(html), "<h2 id=\"section-{}\">{}</h2>\n", i + 1, html_escape(m_sections[i].title));
        html += m_sections[i].html;
        html += '\n';
    }
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    std::format_to(std::back_inserter(html), "<div class=\"footer\">\n<p>Generated by MNE-CPP on {:%FT%TZ}</p>\n</div>\n", now);
    html += "</body>\n</html>\n";
    return html;
}

//=============================================================================================================

bool Report::save(const std::filesystem::path& path) const
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        Log::warning("[Report::save] Cannot open: {}", utf8_from_path(path));
        return false;
    }
    file << to_html();
    return static_cast<bool>(file);
}
