//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     string_utils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Definition of the text and path helpers in base/string_utils.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "string_utils.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <charconv>
#include <cmath>
#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <limits>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <xlocale.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

bool isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

char lower(char c)
{
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

double strtodC(const char* text)
{
#ifdef _WIN32
    static _locale_t c = _create_locale(LC_NUMERIC, "C");
    return _strtod_l(text, nullptr, c);
#else
    static locale_t c = newlocale(LC_NUMERIC_MASK, "C", nullptr);
    return strtod_l(text, nullptr, c);
#endif
}

/** [+-]? (digits [. digits?] | . digits) ([eE] [+-]? digits)?, so every standard library parses the same set. */
bool isDecimal(std::string_view text)
{
    std::size_t i = 0;
    const auto digits = [&] {
        const std::size_t start = i;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
            ++i;
        }
        return i - start;
    };
    if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
        ++i;
    }
    std::size_t mantissa = digits();
    if (i < text.size() && text[i] == '.') {
        ++i;
        mantissa += digits();
    }
    if (mantissa == 0) {
        return false;
    }
    if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
        ++i;
        if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
            ++i;
        }
        if (digits() == 0) {
            return false;
        }
    }
    return i == text.size();
}

/** from_chars takes neither a leading '+' nor inf/nan spelled the way QString accepts them. */
std::string_view stripPlus(std::string_view text)
{
    return text.size() > 1 && text.front() == '+' && text[1] != '-' && text[1] != '+' ? text.substr(1) : text;
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::string_view mne::trimmed(std::string_view text)
{
    while (!text.empty() && isSpace(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && isSpace(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

//=============================================================================================================

mne::StringList mne::split(std::string_view text, char separator, bool skipEmpty)
{
    StringList fields;
    for (std::size_t start = 0;;) {
        const std::size_t end = text.find(separator, start);
        const std::string_view field = text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
        if (!skipEmpty || !field.empty()) {
            fields.emplace_back(field);
        }
        if (end == std::string_view::npos) {
            return fields;
        }
        start = end + 1;
    }
}

//=============================================================================================================

std::vector<std::string_view> mne::split_whitespace(std::string_view text)
{
    std::vector<std::string_view> fields;
    std::size_t pos = 0;
    while (true) {
        while (pos < text.size() && isSpace(text[pos])) {
            ++pos;
        }
        if (pos == text.size()) {
            return fields;
        }
        const std::size_t start = pos;
        while (pos < text.size() && !isSpace(text[pos])) {
            ++pos;
        }
        fields.push_back(text.substr(start, pos - start));
    }
}

//=============================================================================================================

int mne::index_of(const StringList& list, std::string_view name)
{
    const auto it = std::find(list.begin(), list.end(), name);
    return it == list.end() ? -1 : static_cast<int>(it - list.begin());
}

//=============================================================================================================

std::string mne::join(const StringList& list, std::string_view separator)
{
    std::string out;
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (i > 0)
            out += separator;
        out += list[i];
    }
    return out;
}

//=============================================================================================================

std::string mne::to_lower(std::string_view text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), lower);
    return out;
}

//=============================================================================================================

bool mne::iequals(std::string_view a, std::string_view b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) { return lower(x) == lower(y); });
}

//=============================================================================================================

std::optional<double> mne::to_double(std::string_view text)
{
    text = stripPlus(trimmed(text));
    const std::string lowered = to_lower(text);
    const bool negative = !lowered.empty() && lowered.front() == '-';
    const std::string_view word = negative ? std::string_view(lowered).substr(1) : std::string_view(lowered);
    if (word == "inf") {
        return negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    }
    if (word == "nan" && !negative) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (!isDecimal(text)) {
        return std::nullopt;
    }
    // Floating-point std::from_chars needs macOS 26 with libc++, and streams disagree on denormals across
    // standard libraries, so the C library parses in the "C" locale whatever the application set.
    const std::string copy(text);
    const double value = strtodC(copy.c_str());
    const bool nonZeroDigit = std::any_of(text.begin(), std::find_if(text.begin(), text.end(), [](char c) { return c == 'e' || c == 'E'; }),
                                          [](char c) { return c >= '1' && c <= '9'; });
    if (std::isinf(value) || (value == 0.0 && nonZeroDigit)) {
        return std::nullopt; // overflow or underflow to zero; QString rejects both and keeps denormals
    }
    return value;
}

//=============================================================================================================

