//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_process.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Tests run_process() by running this executable as the child, and the host description against QSysInfo.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/process.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QNetworkInterface>
#include <QOperatingSystemVersion>
#include <QSysInfo>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

constexpr std::string_view kChild = "--child";
constexpr std::size_t kLarge = 1 << 20;

std::filesystem::path self()
{
    return path_from_utf8(QCoreApplication::applicationFilePath().toStdString());
}

ProcessResult runChild(std::vector<std::string> args, const ProcessOptions& options = {})
{
    args.insert(args.begin(), std::string(kChild));
    return run_process(self(), args, options);
}

void put(std::FILE* stream, std::string_view text)
{
    std::fwrite(text.data(), 1, text.size(), stream);
}

/** What this executable does when started with --child. */
int child(int argc, char* argv[])
{
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);
#endif
    const std::string_view mode = argc > 2 ? argv[2] : "";
    if (mode == "args") {
        for (int i = 3; i < argc; ++i) {
            put(stdout, argv[i]);
            put(stdout, std::string_view("\0", 1));
        }
    } else if (mode == "streams") {
        put(stdout, std::string(kLarge, 'o'));
        put(stderr, std::string(kLarge, 'e'));
        return 3;
    } else if (mode == "env") {
        const char* value = std::getenv(argv[3]); // NOLINT(concurrency-mt-unsafe): single-threaded child
        put(stdout, value != nullptr ? value : "<unset>");
    } else if (mode == "cwd") {
        put(stdout, utf8_from_path(std::filesystem::current_path()));
    } else if (mode == "sleep") {
        std::this_thread::sleep_for(10s);
    } else if (mode == "crash") {
        std::abort();
    }
    return 0;
}

} // namespace

//=============================================================================================================
/**
 * @brief Tests run_process() and the host description.
 */
class TestBaseProcess : public QObject
{
    Q_OBJECT

private slots:
    void capturesBothStreamsAndTheExitCode();
    void passesArgumentsVerbatim();
    void setsEnvironmentAndWorkingDirectory();
    void streamsOutputToTheCallback();
    void killsAProgramThatTimesOut();
    void reportsAMissingProgram();
    void reportsACrash();
    void describesTheHostLikeQSysInfo();
    void listsTheHardwareAddressesLikeQNetworkInterface();
};

//=============================================================================================================

void TestBaseProcess::capturesBothStreamsAndTheExitCode()
{
    const ProcessResult result = runChild({"streams"});
    QVERIFY(result.started);
    QVERIFY(!result.ok());
    QCOMPARE(result.exit_code, 3);
    QVERIFY(result.std_out == std::string(kLarge, 'o'));
    QVERIFY(result.std_err == std::string(kLarge, 'e'));
}

//=============================================================================================================

void TestBaseProcess::passesArgumentsVerbatim()
{
    std::vector<std::string> args{"plain", "two words", "", "quote\"inside", "back\\slash", "trailing\\", "\\\"mixed\\\\\" end\\\\",
                                  "tab\there", "$HOME", "*", "a;b|c&d"};
#ifndef _WIN32 // the Windows C runtime decodes argv in the ANSI code page
    args.emplace_back("\xC2\xB5V \xE6\x97\xA5");
#endif
    std::vector<std::string> childArgs{"args"};
    childArgs.insert(childArgs.end(), args.begin(), args.end());
    const ProcessResult result = runChild(childArgs);
    QVERIFY(result.ok());

    std::vector<std::string> received;
    for (std::size_t start = 0, end = 0; (end = result.std_out.find('\0', start)) != std::string::npos; start = end + 1)
        received.emplace_back(result.std_out.substr(start, end - start));
    QCOMPARE(received, args);
}

//=============================================================================================================

void TestBaseProcess::setsEnvironmentAndWorkingDirectory()
{
    ProcessOptions options;
    options.environment = {{"MNE_TEST_PROCESS_VALUE", "a=b c"}};
    QCOMPARE(runChild({"env", "MNE_TEST_PROCESS_VALUE"}, options).std_out, std::string("a=b c"));
    QCOMPARE(runChild({"env", "MNE_TEST_PROCESS_VALUE"}).std_out, std::string("<unset>"));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const std::filesystem::path expected = std::filesystem::canonical(path_from_utf8(dir.path().toStdString()));
    options.working_dir = expected;
    const ProcessResult result = runChild({"cwd"}, options);
    QVERIFY(result.ok());
    QCOMPARE(std::filesystem::canonical(path_from_utf8(result.std_out)), expected);
}

