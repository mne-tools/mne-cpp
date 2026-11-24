//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     command.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Translation unit for @ref RTSERVER::Command: constructors, JSON/CLI serialisation, parameter access and dispatch glue.
 *
 * Defines the five constructor overloads (default, JSON-object,
 * keyword+description, fully-specified parameter list, copy), the
 * @c toJsonObject / @c toStringList / @c toStringReadySend formatters
 * of the command port, and the @c ICommand @c execute() override
 * that lets a @c CommandManager dispatch the command through the
 * generic command-pattern infrastructure in @ref command_pattern.h. The
 * @c operator[] overloads expose parameter values both by name and by
 * index against the @c m_qListParamNames / @c m_qListParamValues pair.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "command.h"
#include "command_manager.h"

#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <limits>
#include <type_traits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

namespace
{

/** Value returned by the subscript operators for an unknown parameter. */
CommandValue s_defaultValue;

/** @return @p text without leading and trailing whitespace and without a leading '+'. */
std::string_view numberText(std::string_view text)
{
    text = mne::trimmed(text);
    if (!text.empty() && text.front() == '+') {
        text.remove_prefix(1);
    }
    return text;
}

/** @return @p text as unsigned int, nothing if it is none. */
std::optional<unsigned int> toUnsigned(std::string_view text)
{
    text = numberText(text);
    unsigned int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || error != std::errc() || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

/** @return @p text as float, nothing if it is none or out of the range of float. */
std::optional<float> toFloatValue(std::string_view text)
{
    const std::optional<double> value = mne::to_double(text);
    if (!value || !std::isfinite(*value) || std::abs(*value) > static_cast<double>(std::numeric_limits<float>::max())) {
        return std::nullopt;
    }
    return static_cast<float>(*value);
}

/** Sets @p ok, if given, to @p value. */
void setOk(bool* ok, bool value)
{
    if (ok != nullptr) {
        *ok = value;
    }
}

/** @return @p value converted to the arithmetic type T, as a QVariant converts it. */
template<typename T>
T toNumber(const CommandValue::Storage& value, bool* ok)
{
    setOk(ok, true);
    return std::visit(
        [ok](const auto& held) -> T {
            using Held = std::decay_t<decltype(held)>;
            if constexpr (std::is_same_v<Held, std::monostate>) {
                setOk(ok, false);
                return T{};
            } else if constexpr (std::is_same_v<Held, std::string>) {
                std::optional<T> parsed;
                if constexpr (std::is_same_v<T, int>) {
                    parsed = mne::to_int(held);
                } else if constexpr (std::is_same_v<T, unsigned int>) {
                    parsed = toUnsigned(held);
                } else if constexpr (std::is_same_v<T, float>) {
                    parsed = toFloatValue(held);
                } else {
                    parsed = mne::to_double(held);
                }
                setOk(ok, parsed.has_value());
                return parsed.value_or(T{});
            } else if constexpr (std::is_integral_v<T> && std::is_floating_point_v<Held>) {
                return static_cast<T>(std::llround(held));
            } else {
                return static_cast<T>(held);
            }
        },
        value);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandValue::CommandValue(bool value)
: m_value(value)
{
}

//=============================================================================================================

CommandValue::CommandValue(int value)
: m_value(value)
{
}

//=============================================================================================================

CommandValue::CommandValue(unsigned int value)
: m_value(value)
{
}

//=============================================================================================================

CommandValue::CommandValue(float value)
: m_value(value)
{
}

//=============================================================================================================

CommandValue::CommandValue(double value)
: m_value(value)
{
}

//=============================================================================================================

CommandValue::CommandValue(const char* value)
: m_value(std::string(value != nullptr ? value : ""))
{
}

//=============================================================================================================

CommandValue::CommandValue(std::string value)
: m_value(std::move(value))
{
}

//=============================================================================================================

CommandValue CommandValue::fromTypeName(std::string_view typeName)
{
    if (typeName == "bool") {
        return CommandValue(false);
    }
    if (typeName == "int") {
        return CommandValue(0);
    }
    if (typeName == "uint") {
        return CommandValue(0U);
    }
    if (typeName == "float") {
        return CommandValue(0.0F);
    }
    if (typeName == "double") {
        return CommandValue(0.0);
    }
    if (typeName == "QString") {
        return CommandValue(std::string());
    }
    return {};
}

//=============================================================================================================

std::string CommandValue::typeName() const
{
    return std::visit(
        [](const auto& held) -> std::string {
            using Held = std::decay_t<decltype(held)>;
            if constexpr (std::is_same_v<Held, bool>) {
                return "bool";
            } else if constexpr (std::is_same_v<Held, int>) {
                return "int";
            } else if constexpr (std::is_same_v<Held, unsigned int>) {
                return "uint";
            } else if constexpr (std::is_same_v<Held, float>) {
                return "float";
            } else if constexpr (std::is_same_v<Held, double>) {
                return "double";
            } else if constexpr (std::is_same_v<Held, std::string>) {
                return "QString";
            } else {
                return {};
            }
        },
        m_value);
}

//=============================================================================================================

bool CommandValue::isValid() const
{
    return !std::holds_alternative<std::monostate>(m_value);
}

//=============================================================================================================

bool CommandValue::convertFrom(std::string_view text)
{
    const CommandValue source{std::string(text)};
    bool ok = true;
    return std::visit(
        [&](auto& held) -> bool {
            using Held = std::decay_t<decltype(held)>;
            if constexpr (std::is_same_v<Held, std::monostate>) {
                return false;
            } else if constexpr (std::is_same_v<Held, std::string>) {
                held = std::string(text);
                return true;
            } else if constexpr (std::is_same_v<Held, bool>) {
                held = source.toBool();
                return true;
            } else {
                const Held converted = toNumber<Held>(source.m_value, &ok);
                if (ok) {
                    held = converted;
                }
                return ok;
            }
        },
        m_value);
}

//=============================================================================================================

int CommandValue::toInt(bool* ok) const
{
    return toNumber<int>(m_value, ok);
}

//=============================================================================================================

unsigned int CommandValue::toUInt(bool* ok) const
{
    return toNumber<unsigned int>(m_value, ok);
}

//=============================================================================================================

float CommandValue::toFloat(bool* ok) const
{
    return toNumber<float>(m_value, ok);
}

//=============================================================================================================

double CommandValue::toDouble(bool* ok) const
{
    return toNumber<double>(m_value, ok);
}

//=============================================================================================================

bool CommandValue::toBool() const
{
    return std::visit(
        [](const auto& held) -> bool {
            using Held = std::decay_t<decltype(held)>;
            if constexpr (std::is_same_v<Held, std::monostate>) {
                return false;
            } else if constexpr (std::is_same_v<Held, std::string>) {
                return !(held.empty() || held == "0" || mne::iequals(held, "false"));
            } else {
                return held != Held{};
            }
        },
        m_value);
}

//=============================================================================================================

std::string CommandValue::toString() const
{
    return std::visit(
        [](const auto& held) -> std::string {
            using Held = std::decay_t<decltype(held)>;
            if constexpr (std::is_same_v<Held, std::monostate>) {
                return {};
            } else if constexpr (std::is_same_v<Held, std::string>) {
                return held;
            } else if constexpr (std::is_same_v<Held, bool>) {
                return held ? "true" : "false";
            } else {
                return std::format("{}", held);
            }
        },
        m_value);
}

//=============================================================================================================

void CommandValue::setValue(const CommandValue& value)
{
    m_value = value.m_value;
}

//=============================================================================================================

const CommandValue::Storage& CommandValue::storage() const
{
    return m_value;
}

//=============================================================================================================

Command::Command(bool p_bIsJson)
: m_bIsJson(p_bIsJson)
{
}

//=============================================================================================================

Command::Command(const std::string& p_sCommand, const mne::JsonObject& p_qCommandDescription, bool p_bIsJson)
: m_sCommand(p_sCommand)
, m_sDescription(p_qCommandDescription.value("description").to_string())
, m_bIsJson(p_bIsJson)
{
    for (const auto& [name, parameter] : p_qCommandDescription.value("parameters").to_object()) {
        m_qListParamNames.push_back(name);
        m_qListParamValues.push_back(CommandValue::fromTypeName(parameter.to_object().value("type").to_string()));
        m_qListParamDescriptions.emplace_back(parameter.to_object().value("description").to_string());
    }
}

//=============================================================================================================

Command::Command(const std::string& p_sCommand, const std::string& p_sDescription, bool p_bIsJson)
: m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
, m_bIsJson(p_bIsJson)
{
}

//=============================================================================================================

Command::Command(const std::string& p_sCommand, const std::string& p_sDescription,
                 const mne::StringList& p_qListParamNames, const std::vector<CommandValue>& p_qListParamValues, bool p_bIsJson)
: m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
, m_qListParamNames(p_qListParamNames)
, m_qListParamValues(p_qListParamValues)
, m_qListParamDescriptions(p_qListParamValues.size())
, m_bIsJson(p_bIsJson)
{
}

//=============================================================================================================

Command::Command(const std::string& p_sCommand, const std::string& p_sDescription,
                 const mne::StringList& p_qListParamNames, const std::vector<CommandValue>& p_qListParamValues, const mne::StringList& p_vecParameterDescriptions, bool p_bIsJson)
: m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
, m_bIsJson(p_bIsJson)
{
    if (p_qListParamNames.size() == p_qListParamValues.size()) {
        if (p_qListParamValues.size() == p_vecParameterDescriptions.size()) {
            m_qListParamNames = p_qListParamNames;
            m_qListParamValues = p_qListParamValues;
            m_qListParamDescriptions = p_vecParameterDescriptions;
        }
    } else {
        mne::Log::critical("error: description vector hasn't the same size like parameter map.\n");
        return;
    }
}

//=============================================================================================================

Command::Command(const Command& p_Command)
: ICommand(p_Command)
, m_sCommand(p_Command.m_sCommand)
, m_sDescription(p_Command.m_sDescription)
, m_qListParamNames(p_Command.m_qListParamNames)
, m_qListParamValues(p_Command.m_qListParamValues)
, m_qListParamDescriptions(p_Command.m_qListParamDescriptions)
, m_bIsJson(p_Command.m_bIsJson)
, m_pManager(p_Command.m_pManager)
{
}

//=============================================================================================================

Command::~Command() = default;

//=============================================================================================================

void Command::execute()
{
    for (const ExecutedCallback& callback : m_executed) {
        callback(*this);
    }
}

//=============================================================================================================

void Command::connectExecuted(ExecutedCallback callback)
{
    m_executed.push_back(std::move(callback));
}

//=============================================================================================================

void Command::reply(const std::string& p_sReply)
{
    if (m_pManager != nullptr) {
        m_pManager->response(p_sReply, *this);
    }
}

//=============================================================================================================

void Command::send()
{
    if (m_pManager != nullptr) {
        m_pManager->triggered(*this);
    }
}

//=============================================================================================================

mne::JsonObject Command::toJsonObject() const
{
    mne::JsonObject p_jsonCommandObject;
    p_jsonCommandObject.insert("description", m_sDescription);

    mne::JsonObject t_jsonAllParametersObject;
    for (std::size_t i = 0; i < m_qListParamValues.size(); ++i) {
        mne::JsonObject t_jsonParameterObject;
        t_jsonParameterObject.insert("description", m_qListParamDescriptions[i]);
        t_jsonParameterObject.insert("type", m_qListParamValues[i].typeName());
        t_jsonAllParametersObject.insert(m_qListParamNames[i], t_jsonParameterObject);
    }
    p_jsonCommandObject.insert("parameters", t_jsonAllParametersObject);

    return p_jsonCommandObject;
}

//=============================================================================================================

mne::StringList Command::toStringList() const
{
    std::string t_sParameters;
    for (const std::string& description : m_qListParamDescriptions) {
        t_sParameters += std::format("[{}]", description);
    }
    return {m_sCommand, t_sParameters, m_sDescription};
}

//=============================================================================================================

std::string Command::toStringReadySend() const
{
    std::string t_sParameters;
    for (std::size_t i = 0; i < m_qListParamNames.size(); ++i) {
        t_sParameters += std::format("\"{}\":\"{}\"", m_qListParamNames[i], m_qListParamValues[i].toString());

        if (i + 1 < m_qListParamNames.size()) {
            t_sParameters += ",";
        }
    }

    return std::format("\"{}\":{{{}}}", m_sCommand, t_sParameters);
}

//=============================================================================================================

Command& Command::operator=(const Command& rhs)
{
    if (this != &rhs) {
        m_sCommand = rhs.m_sCommand;
        m_sDescription = rhs.m_sDescription;
        m_qListParamNames = rhs.m_qListParamNames;
        m_qListParamValues = rhs.m_qListParamValues;
        m_qListParamDescriptions = rhs.m_qListParamDescriptions;
    }
    return *this;
}

//=============================================================================================================

CommandValue& Command::operator[](const std::string& key)
{
    const int idx = mne::index_of(m_qListParamNames, key);
    if (idx >= 0) {
        return m_qListParamValues[static_cast<std::size_t>(idx)];
    }
    return s_defaultValue;
}

//=============================================================================================================

CommandValue& Command::operator[](std::int32_t idx)
{
    if (idx >= 0 && std::ssize(m_qListParamValues) > idx) {
        return m_qListParamValues[static_cast<std::size_t>(idx)];
    }
    return s_defaultValue;
}

//=============================================================================================================

CommandValue Command::operator[](const std::string& key) const
{
    const int idx = mne::index_of(m_qListParamNames, key);
    if (idx >= 0) {
        return m_qListParamValues[static_cast<std::size_t>(idx)];
    }
    return {};
}