std::optional<int> mne::to_int(std::string_view text)
{
    text = stripPlus(trimmed(text));
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || error != std::errc() || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

//=============================================================================================================

std::filesystem::path mne::path_from_utf8(std::string_view utf8)
{
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
}

//=============================================================================================================

std::string mne::utf8_from_path(const std::filesystem::path& path)
{
    const std::u8string utf8 = path.u8string();
    return std::string(utf8.begin(), utf8.end());
}

//=============================================================================================================

std::string mne::latin1_to_utf8(std::string_view latin1)
{
    std::string utf8;
    utf8.reserve(latin1.size() + static_cast<std::size_t>(std::ranges::count_if(latin1, [](char c) { return static_cast<unsigned char>(c) >= 0x80; })));
    for (const char c : latin1) {
        const auto byte = static_cast<unsigned char>(c);
        if (byte < 0x80) {
            utf8.push_back(c);
        } else {
            utf8.push_back(static_cast<char>(0xC0 | (byte >> 6)));
            utf8.push_back(static_cast<char>(0x80 | (byte & 0x3F)));
        }
    }
    return utf8;
}

//=============================================================================================================

bool mne::is_utf8(std::string_view text)
{
    const auto byte = [&](std::size_t i) {
        return static_cast<unsigned char>(text[i]);
    };
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char lead = byte(i);
        if (lead < 0x80) {
            ++i;
            continue;
        }
        // Second-byte range per lead byte (Unicode table 3-7), which rules out overlongs, surrogates and > U+10FFFF
        std::size_t length = 0;
        unsigned char low = 0x80;
        unsigned char high = 0xBF;
        if (lead >= 0xC2 && lead <= 0xDF) {
            length = 2;
        } else if (lead >= 0xE0 && lead <= 0xEF) {
            length = 3;
            low = lead == 0xE0 ? 0xA0 : 0x80;
            high = lead == 0xED ? 0x9F : 0xBF;
        } else if (lead >= 0xF0 && lead <= 0xF4) {
            length = 4;
            low = lead == 0xF0 ? 0x90 : 0x80;
            high = lead == 0xF4 ? 0x8F : 0xBF;
        } else {
            return false;
        }
        if (i + length > text.size() || byte(i + 1) < low || byte(i + 1) > high) {
            return false;
        }
        for (std::size_t k = 2; k < length; ++k) {
            if ((byte(i + k) & 0xC0) != 0x80) {
                return false;
            }
        }
        i += length;
    }
    return true;
}

//=============================================================================================================

std::string mne::to_hex(std::span<const std::byte> data)
{
    constexpr std::string_view digits = "0123456789abcdef";
    std::string hex;
    hex.reserve(2 * data.size());
    for (const std::byte b : data) {
        hex.push_back(digits[std::to_integer<std::size_t>(b >> 4)]);
        hex.push_back(digits[std::to_integer<std::size_t>(b & std::byte{0x0F})]);
    }
    return hex;
}

//=============================================================================================================

std::filesystem::path mne::executable_dir()
{
    static const std::filesystem::path dir = [] {
        std::error_code error;
#if defined(__EMSCRIPTEN__)
        return std::filesystem::path("/");
#elif defined(__APPLE__)
        std::uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::string buffer(size, '\0');
        if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
            return std::filesystem::path();
        }
        const std::filesystem::path exe = std::filesystem::canonical(buffer.c_str(), error);
#elif defined(_WIN32)
        std::wstring buffer(MAX_PATH, L'\0');
        DWORD length = 0;
        while ((length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()))) == buffer.size()) {
            buffer.resize(buffer.size() * 2);
        }
        buffer.resize(length);
        const std::filesystem::path exe = std::filesystem::canonical(buffer, error);
#else
        const std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", error);
#endif
        return error ? std::filesystem::path() : exe.parent_path();
    }();
    return dir;
}

//=============================================================================================================

std::filesystem::path mne::resource_path(std::string_view relative)
{
    const std::filesystem::path name = path_from_utf8(relative);
    std::filesystem::path installed = executable_dir().parent_path() / "resources" / name;
    std::error_code error;
    if (std::filesystem::exists(installed, error)) {
        return installed;
    }
    return std::filesystem::path("..") / "resources" / name;
}

//=============================================================================================================

std::string mne::to_base64(std::span<const std::byte> data, bool url)
{
    const std::string_view alphabet = url ? "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
                                          : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const auto at = [&](unsigned bits) {
        return alphabet[bits & 63U];
    };
    std::string text;
    text.reserve((data.size() + 2) / 3 * 4);
    std::size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        const unsigned bits = (std::to_integer<unsigned>(data[i]) << 16) | (std::to_integer<unsigned>(data[i + 1]) << 8) | std::to_integer<unsigned>(data[i + 2]);
        text += {at(bits >> 18), at(bits >> 12), at(bits >> 6), at(bits)};
    }
    if (i < data.size()) {
        const bool two = i + 1 < data.size();
        const unsigned bits = (std::to_integer<unsigned>(data[i]) << 16) | (two ? std::to_integer<unsigned>(data[i + 1]) << 8 : 0U);
        text += {at(bits >> 18), at(bits >> 12)};
        if (two) {
            text += at(bits >> 6);
        }
        if (!url) {
            text.append(two ? 1 : 2, '=');
        }
    }
    return text;
}

