//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_logger.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2026
 * @brief    Tests the console and file writer of Log and its facade with the capture sink.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include <mne/base/process.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================

namespace
{

std::filesystem::path g_self;

constexpr const char* kLogAtExit = "--log-at-exit";

/** What this executable does when started with --log-at-exit: log from an exit handler, after the statics are gone. */
int logAtExit()
{
    // Registered before Log's statics are built, so it runs after they were destroyed, like Qt tearing down a
    // QApplication that the program left behind when QCommandLineParser called exit()
    std::atexit([] { Log::warning("late message from an exit handler"); });
    Log::info("logging works");
    return 0;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestUtilsLogger
 *
 * @brief The TestUtilsLogger class provides tests for Log.
 *
 */
class TestUtilsLogger : public QObject
{
    Q_OBJECT

public:
    TestUtilsLogger();

private slots:
    void initTestCase();
    void testWritesMessagesToFile();
    void testDisablingStopsFileOutput();
    void testRoutesDiagnosticsToStderr();
    void testLogCaptureSwallowsAndRecords();
    void testLogCaptureNestsAndRestores();
    void testLogCustomSink();
    void testLogCaptureFromManyThreads();
    void testLogLevelSkipsFormatting();
    void testLogsAfterStaticDestruction();
    void cleanupTestCase();

private:
    QTemporaryDir m_scratch;
    QString m_sLogFile;
    QString readLog() const;
};

//=============================================================================================================

TestUtilsLogger::TestUtilsLogger()
: m_sLogFile(m_scratch.filePath("mne_test_utils_logger.log"))
{
}

//=============================================================================================================

QString TestUtilsLogger::readLog() const
{
    QFile file(m_sLogFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream stream(&file);

    return stream.readAll();
}

//=============================================================================================================

void TestUtilsLogger::initTestCase()
{
    QFile::remove(m_sLogFile);
    // The default follows the MNE_LOG_LEVEL build option; these tests need every level.
    Log::set_level(Log::Level::Debug);
}

//=============================================================================================================

void TestUtilsLogger::testWritesMessagesToFile()
{
    QVERIFY(Log::set_file(m_sLogFile.toStdString()));
    QCOMPARE(QString::fromStdString(Log::file().string()), m_sLogFile);

    Log::warning("test-logger-warning");
    Log::critical("test-logger-critical");

    const QString sContent = readLog();

    QVERIFY(sContent.contains("test-logger-warning"));
    QVERIFY(sContent.contains("test-logger-critical"));

    // The level has to be recoverable from the file, without terminal colours.
    QVERIFY(sContent.contains("[WARN]"));
    QVERIFY(sContent.contains("[CRIT]"));
    QVERIFY(!sContent.contains("\033"));
}

//=============================================================================================================

void TestUtilsLogger::testDisablingStopsFileOutput()
{
    QVERIFY(Log::set_file({}));
    QVERIFY(Log::file().empty());

    Log::warning("test-logger-must-not-appear");

    QVERIFY(!readLog().contains("test-logger-must-not-appear"));
}

//=============================================================================================================

void TestUtilsLogger::testRoutesDiagnosticsToStderr()
{
    // Tools are piped: results on stdout must not be interleaved with warnings and errors.
    std::ostringstream out;
    std::ostringstream err;
    std::streambuf* oldOut = std::cout.rdbuf(out.rdbuf());
    std::streambuf* oldErr = std::cerr.rdbuf(err.rdbuf());

    Log::info("test-logger-info");
    Log::warning("test-logger-warn-stream");
    Log::critical("test-logger-crit-stream");

    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);

    QVERIFY(out.str().find("test-logger-info") != std::string::npos);
    QVERIFY(out.str().find("test-logger-warn-stream") == std::string::npos);
    QVERIFY(out.str().find("test-logger-crit-stream") == std::string::npos);
    QVERIFY(err.str().find("test-logger-warn-stream") != std::string::npos);
    QVERIFY(err.str().find("test-logger-crit-stream") != std::string::npos);
    QVERIFY(err.str().find("test-logger-info") == std::string::npos);
}

//=============================================================================================================

void TestUtilsLogger::testLogCaptureSwallowsAndRecords()
{
    StringList passedOn;
    const Log::Sink previousSink = Log::set_sink([&passedOn](Log::Level, std::string_view message) {
        passedOn.emplace_back(message);
    });
    {
        LogCapture capture;
        Log::warning("disk almost full");
        Log::warning("disk full");
        Log::info("disk checked");

        QCOMPARE(capture.entries().size(), std::size_t(3));
        QCOMPARE(capture.entries()[1].level, Log::Level::Warning);
        QCOMPARE(capture.entries()[1].message, std::string("disk full"));
        QCOMPARE(capture.count(Log::Level::Warning), 2);
        QCOMPARE(capture.count(Log::Level::Warning, "almost"), 1);
        QCOMPARE(capture.count(Log::Level::Critical), 0);

        capture.clear();
        QVERIFY(capture.entries().empty());
    }
    Log::warning("after capture");
    Log::set_sink(previousSink);

    // Captured messages never reach the sink before; once the capture is gone, that sink gets them again.
    QCOMPARE(passedOn.size(), std::size_t(1));
    QCOMPARE(passedOn[0], std::string("after capture"));
}

//=============================================================================================================

void TestUtilsLogger::testLogCaptureNestsAndRestores()
{
    LogCapture outer;
    Log::info("to outer");
    {
        LogCapture inner;
        Log::info("to inner");
        QCOMPARE(inner.count(Log::Level::Info), 1);
    }
    Log::info("to outer again");
    QCOMPARE(outer.count(Log::Level::Info), 2);
    QCOMPARE(outer.count(Log::Level::Info, "inner"), 0);
}

//=============================================================================================================

void TestUtilsLogger::testLogCustomSink()
{
    StringList seen;
    Log::Sink previous = Log::set_sink([&seen](Log::Level level, std::string_view message) {
        seen.emplace_back(std::to_string(static_cast<int>(level)) + ":" + std::string(message));
    });
    Log::critical("boom {} of {:.1f}", 3, 4.0);
    Log::write(Log::Level::Debug, "trace");
    Log::Sink mine = Log::set_sink(std::move(previous));

    QVERIFY(mine);
    QCOMPARE(seen.size(), std::size_t(2));
    QCOMPARE(seen[0], std::string("3:boom 3 of 4.0"));
    QCOMPARE(seen[1], std::string("0:trace"));

    // An empty sink restores printing.
    std::ostringstream out;
    std::streambuf* oldOut = std::cout.rdbuf(out.rdbuf());
    Log::set_sink([](Log::Level, std::string_view) {});
    Log::set_sink({});
    Log::info("printed");
    std::cout.rdbuf(oldOut);
    QVERIFY(out.str().find("printed") != std::string::npos);
}

//=============================================================================================================

void TestUtilsLogger::testLogCaptureFromManyThreads()
{
    constexpr int kThreads = 8;
    constexpr int kMessages = 500;
    LogCapture capture;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < kMessages; ++i) {
                Log::warning("thread {}", t);
            }
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    QCOMPARE(capture.count(Log::Level::Warning), kThreads * kMessages);
    QCOMPARE(capture.count(Log::Level::Warning, "thread 3"), kMessages);
}

//=============================================================================================================

namespace
{
// Counts how often it is formatted
struct Probe
{
    int* formatted;
};
} // namespace

template<>
struct std::formatter<Probe> : std::formatter<int>
{
    auto format(const Probe& probe, std::format_context& ctx) const
    {
        return std::formatter<int>::format(++*probe.formatted, ctx);
    }
};

void TestUtilsLogger::testLogLevelSkipsFormatting()
{
    LogCapture capture;
    QCOMPARE(Log::level(), Log::Level::Debug);
    int formatted = 0;

    const Log::Level previous = Log::set_level(Log::Level::Warning);
    QCOMPARE(previous, Log::Level::Debug);
    QVERIFY(!Log::enabled(Log::Level::Info));
    QVERIFY(Log::enabled(Log::Level::Critical));
    Log::debug("{}", Probe{&formatted});
    Log::info("{}", Probe{&formatted});
    Log::write(Log::Level::Info, "dropped too");
    Log::warning("{}", Probe{&formatted});
    QCOMPARE(formatted, 1);
    QCOMPARE(static_cast<int>(capture.entries().size()), 1);
    QCOMPARE(capture.count(Log::Level::Warning, "1"), 1);

    Log::set_level(previous);
    Log::debug("{}", Probe{&formatted});
    QCOMPARE(formatted, 2);
    QCOMPARE(static_cast<int>(capture.entries().size()), 2);
}

//=============================================================================================================

void TestUtilsLogger::testLogsAfterStaticDestruction()
{
    const ProcessResult result = run_process(g_self, std::vector<std::string>{kLogAtExit});
    QVERIFY2(result.started, result.error.c_str());
    QVERIFY2(!result.crashed && result.exit_code == 0, result.std_err.c_str());
    QVERIFY(result.std_err.find("late message from an exit handler") != std::string::npos);
}

//=============================================================================================================

void TestUtilsLogger::cleanupTestCase()
{
    Log::set_file({});
    QFile::remove(m_sLogFile);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    g_self = std::filesystem::absolute(path_from_utf8(argv[0]));
    if (argc > 1 && std::string_view(argv[1]) == kLogAtExit) {
        return logAtExit();
    }
    TestUtilsLogger test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_base_logger.moc"
