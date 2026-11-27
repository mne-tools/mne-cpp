//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     settings_store.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Typed accessors of SettingsStore and the in-memory MemorySettingsStore.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settings_store.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <charconv>
#include <cmath>
#include <format>
#include <limits>
#include <system_error>
#include <type_traits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

std::optional<double> parseNumber(const std::string& text)
{
    double number = 0.0;
    const char* end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(text.data(), end, number);
    if (ec != std::errc() || ptr != end) {
        return std::nullopt;
    }
    return number;
}

std::optional<double> toNumber(const SettingValue& value)
{
    if (const bool* b = std::get_if<bool>(&value)) {
        return *b ? 1.0 : 0.0;
    }
    if (const int* i = std::get_if<int>(&value)) {
        return static_cast<double>(*i);
    }
    if (const double* d = std::get_if<double>(&value)) {
        return *d;
    }
    return parseNumber(std::get<std::string>(value));
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool SettingsStore::contains(const std::string& key) const
{
    return value(key).has_value();
}

//=============================================================================================================

bool SettingsStore::read_bool(const std::string& key, bool fallback) const
{
    const std::optional<SettingValue> stored = value(key);
    if (!stored) {
        return fallback;
    }
    if (const std::string* text = std::get_if<std::string>(&*stored)) {
        if (*text == "true") {
            return true;
        }
        if (*text == "false") {
            return false;
        }
    }
    const std::optional<double> number = toNumber(*stored);
    return number ? *number != 0.0 : fallback;
}

//=============================================================================================================

int SettingsStore::read_int(const std::string& key, int fallback) const
{
    const std::optional<SettingValue> stored = value(key);
    const std::optional<double> number = stored ? toNumber(*stored) : std::nullopt;
    if (!number || !std::isfinite(*number) || *number < std::numeric_limits<int>::min() || *number > std::numeric_limits<int>::max()) {
        return fallback;
    }
    return static_cast<int>(std::lround(*number));
}

//=============================================================================================================

double SettingsStore::read_double(const std::string& key, double fallback) const
{
    const std::optional<SettingValue> stored = value(key);
    const std::optional<double> number = stored ? toNumber(*stored) : std::nullopt;
    return number.value_or(fallback);
}

//=============================================================================================================

std::string SettingsStore::read_string(const std::string& key, const std::string& fallback) const
{
    const std::optional<SettingValue> stored = value(key);
    if (!stored) {
        return fallback;
    }
    return std::visit(
        [](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else {
                return std::format("{}", v);
            }
        },
        *stored);
}

//=============================================================================================================

std::optional<SettingValue> MemorySettingsStore::value(const std::string& key) const
{
    const auto it = m_values.find(key);
    if (it == m_values.end()) {
        return std::nullopt;
    }
    return it->second;
}

//=============================================================================================================

void MemorySettingsStore::set_value(const std::string& key, const SettingValue& value)
{
    m_values[key] = value;
}
