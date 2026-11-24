//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     command_line.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Command-line parsing in standard C++, the counterpart of QCommandLineParser for the tools.
 *
 * Options are written --name value, --name=value or, for one-letter names, -n value. A single dash before a longer
 * name is accepted too (-fwd file), as MNE-C's tools spell their options. "--" ends the options; everything after it
 * and every argument that is not an option is positional.
 */

#ifndef BASE_COMMAND_LINE_H
#define BASE_COMMAND_LINE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
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
 * One option: a flag if it has no value name, otherwise an option that takes a value.
 *
 * @snippet ex_base/main.cpp command_line_usage
 *
 * @brief An option of a CommandLineParser.
 *
 * @since 3.0.0
 */
struct CommandLineOption
{
    std::vector<std::string> names; /**< Names without dashes, e.g. {"r", "repeats"}. */
    std::string description;        /**< One line for the help text. */
    std::string valueName;          /**< Placeholder of the value in the help text; empty for a flag. */
    std::string defaultValue;       /**< Value when the option is not given. */
};

//=============================================================================================================
/**
 * Parses a tool's command line against the options it declares and writes its help and version texts.
 *
 * @snippet ex_base/main.cpp command_line_usage
 *
 * @brief Command-line parser for the tools, without Qt.
 *
 * @since 3.0.0
 */
class BASESHARED_EXPORT CommandLineParser
{
public:
    //=========================================================================================================
    /**
     * @param[in] description   What the tool does, printed by --help.
     */
    explicit CommandLineParser(std::string description = {});

    /** Adds -h and --help, which print the help text. */
    void add_help_option();

    /** Adds -v and --version, which print the tool's name and version. */
    void add_version_option();

    //=========================================================================================================
    /**
     * Declares an option.
     *
     * @param[in] option   The option; none of its names may be declared already.
     *
     * @return false if the option has no name or a name is taken.
     */
    bool add_option(CommandLineOption option);

    //=========================================================================================================
    /**
     * Names a positional argument in the help text.
     *
     * @param[in] name          Placeholder, e.g. "file".
     * @param[in] description   One line for the help text.
     */
    void add_positional_argument(std::string name, std::string description);

    //=========================================================================================================
    /**
     * Parses @p args; args[0] is the program and is skipped. Parsing again starts afresh.
     *
     * @param[in] args   The arguments.
     *
     * @return false on an unknown option or a missing value; error_text() says which.
     */
    bool parse(std::span<const std::string> args);

    //=========================================================================================================
    /**
     * Parses main()'s arguments; see parse(std::span<const std::string>).
     *
     * @param[in] argc   Argument count.
     * @param[in] argv   Arguments; argv[0] is the program.
     *
     * @return false on an unknown option or a missing value.
     */
    bool parse(int argc, const char* const* argv);

    //=========================================================================================================
    /**
     * Parses main()'s arguments like QCommandLineParser::process(): prints the help or version text and exits
     * with 0 when asked for, prints the error and exits with 1 when parsing fails, and returns otherwise.
     *
     * @param[in] argc      Argument count.
     * @param[in] argv      Arguments.
     * @param[in] name      The tool's name.
     * @param[in] version   The tool's version.
     */
    void process(int argc, const char* const* argv, std::string_view name, std::string_view version);

    /**
     * @param[in] name   Any of the option's names.
     *
     * @return Whether the option was given.
     */
    bool is_set(std::string_view name) const;

    /**
     * @param[in] name   Any of the option's names.
     *
     * @return The option's last value, or its default value if it was not given.
     */
    std::string value(std::string_view name) const;

    /**
     * @param[in] name   Any of the option's names.
     *
     * @return Every value of the option in order, or its default value if it was not given.
     */
    std::vector<std::string> values(std::string_view name) const;

    /** @return The arguments that are not options, in order. */
    const std::vector<std::string>& positional_arguments() const
    {
        return m_positional;
    }

    /** @return Why the last parse() failed; empty if it did not. */
    const std::string& error_text() const
    {
        return m_error;
    }

    /**
     * @param[in] name   The tool's name, for the usage line.
     *
     * @return The help text.
     */
    std::string help_text(std::string_view name) const;

private:
    // Index into m_options of the option named @p name, or npos
    std::size_t find(std::string_view name) const;

    struct Positional
    {
        std::string name;
        std::string description;
    };

    std::string m_description;
    std::vector<CommandLineOption> m_options;
    std::vector<Positional> m_positionalHelp;
    std::vector<std::vector<std::string>> m_values; // per option, the values given; a flag gets one empty value
    std::vector<std::string> m_positional;
    std::string m_error;
};

} // namespace mne

#endif // BASE_COMMAND_LINE_H
