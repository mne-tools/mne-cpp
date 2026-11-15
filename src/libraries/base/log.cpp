//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     log.cpp
 * @author   Mainak Jas <mainakjas@gmail.com>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2019
 * @brief    Definition of @ref mne::Log and @ref mne::LogCapture.
 *
 * The console writer serialises its writes through one mutex, so lines from different threads never
 * interleave inside one record.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "log.h"

#include "string_utils.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QString>
#include <QUtf8StringView>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

struct SinkSlot
{
    std::mutex mutex;
    std::shared_ptr<const Log::Sink> sink;
};

#ifdef MNE_LOG_LEVEL
constexpr Log::Level kDefaultLevel = static_cast<Log::Level>(MNE_LOG_LEVEL);
#else
constexpr Log::Level kDefaultLevel = Log::Level::Debug;
#endif

std::atomic<Log::Level> g_level{kDefaultLevel};

struct Console
{
    std::mutex mutex;
    std::filesystem::path path;
    std::ofstream file;
};

Console& console()
{
    static Console instance;
    return instance;
}

constexpr std::array<std::string_view, 4> kTags{"DEBUG", "INFO", "WARN", "CRIT"};

std::string timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif
    std::array<char, 32> date{};
    std::strftime(date.data(), date.size(), "%Y-%m-%d %H:%M:%S", &local);
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    return std::format("{}.{:03}", date.data(), millis);
}

void print(Log::Level level, std::string_view text)
{
    const auto index = static_cast<std::size_t>(level);
    // Results go to stdout and diagnostics to stderr, so piped tool output stays parseable.
    std::ostream& stream = level >= Log::Level::Warning ? std::cerr : std::cout;
    if (!text.empty() && text.back() == '\n') {
        text.remove_suffix(1);
    }

    Console& out = console();
    std::lock_guard<std::mutex> lock(out.mutex);
    if (out.file.is_open()) {
        // Flushed per line, so an unattended session keeps everything up to a crash.
        out.file << timestamp() << " [" << kTags[index] << "] " << trimmed(text) << std::endl;
    }
#ifdef MNE_LOG_MODE_FORMATTED
    constexpr std::array<std::string_view, 4> colors{"\033[32;1m", "\033[32;1m", "\033[33;1m", "\033[31;1m"};
    for (std::size_t start = 0, end = 0; end != std::string_view::npos; start = end + 1) {
        end = text.find('\n', start);
        stream << colors[index] << '[' << kTags[index] << "] \033[0m" << text.substr(start, end - start) << '\n';
    }
#else
    stream << text << '\n';
#endif
    stream.flush();
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString& message)
{
    Log::Level level = Log::Level::Debug;
    switch (type) {
        case QtDebugMsg:
            level = Log::Level::Debug;
            break;
        case QtInfoMsg:
            level = Log::Level::Info;
            break;
        case QtWarningMsg:
            level = Log::Level::Warning;
            break;
        case QtCriticalMsg:
        case QtFatalMsg:
            level = Log::Level::Critical;
            break;
    }
    if (type != QtFatalMsg && !Log::enabled(level)) {
        return;
    }
    print(level, message.toStdString());
}

SinkSlot& sinkSlot()
{
    static SinkSlot slot;
    return slot;
}

void forwardToQt(Log::Level level, std::string_view message)
{
    const QUtf8StringView text(message);
    switch (level) {
        case Log::Level::Debug:
            qDebug().noquote() << text;
            break;
        case Log::Level::Info:
            qInfo().noquote() << text;
            break;
        case Log::Level::Warning:
            qWarning().noquote() << text;
            break;
        case Log::Level::Critical:
            qCritical().noquote() << text;
            break;
    }
}

} // namespace

//=============================================================================================================

struct LogCapture::State
{
    mutable std::mutex mutex;
    std::vector<Entry> entries;
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void Log::write(Level level, std::string_view message)
{
    if (!enabled(level)) {
        return;
    }
    std::shared_ptr<const Sink> sink;
    {
        std::lock_guard<std::mutex> lock(sinkSlot().mutex);
        sink = sinkSlot().sink;
    }
    // Called outside the lock, so a sink may itself change the sink.
    if (sink) {
        (*sink)(level, message);
    } else {
        forwardToQt(level, message);
    }
}

//=============================================================================================================

Log::Level Log::set_level(Level level) noexcept
{
    return g_level.exchange(level, std::memory_order_relaxed);
}

//=============================================================================================================

Log::Level Log::level() noexcept
{
    return g_level.load(std::memory_order_relaxed);
}

//=============================================================================================================

bool Log::enabled(Level level) noexcept
{
    return level >= g_level.load(std::memory_order_relaxed);
}

//=============================================================================================================

Log::Sink Log::set_sink(Sink sink)
{
    std::shared_ptr<const Sink> next = sink ? std::make_shared<const Sink>(std::move(sink)) : nullptr;
    std::lock_guard<std::mutex> lock(sinkSlot().mutex);
    std::swap(sinkSlot().sink, next);
    return next ? *next : Sink();
}

//=============================================================================================================

void Log::install_qt_message_handler()
{
    qInstallMessageHandler(qtMessageHandler);
}

//=============================================================================================================

bool Log::set_file(const std::filesystem::path& path)
{
    std::ofstream file;
    if (!path.empty()) {
        file.open(path, std::ios::app);
        if (!file) {
            return false;
        }
    }
    std::lock_guard<std::mutex> lock(console().mutex);
    console().path = path;
    console().file = std::move(file);
    return true;
}

//=============================================================================================================

std::filesystem::path Log::file()
{
    std::lock_guard<std::mutex> lock(console().mutex);
    return console().path;
}

//=============================================================================================================

LogCapture::LogCapture()
: m_state(std::make_shared<State>())
, m_previousLevel(Log::set_level(Log::Level::Debug))
{
    m_previous = Log::set_sink([state = m_state](Log::Level level, std::string_view message) {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->entries.push_back({level, std::string(message)});
    });
}

//=============================================================================================================

LogCapture::~LogCapture()
{
    Log::set_sink(std::move(m_previous));
    Log::set_level(m_previousLevel);
}

//=============================================================================================================

std::vector<LogCapture::Entry> LogCapture::entries() const
{
    std::lock_guard<std::mutex> lock(m_state->mutex);
    return m_state->entries;
}

//=============================================================================================================

int LogCapture::count(Log::Level level, std::string_view contains) const
{
    std::lock_guard<std::mutex> lock(m_state->mutex);
    return static_cast<int>(std::count_if(m_state->entries.begin(), m_state->entries.end(), [&](const Entry& entry) {
        return entry.level == level && entry.message.find(contains) != std::string::npos;
    }));
}

//=============================================================================================================

void LogCapture::clear()
{
    std::lock_guard<std::mutex> lock(m_state->mutex);
    m_state->entries.clear();
}