//=============================================================================================================

void TestBaseProcess::streamsOutputToTheCallback()
{
    std::string out;
    std::string err;
    int calls = 0;
    ProcessOptions options;
    options.on_output = [&](ProcessChannel channel, std::string_view data) {
        ++calls;
        (channel == ProcessChannel::StdOut ? out : err).append(data);
    };
    const ProcessResult result = runChild({"streams"}, options);
    QCOMPARE(result.exit_code, 3);
    QVERIFY(result.std_out.empty() && result.std_err.empty());
    QVERIFY(out == std::string(kLarge, 'o'));
    QVERIFY(err == std::string(kLarge, 'e'));
    QVERIFY(calls > 2);
}

//=============================================================================================================

void TestBaseProcess::killsAProgramThatTimesOut()
{
    ProcessOptions options;
    options.timeout = 200ms;
    const auto start = std::chrono::steady_clock::now();
    const ProcessResult result = runChild({"sleep"}, options);
    QVERIFY(result.started);
    QVERIFY(result.timed_out);
    QVERIFY(!result.ok());
    QVERIFY(std::chrono::steady_clock::now() - start < 5s);
}

//=============================================================================================================

void TestBaseProcess::reportsAMissingProgram()
{
    const ProcessResult result = run_process("mne-cpp-no-such-program", {});
    QVERIFY(!result.started);
    QVERIFY(!result.ok());
    QVERIFY(!result.error.empty());
}

//=============================================================================================================

void TestBaseProcess::reportsACrash()
{
#ifdef _WIN32
    QSKIP("Windows reports a crash as an exit code");
#else
    const ProcessResult result = runChild({"crash"});
    QVERIFY(result.started);
    QVERIFY(result.crashed);
    QVERIFY(!result.ok());
#endif
}

//=============================================================================================================

void TestBaseProcess::describesTheHostLikeQSysInfo()
{
    QCOMPARE(host_name(), QSysInfo::machineHostName().toStdString());
    QCOMPARE(std::string(cpu_architecture()), QSysInfo::currentCpuArchitecture().toStdString());
    const std::string ours = os_pretty_name();
    const std::string qt = QSysInfo::prettyProductName().toStdString();
#if defined(Q_OS_LINUX)
    QCOMPARE(ours, qt);
#elif defined(Q_OS_MACOS)
    const QOperatingSystemVersion version = QOperatingSystemVersion::current();
    QVERIFY2(ours.starts_with("macOS ") && qt.find(ours.substr(6)) != std::string::npos, (ours + " vs " + qt).c_str());
    QVERIFY(ours.substr(6).starts_with(std::to_string(version.majorVersion()) + '.' + std::to_string(version.minorVersion())));
#else
    QVERIFY2(ours.find(QSysInfo::productVersion().toStdString()) != std::string::npos, (ours + " vs " + qt).c_str());
#endif
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    if (argc > 1 && std::string_view(argv[1]) == kChild)
        return child(argc, argv);
    QCoreApplication app(argc, argv);
    TestBaseProcess test;
    return QTest::qExec(&test, argc, argv);
}

//=============================================================================================================

void TestBaseProcess::listsTheHardwareAddressesLikeQNetworkInterface()
{
    std::vector<std::string> expected;
    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        const QNetworkInterface::InterfaceFlags flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp) || !flags.testFlag(QNetworkInterface::IsRunning) || flags.testFlag(QNetworkInterface::IsLoopBack) || flags.testFlag(QNetworkInterface::IsPointToPoint))
            continue;
        const QString address = iface.hardwareAddress();
        if (address.count(u':') == 5 && address != QStringLiteral("00:00:00:00:00:00"))
            expected.push_back(address.toUpper().toStdString());
    }
    std::vector<std::string> ours;
    for (const HardwareAddress& address : hardware_addresses())
        ours.push_back(std::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", address[0], address[1], address[2], address[3], address[4], address[5]));
    QCOMPARE(ours, expected);
}

#include "test_base_process.moc"
