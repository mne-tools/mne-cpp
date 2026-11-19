//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_uri.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqUri parsing and formatting, and typed parameter access for backends.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_device_registry.h"
#include "detail/acq_uri_params.h"

#include <array>
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{

constexpr std::string_view kSeparator = "://";

bool isAlpha(char c) noexcept
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isDigit(char c) noexcept
{
    return c >= '0' && c <= '9';
}

int hexValue(char c) noexcept
{
    if (isDigit(c)) {
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

AcqError invalid(std::string_view uri, std::size_t position, std::string_view what)
{
    return {AcqErrorCode::InvalidUri, std::string(what) + " at position " + std::to_string(position) + " of '" + std::string(uri) + "'"};
}

/** Decodes uri[begin, end); positions in errors refer to the whole uri. */
AcqExpected<std::string> percentDecode(std::string_view uri, std::size_t begin, std::size_t end)
{
    std::string out;
    out.reserve(end - begin);
    for (std::size_t i = begin; i < end; ++i) {
        if (uri[i] != '%') {
            out.push_back(uri[i]);
            continue;
        }
        const int hi = i + 2 < end ? hexValue(uri[i + 1]) : -1;
        const int lo = i + 2 < end ? hexValue(uri[i + 2]) : -1;
        if (hi < 0 || lo < 0) {
            return invalid(uri, i, "invalid percent escape");
        }
        out.push_back(static_cast<char>(hi * 16 + lo));
        i += 2;
    }
    return out;
}

void percentEncode(std::string& out, std::string_view text, std::string_view keep)
{
    static constexpr std::array<char, 16> kHex = {'0', '1', '2', '3', '4', '5', '6', '7',
                                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    for (const char c : text) {
        if (isAlpha(c) || isDigit(c) || c == '-' || c == '.' || c == '_' || c == '~' ||
            keep.find(c) != std::string_view::npos) {
            out.push_back(c);
        } else {
            const auto byte = static_cast<unsigned char>(c);
            out.push_back('%');
            out.push_back(kHex[byte >> 4U]);
            out.push_back(kHex[byte & 0xFU]);
        }
    }
}

AcqError badValue(const AcqUri& uri, std::string_view key, std::string_view value, std::string_view expected)
{
    return {AcqErrorCode::InvalidParameter, std::string(key) + "=" + std::string(value) + " in " + uri.scheme + "://: expected " + std::string(expected)};
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AcqExpected<AcqUri> AcqUri::parse(std::string_view uri)
{
    const std::size_t separator = uri.find(kSeparator);
    if (separator == std::string_view::npos) {
        return invalid(uri, uri.size(), "missing '://'");
    }
    if (separator == 0) {
        return invalid(uri, 0, "empty scheme");
    }
    AcqUri out;
    for (std::size_t i = 0; i < separator; ++i) {
        const char c = uri[i];
        // RFC 3986 characters, and '_' as in BrainFlow's streaming_board://
        const bool ok = isAlpha(c) || (i > 0 && (isDigit(c) || c == '+' || c == '-' || c == '.' || c == '_'));
        if (!ok) {
            return invalid(uri, i, std::string("invalid scheme character '") + c + "'");
        }
        out.scheme.push_back(c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c);
    }

    const std::size_t targetBegin = separator + kSeparator.size();
    const std::size_t query = uri.find('?', targetBegin);
    const std::size_t targetEnd = query == std::string_view::npos ? uri.size() : query;
    auto target = percentDecode(uri, targetBegin, targetEnd);
    if (!target) {
        return target.error();
    }
    out.target = std::move(*target);
    if (query == std::string_view::npos) {
        return out;
    }

    std::size_t begin = query + 1;
    while (true) {
        const std::size_t amp = uri.find('&', begin);
        const std::size_t end = amp == std::string_view::npos ? uri.size() : amp;
        const std::size_t eq = uri.substr(begin, end - begin).find('=');
        const std::size_t keyEnd = eq == std::string_view::npos ? end : begin + eq;
        if (keyEnd == begin) {
            return invalid(uri, begin, "empty parameter name");
        }
        auto key = percentDecode(uri, begin, keyEnd);
        if (!key) {
            return key.error();
        }
        auto value = percentDecode(uri, keyEnd == end ? end : keyEnd + 1, end);
        if (!value) {
            return value.error();
        }
        if (out.param(*key)) {
            return invalid(uri, begin, "duplicate parameter '" + *key + "'");
        }
        out.params.emplace_back(std::move(*key), std::move(*value));
        if (amp == std::string_view::npos) {
            break;
        }
        begin = amp + 1;
    }
    return out;
}

//=============================================================================================================

std::optional<std::string_view> AcqUri::param(std::string_view key) const noexcept
{
    for (const auto& [name, value] : params) {
        if (name == key) {
            return std::string_view(value);
        }
    }
    return std::nullopt;
}

//=============================================================================================================

std::string AcqUri::toString() const
{
    std::string out = scheme + std::string(kSeparator);
    percentEncode(out, target, "/:@");
    for (std::size_t i = 0; i < params.size(); ++i) {
        out.push_back(i == 0 ? '?' : '&');
        percentEncode(out, params[i].first, "");
        out.push_back('=');
        percentEncode(out, params[i].second, "");
    }
    return out;
}

//=============================================================================================================
// DEFINE PARAMETER HELPERS
//=============================================================================================================

AcqExpected<std::int64_t> mne::detail::paramInt(const AcqUri& uri, std::string_view key, std::int64_t fallback,
                                                std::int64_t min, std::int64_t max)
{
    const auto text = uri.param(key);
    if (!text) {
        return fallback;
    }
    std::int64_t value = 0;
    const auto [end, ec] = std::from_chars(text->data(), text->data() + text->size(), value);
    if (ec != std::errc() || end != text->data() + text->size() || value < min || value > max) {
        return badValue(uri, key, *text, "an integer in [" + std::to_string(min) + ", " + std::to_string(max) + "]");
    }
    return value;
}

//=============================================================================================================

AcqExpected<double> mne::detail::paramDouble(const AcqUri& uri, std::string_view key, double fallback, double min,
                                             double max)
{
    const auto text = uri.param(key);
    if (!text) {
        return fallback;
    }
    // std::from_chars for double is unavailable on older macOS runtimes; the classic locale keeps '.'.
    std::istringstream stream{std::string(*text)};
    stream.imbue(std::locale::classic());
    double value = 0.0;
    stream >> value;
    const bool consumed = !stream.fail() && stream.peek() == std::char_traits<char>::eof();
    if (!consumed || !std::isfinite(value) || value < min || value > max) {
        std::ostringstream range;
        range.imbue(std::locale::classic());
        range << "a number in [" << min << ", " << max << "]";
        return badValue(uri, key, *text, range.str());
    }
    return value;
}

//=============================================================================================================

AcqExpected<bool> mne::detail::paramBool(const AcqUri& uri, std::string_view key, bool fallback)
{
    const auto text = uri.param(key);
    if (!text) {
        return fallback;
    }
    if (*text == "1" || *text == "true" || *text == "yes" || *text == "on") {
        return true;
    }
    if (*text == "0" || *text == "false" || *text == "no" || *text == "off") {
        return false;
    }
    return badValue(uri, key, *text, "1/0, true/false, yes/no or on/off");
}

//=============================================================================================================

AcqExpected<AcqThreadPolicy> mne::detail::paramThreadPolicy(const AcqUri& uri)
{
    AcqThreadPolicy policy;
    if (const auto cpus = uri.param("cpus")) {
        const char* p = cpus->data();
        const char* const end = p + cpus->size();
        while (true) {
            unsigned cpu = 0;
            const auto [next, ec] = std::from_chars(p, end, cpu);
            if (ec != std::errc() || (next != end && *next != ',')) {
                return badValue(uri, "cpus", *cpus, "comma-separated CPU numbers, e.g. 2,3");
            }
            policy.cpus.push_back(cpu);
            if (next == end) {
                break;
            }
            p = next + 1;
        }
    }
    if (const auto priority = uri.param("priority")) {
        if (*priority == "high") {
            policy.priority = AcqThreadPriority::High;
        } else if (*priority == "realtime") {
            policy.priority = AcqThreadPriority::Realtime;
        } else if (*priority != "normal") {
            return badValue(uri, "priority", *priority, "normal, high or realtime");
        }
    }
    return policy;
}
