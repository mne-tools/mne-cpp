//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scoped_log_capture.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Records the messages written through @ref mne::Log while a test runs, so the test can assert on them.
 *
 * Replaces @c QTest::ignoreMessage() and @c QTest::failOnWarning() for messages of the MNE-CPP libraries and
 * programs, which no longer reach Qt's message handler:
 *
 * @code
 * ScopedLogCapture capture;
 * QVERIFY(!fit(...));
 * QVERIFY2(capture.contains(mne::Log::Level::Warning, "No data or Projectors passed"), capture.dump().c_str());
 * QVERIFY2(!capture.contains(mne::Log::Level::Warning), capture.dump().c_str());
 * @endcode
 */

#ifndef MNE_TESTFRAMES_SCOPED_LOG_CAPTURE_H
#define MNE_TESTFRAMES_SCOPED_LOG_CAPTURE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//=============================================================================================================
/**
 * @brief Installs a @ref mne::Log sink that records every message (and, with @p echo, passes it on); the sink before
 *        is restored on destruction. Does not change @ref mne::Log::level.
 */
class ScopedLogCapture
{
public:
    /** One recorded message. */
    struct Entry
    {
        mne::Log::Level level = mne::Log::Level::Debug;
        std::string message;
    };

    /**
     * @param[in] echo   Also pass every message on to the sink before (or print it), so it stays in the test log.
     */
    explicit ScopedLogCapture(bool echo = false)
    : m_state(std::make_shared<State>())
    {
        auto previous = std::make_shared<mne::Log::Sink>();
        m_previous = previous;
        *previous = mne::Log::set_sink([state = m_state, previous, echo](mne::Log::Level level, std::string_view message) {
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->entries.push_back({level, std::string(message)});
            }
            if (echo) {
                if (*previous) {
                    (*previous)(level, message);
                } else {
                    mne::Log::print(level, message);
                }
            }
        });
    }

    ~ScopedLogCapture()
    {
        mne::Log::set_sink(std::move(*m_previous));
    }

    ScopedLogCapture(const ScopedLogCapture&) = delete;
    ScopedLogCapture& operator=(const ScopedLogCapture&) = delete;

    /** @return The messages recorded so far, oldest first. */
    std::vector<Entry> entries() const
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        return m_state->entries;
    }

    /**
     * @param[in] level   Severity; messages of this level or above count.
     * @param[in] text    Only messages containing this text; empty counts all.
     *
     * @return The number of such messages recorded.
     */
    int count(mne::Log::Level level, std::string_view text = {}) const
    {
        return countIf(level, [text](const std::string& message) { return message.find(text) != std::string::npos; });
    }

    /**
     * @param[in] level     Severity; messages of this level or above count.
     * @param[in] pattern   ECMAScript regular expression searched in each message.
     *
     * @return The number of such messages recorded.
     */
    int count(mne::Log::Level level, const std::regex& pattern) const
    {
        return countIf(level, [&pattern](const std::string& message) { return std::regex_search(message, pattern); });
    }

    /** @return Whether count(@p level, @p text) is not zero. */
    bool contains(mne::Log::Level level, std::string_view text = {}) const
    {
        return count(level, text) > 0;
    }

    /** @return Whether count(@p level, @p pattern) is not zero. */
    bool contains(mne::Log::Level level, const std::regex& pattern) const
    {
        return count(level, pattern) > 0;
    }

    /** @return All recorded messages, one per line with their level, for QVERIFY2 failure messages. */
    std::string dump() const
    {
        static constexpr const char* names[] = {"debug", "info", "warning", "critical"};
        std::string text = "Logged messages:";
        for (const Entry& entry : entries()) {
            text += "\n  [" + std::string(names[static_cast<int>(entry.level)]) + "] " + entry.message;
        }
        return text;
    }

    /** Forgets the messages recorded so far. */
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        m_state->entries.clear();
    }

private:
    struct State
    {
        mutable std::mutex mutex;
        std::vector<Entry> entries;
    };

    template<class Predicate>
    int countIf(mne::Log::Level level, Predicate predicate) const
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        return static_cast<int>(std::count_if(m_state->entries.begin(), m_state->entries.end(), [&](const Entry& entry) {
            return entry.level >= level && predicate(entry.message);
        }));
    }

    std::shared_ptr<State> m_state;
    std::shared_ptr<mne::Log::Sink> m_previous;
};

#endif // MNE_TESTFRAMES_SCOPED_LOG_CAPTURE_H