//=============================================================================================================

std::optional<std::vector<std::byte>> mne::from_base64(std::string_view text)
{
    const auto sextet = [](char c) -> int {
        if (c >= 'A' && c <= 'Z')
            return c - 'A';
        if (c >= 'a' && c <= 'z')
            return c - 'a' + 26;
        if (c >= '0' && c <= '9')
            return c - '0' + 52;
        if (c == '+' || c == '-')
            return 62;
        if (c == '/' || c == '_')
            return 63;
        return -1;
    };
    std::vector<std::byte> data;
    data.reserve(text.size() / 4 * 3);
    unsigned bits = 0;
    int count = 0;
    bool padding = false;
    for (const char c : text) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            continue;
        }
        if (c == '=') {
            padding = true;
            continue;
        }
        const int value = sextet(c);
        if (value < 0 || padding) {
            return std::nullopt;
        }
        bits = (bits << 6) | static_cast<unsigned>(value);
        if (++count == 4) {
            data.insert(data.end(), {std::byte(bits >> 16), std::byte(bits >> 8), std::byte(bits)});
            bits = 0;
            count = 0;
        }
    }
    if (count == 1) {
        return std::nullopt;
    }
    if (count == 2) {
        data.push_back(std::byte(bits >> 4));
    } else if (count == 3) {
        data.insert(data.end(), {std::byte(bits >> 10), std::byte(bits >> 2)});
    }
    return data;
}

//=============================================================================================================

std::string mne::format_iso8601(UtcTime time, bool milliseconds)
{
    using namespace std::chrono;
    const sys_seconds seconds = floor<std::chrono::seconds>(time);
    if (!milliseconds) {
        return std::format("{:%FT%T}Z", seconds);
    }
    return std::format("{:%FT%T}.{:03}Z", seconds, (time - seconds).count());
}

//=============================================================================================================

std::optional<mne::UtcTime> mne::parse_iso8601(std::string_view text)
{
    using namespace std::chrono;
    std::size_t pos = 0;
    const auto number = [&](std::size_t digits, int& value) {
        if (pos + digits > text.size()) {
            return false;
        }
        const char* first = text.data() + pos;
        const auto [end, ec] = std::from_chars(first, first + digits, value);
        if (ec != std::errc{} || end != first + digits || *first == '-' || *first == '+') {
            return false;
        }
        pos += digits;
        return true;
    };
    const auto expect = [&](char c) {
        if (pos >= text.size() || text[pos] != c) {
            return false;
        }
        ++pos;
        return true;
    };

    int y = 0;
    int mo = 0;
    int d = 0;
    if (!number(4, y) || !expect('-') || !number(2, mo) || !expect('-') || !number(2, d)) {
        return std::nullopt;
    }
    const year_month_day date{year{y}, month{static_cast<unsigned>(mo)}, day{static_cast<unsigned>(d)}};
    if (!date.ok()) {
        return std::nullopt;
    }
    UtcTime time = sys_days(date);
    if (pos == text.size()) {
        return time;
    }
    if (text[pos] != 'T' && text[pos] != ' ') {
        return std::nullopt;
    }
    ++pos;

    int h = 0;
    int mi = 0;
    int s = 0;
    if (!number(2, h) || !expect(':') || !number(2, mi) || h > 23 || mi > 59) {
        return std::nullopt;
    }
    if (expect(':')) {
        if (!number(2, s) || s > 59) {
            return std::nullopt;
        }
        if (expect('.')) {
            std::int64_t nanos = 0;
            std::int64_t scale = 100'000'000;
            const std::size_t first = pos;
            for (; pos < text.size() && text[pos] >= '0' && text[pos] <= '9'; ++pos, scale /= 10) {
                nanos += (text[pos] - '0') * scale;
            }
            if (pos == first) {
                return std::nullopt;
            }
            time += std::chrono::milliseconds((nanos + 500'000) / 1'000'000); // rounded, as Qt does
        }
    }
    time += hours(h) + minutes(mi) + seconds(s);

    if (pos == text.size() || (text[pos] == 'Z' && pos + 1 == text.size())) {
        return time;
    }
    if (text[pos] != '+' && text[pos] != '-') {
        return std::nullopt;
    }
    const bool east = text[pos++] == '+';
    int oh = 0;
    int om = 0;
    if (!number(2, oh) || oh > 23) {
        return std::nullopt;
    }
    expect(':');
    if (!number(2, om) || om > 59 || pos != text.size()) {
        return std::nullopt;
    }
    const minutes offset = hours(oh) + minutes(om);
    return east ? time - offset : time + offset;
}
