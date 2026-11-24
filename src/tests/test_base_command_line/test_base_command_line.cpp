//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_command_line.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests CommandLineParser, also against the QCommandLineParser it replaces in the tools.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/command_line.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
/**
 * DECLARE CLASS TestBaseCommandLine
 *
 * @brief Tests the helpers in base/command_line.h.
 */
class TestBaseCommandLine : public QObject
{
    Q_OBJECT

private slots:
    void optionsAndPositionals();
    void defaultsAndRepeatedValues();
    void errors();
    void declaringATakenNameFails();
    void helpTextListsEverything();
    void agreesWithQCommandLineParser_data();
    void agreesWithQCommandLineParser();

private:
    static CommandLineParser makeParser();
};

//=============================================================================================================

CommandLineParser TestBaseCommandLine::makeParser()
{
    CommandLineParser parser("Test tool.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"quick"}, "Short runs.", {}, {}});
    parser.add_option({{"r", "repeats"}, "Runs per measurement.", "n", "3"});
    parser.add_option({{"fwd"}, "Forward solution.", "file", {}});
    return parser;
}

//=============================================================================================================

void TestBaseCommandLine::optionsAndPositionals()
{
    CommandLineParser parser = makeParser();
    const std::vector<std::string> args{"tool", "--quick", "-r", "5", "-fwd", "a.fif", "in.fif", "--", "--quick"};
    QVERIFY(parser.parse(args));
    QVERIFY(parser.is_set("quick"));
    QVERIFY(parser.is_set("repeats"));
    QCOMPARE(parser.value("repeats"), std::string("5"));
    QCOMPARE(parser.value("r"), std::string("5"));
    QCOMPARE(parser.value("fwd"), std::string("a.fif"));
    QCOMPARE(parser.positional_arguments(), (std::vector<std::string>{"in.fif", "--quick"}));

    QVERIFY(parser.parse(std::vector<std::string>{"tool", "--repeats=7", "-"}));
    QVERIFY(!parser.is_set("quick"));
    QCOMPARE(parser.value("repeats"), std::string("7"));
    QCOMPARE(parser.positional_arguments(), (std::vector<std::string>{"-"}));
}

//=============================================================================================================

void TestBaseCommandLine::defaultsAndRepeatedValues()
{
    CommandLineParser parser = makeParser();
    QVERIFY(parser.parse(std::vector<std::string>{"tool"}));
    QVERIFY(!parser.is_set("repeats"));
    QCOMPARE(parser.value("repeats"), std::string("3"));
    QCOMPARE(parser.values("repeats"), (std::vector<std::string>{"3"}));
    QVERIFY(parser.values("fwd").empty());
    QVERIFY(parser.value("unknown").empty());

    QVERIFY(parser.parse(std::vector<std::string>{"tool", "--fwd", "a.fif", "--fwd", "-b.fif"}));
    QCOMPARE(parser.value("fwd"), std::string("-b.fif"));
    QCOMPARE(parser.values("fwd"), (std::vector<std::string>{"a.fif", "-b.fif"}));
}

//=============================================================================================================

void TestBaseCommandLine::errors()
{
    CommandLineParser parser = makeParser();
    QVERIFY(!parser.parse(std::vector<std::string>{"tool", "--nope"}));
    QCOMPARE(parser.error_text(), std::string("Unknown option 'nope'."));
    QVERIFY(!parser.parse(std::vector<std::string>{"tool", "--repeats"}));
    QCOMPARE(parser.error_text(), std::string("Missing value after '--repeats'."));
    QVERIFY(!parser.parse(std::vector<std::string>{"tool", "--quick=1"}));
    QCOMPARE(parser.error_text(), std::string("Unexpected value after 'quick'."));
    QVERIFY(parser.parse(std::vector<std::string>{"tool"}));
    QVERIFY(parser.error_text().empty());
}

//=============================================================================================================

void TestBaseCommandLine::declaringATakenNameFails()
{
    CommandLineParser parser = makeParser();
    QVERIFY(!parser.add_option({{"repeats"}, "Again.", {}, {}}));
    QVERIFY(!parser.add_option({{"x", "r"}, "Short name taken.", {}, {}}));
    QVERIFY(!parser.add_option({{}, "No name.", {}, {}}));
    QVERIFY(!parser.add_option({{"--dashes"}, "Dashes are not part of a name.", {}, {}}));
    QVERIFY(parser.add_option({{"x"}, "Free.", {}, {}}));
}

//=============================================================================================================

void TestBaseCommandLine::helpTextListsEverything()
{
    CommandLineParser parser = makeParser();
    parser.add_positional_argument("input", "Raw data file.");
    const std::string help = parser.help_text("tool");
    QVERIFY(help.starts_with("Usage: tool [options] input\nTest tool.\n"));
    for (const char* expected : {"-h, --help", "-v, --version", "--quick", "-r, --repeats <n>", "Runs per measurement (default: 3).",
                                 "--fwd <file>", "Arguments:", "input"}) {
        QVERIFY2(help.find(expected) != std::string::npos, expected);
    }
}

//=============================================================================================================

void TestBaseCommandLine::agreesWithQCommandLineParser_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::newRow("long with space") << QStringList{"tool", "--repeats", "5", "x"};
    QTest::newRow("long with equals") << QStringList{"tool", "--repeats=5", "--quick"};
    QTest::newRow("short") << QStringList{"tool", "-r", "9", "a", "b"};
    QTest::newRow("value starting with a dash") << QStringList{"tool", "--fwd", "-a.fif"};
    QTest::newRow("end of options") << QStringList{"tool", "--", "--quick", "-r"};
    QTest::newRow("repeated") << QStringList{"tool", "-r", "1", "--repeats", "2"};
}

void TestBaseCommandLine::agreesWithQCommandLineParser()
{
    QFETCH(QStringList, args);

    QCommandLineParser qt;
    qt.addOptions({QCommandLineOption("quick", "Short runs."),
                   QCommandLineOption({"r", "repeats"}, "Runs per measurement.", "n", "3"),
                   QCommandLineOption("fwd", "Forward solution.", "file")});
    QVERIFY(qt.parse(args));

    std::vector<std::string> stdArgs;
    for (const QString& arg : args)
        stdArgs.push_back(arg.toStdString());
    CommandLineParser parser = makeParser();
    QVERIFY(parser.parse(stdArgs));

    for (const char* name : {"quick", "repeats", "fwd"}) {
        QCOMPARE(parser.is_set(name), qt.isSet(name));
        QCOMPARE(QString::fromStdString(parser.value(name)), qt.value(name));
    }
    QStringList positional;
    for (const std::string& arg : parser.positional_arguments())
        positional << QString::fromStdString(arg);
    QCOMPARE(positional, qt.positionalArguments());
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestBaseCommandLine)
#include "test_base_command_line.moc"
