//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     command_line.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the CommandLineParser class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "command_line.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandLineParser::CommandLineParser(std::string description)
: m_description(std::move(description))
{
}

//=============================================================================================================

void CommandLineParser::add_help_option()
{
    add_option({{"h", "help"}, "Displays help on commandline options.", {}, {}});
}

//=============================================================================================================

void CommandLineParser::add_version_option()
{
    add_option({{"v", "version"}, "Displays version information.", {}, {}});
}

//=============================================================================================================

bool CommandLineParser::add_option(CommandLineOption option)
{
    if (option.names.empty())
        return false;
    for (const std::string& name : option.names) {
        if (name.empty() || name.front() == '-' || find(name) != std::string::npos)
            return false;
    }
    m_options.push_back(std::move(option));
    m_values.emplace_back();
    return true;
}

//=============================================================================================================

void CommandLineParser::add_positional_argument(std::string name, std::string description)
{
    m_positionalHelp.push_back({std::move(name), std::move(description)});
}

//=============================================================================================================

std::size_t CommandLineParser::find(std::string_view name) const
{
    for (std::size_t i = 0; i < m_options.size(); ++i) {
        if (std::ranges::find(m_options[i].names, name) != m_options[i].names.end())
            return i;
    }
    return std::string::npos;
}

//=============================================================================================================

bool CommandLineParser::parse(std::span<const std::string> args)
{
    for (auto& given : m_values)
        given.clear();
    m_positional.clear();
    m_error.clear();

    bool optionsEnded = false;
    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (optionsEnded || arg.size() < 2 || arg.front() != '-') {
            m_positional.push_back(arg);
            continue;
        }
        if (arg == "--") {
            optionsEnded = true;
            continue;
        }
        const std::string_view body = std::string_view(arg).substr(arg.starts_with("--") ? 2 : 1);
        const std::size_t equals = body.find('=');
        const std::string_view name = body.substr(0, equals);
        const std::size_t index = find(name);
        if (index == std::string::npos) {
            m_error = "Unknown option '" + std::string(name) + "'.";
            return false;
        }
        const CommandLineOption& option = m_options[index];
        if (option.valueName.empty()) {
            if (equals != std::string_view::npos) {
                m_error = "Unexpected value after '" + std::string(name) + "'.";
                return false;
            }
            m_values[index].emplace_back();
        } else if (equals != std::string_view::npos) {
            m_values[index].emplace_back(body.substr(equals + 1));
        } else if (i + 1 < args.size()) {
            m_values[index].push_back(args[++i]);
        } else {
            m_error = "Missing value after '" + arg + "'.";
            return false;
        }
    }
    return true;
}

//=============================================================================================================

bool CommandLineParser::parse(int argc, const char* const* argv)
{
    std::vector<std::string> args(argv, argv + std::max(argc, 0));
    return parse(args);
}

//=============================================================================================================

void CommandLineParser::process(int argc, const char* const* argv, std::string_view name, std::string_view version)
{
    if (!parse(argc, argv)) {
        std::println(stderr, "{}", m_error);
        std::exit(1);
    }
    if (find("help") != std::string::npos && is_set("help")) {
        std::print("{}", help_text(name));
        std::exit(0);
    }
    if (find("version") != std::string::npos && is_set("version")) {
        std::println("{} {}", name, version);
        std::exit(0);
    }
}

//=============================================================================================================

bool CommandLineParser::is_set(std::string_view name) const
{
    const std::size_t index = find(name);
    return index != std::string::npos && !m_values[index].empty();
}

//=============================================================================================================

std::string CommandLineParser::value(std::string_view name) const
{
    const std::size_t index = find(name);
    if (index == std::string::npos)
        return {};
    return m_values[index].empty() ? m_options[index].defaultValue : m_values[index].back();
}

//=============================================================================================================

std::vector<std::string> CommandLineParser::values(std::string_view name) const
{
    const std::size_t index = find(name);
    if (index == std::string::npos)
        return {};
    if (m_values[index].empty()) {
        if (m_options[index].defaultValue.empty())
            return {};
        return {m_options[index].defaultValue};
    }
    return m_values[index];
}

//=============================================================================================================

std::string CommandLineParser::help_text(std::string_view name) const
{
    std::string usage = "Usage: " + std::string(name);
    if (!m_options.empty())
        usage += " [options]";
    for (const Positional& positional : m_positionalHelp)
        usage += " " + positional.name;
    std::string text = usage + "\n";
    if (!m_description.empty())
        text += m_description + "\n";

    std::vector<std::pair<std::string, std::string>> rows;
    for (const CommandLineOption& option : m_options) {
        std::string spelled;
        for (const std::string& optionName : option.names) {
            if (!spelled.empty())
                spelled += ", ";
            spelled += (optionName.size() == 1 ? "-" : "--") + optionName;
        }
        if (!option.valueName.empty())
            spelled += " <" + option.valueName + ">";
        std::string description = option.description;
        // A description that already names its default keeps its own wording
        if (!option.defaultValue.empty() && description.find("default") == std::string::npos) {
            const bool sentence = description.ends_with('.');
            if (sentence)
                description.pop_back();
            description += " (default: " + option.defaultValue + ")" + (sentence ? "." : "");
        }
        rows.emplace_back(std::move(spelled), std::move(description));
    }
    std::size_t width = 0;
    for (const auto& row : rows)
        width = std::max(width, row.first.size());
    for (const Positional& positional : m_positionalHelp)
        width = std::max(width, positional.name.size());

    const auto table = [&](std::string_view heading, const std::vector<std::pair<std::string, std::string>>& entries) {
        if (entries.empty())
            return;
        text.append("\n").append(heading).append(":\n");
        for (const auto& [left, right] : entries)
            text.append("  ").append(left).append(width - left.size() + 2, ' ').append(right).append("\n");
    };
    table("Options", rows);
    std::vector<std::pair<std::string, std::string>> arguments;
    arguments.reserve(m_positionalHelp.size());
    for (const Positional& positional : m_positionalHelp)
        arguments.emplace_back(positional.name, positional.description);
    table("Arguments", arguments);
    return text;
}
