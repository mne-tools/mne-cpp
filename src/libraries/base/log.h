//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     log.h
 * @author   Mainak Jas <mainakjas@gmail.com>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2019
 * @brief    Logging: a std::format facade with a replaceable sink, the console and file writer, and a capture sink for tests.
 *
 * Libraries report through @ref mne::Log, whose interface is standard C++ only. By default every
 * message is printed, results to stdout and diagnostics to stderr, optionally mirrored to a log file;
 * a sink set with @ref mne::Log::set_sink receives them instead. @ref mne::LogCapture records messages for
 * the lifetime of a scope. Qt programs connect Log and Qt's message handler with base/log_qt.h.
 */

#ifndef BASE_LOG_H
#define BASE_LOG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Writes library messages to the current sink; thread-safe.
 *
 * @snippet ex_base/main.cpp log_usage
 *
 * @snippet ex_base/main.cpp log_file_usage
 */
class BASESHARED_EXPORT Log
{
public:
    /** Severity: debug, info, warning and critical, like Qt's message types. */
    enum class Level
    {
        Debug,
        Info,
        Warning,
        Critical
    };

    /** Receives every message; may be called from any thread. */
    using Sink = std::function<void(Level level, std::string_view message)>;

    Log() = delete;

    //=========================================================================================================
    /**
     * Hands @p message to the current sink, unless @p level is below level().
     *
     * @param[in] level     Severity.
     * @param[in] message   UTF-8 text.
     */
    static void write(Level level, std::string_view message);

    /**
     * Formats @p args with @p format as std::format does and writes the result at debug level.
     *
     * @param[in] format   A std::format string, checked at compile time.
     * @param[in] args     The values it formats.
     */
    template<class... Args>
    static void debug(std::format_string<Args...> format, Args&&... args)
    {
        if (enabled(Level::Debug)) {
            write(Level::Debug, std::format(format, std::forward<Args>(args)...));
        }
    }

    /**
     * As debug(), at info level.
     *
     * @param[in] format   A std::format string, checked at compile time.
     * @param[in] args     The values it formats.
     */
    template<class... Args>
    static void info(std::format_string<Args...> format, Args&&... args)
    {
        if (enabled(Level::Info)) {
            write(Level::Info, std::format(format, std::forward<Args>(args)...));
        }
    }

    /**
     * As debug(), at warning level.
     *
     * @param[in] format   A std::format string, checked at compile time.
     * @param[in] args     The values it formats.
     */
    template<class... Args>
    static void warning(std::format_string<Args...> format, Args&&... args)
    {
        if (enabled(Level::Warning)) {
            write(Level::Warning, std::format(format, std::forward<Args>(args)...));
        }
    }

    /**
     * As debug(), at critical level.
     *
     * @param[in] format   A std::format string, checked at compile time.
     * @param[in] args     The values it formats.
     */
    template<class... Args>
    static void critical(std::format_string<Args...> format, Args&&... args)
    {
        if (enabled(Level::Critical)) {
            write(Level::Critical, std::format(format, std::forward<Args>(args)...));
        }
    }

    //=========================================================================================================
    /**
     * Sets the lowest level that is written; messages below it are dropped before they are formatted.
     *
     * @param[in] level   The new threshold; the default is the MNE_LOG_LEVEL build option.
     *
     * @return The threshold before.
     */
    static Level set_level(Level level) noexcept;

    /** @return The lowest level that is written. */
    static Level level() noexcept;

    /**
     * @param[in] level   The severity to check.
     *
     * @return Whether messages of @p level are written.
     */
    static bool enabled(Level level) noexcept;

    /**
     * Replaces the sink.
     *
     * @param[in] sink   The new sink; an empty one restores printing (see print()).
     *
     * @return The sink that was installed before, empty if messages were printed.
     */
    static Sink set_sink(Sink sink);

    //=========================================================================================================
    /**
     * Prints a message the way Log does without a sink: results (debug, info) to stdout, diagnostics
     * (warning, critical) to stderr, with coloured level tags when built with MNE_LOG_MODE=FORMATTED, and
     * to the log file if one is set. Ignores level() and the sink.
     *
     * @param[in] level     Severity.
     * @param[in] message   UTF-8 text.
     */
    static void print(Level level, std::string_view message);

    /**
     * Additionally writes every printed message to a log file, appended, one timestamped and
     * level-tagged line per message.
     *
     * @param[in] path   The log file; an empty path stops file logging.
     *
     * @return Whether the file could be opened for writing.
     */
    static bool set_file(const std::filesystem::path& path);

    /** @return The log file, empty if file logging is off. */
    static std::filesystem::path file();
};

//=============================================================================================================
/**
 * @brief Records every message instead of passing it on, while it exists; replaces @c QTest::ignoreMessage.
 *
 * Records every level; the sink and level before are restored on destruction, so captures nest when
 * destroyed in reverse order.
 *
 * @snippet ex_base/main.cpp log_capture_usage
 */
class BASESHARED_EXPORT LogCapture
{
public:
    /** One recorded message. */
    struct Entry
    {
        Log::Level level = Log::Level::Debug;
        std::string message;
    };

    LogCapture();
    ~LogCapture();

    LogCapture(const LogCapture&) = delete;
    LogCapture& operator=(const LogCapture&) = delete;

    /** @return The messages recorded so far, oldest first. */
    std::vector<Entry> entries() const;

    //=========================================================================================================
    /**
     * @param[in] level      Severity to count.
     * @param[in] contains   Only messages containing this text; empty counts all.
     *
     * @return The number of recorded messages of @p level that contain @p contains.
     */
    int count(Log::Level level, std::string_view contains = {}) const;

    /** Forgets the messages recorded so far. */
    void clear();

private:
    struct State;
    std::shared_ptr<State> m_state;
    Log::Sink m_previous;
    Log::Level m_previousLevel;
};

} // namespace mne

#endif // BASE_LOG_H
