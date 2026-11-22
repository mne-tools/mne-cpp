//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_string_utils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests the string and path helpers against the QString and QFileInfo calls they replace.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <clocale>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
/**
 * DECLARE CLASS TestUtilsStringUtils
 *
 * @brief Tests the helpers in utils/string_utils.h.
 */
class TestUtilsStringUtils : public QObject
{
    Q_OBJECT

private slots:
    void trimmedMatchesQString_data();
    void trimmedMatchesQString();
    void splitMatchesQString_data();
    void splitMatchesQString();
    void splitWhitespaceMatchesQString();
    void caseHelpersMatchQString();
    void nameListHelpersMatchQStringList();
    void toDoubleMatchesQString_data();
    void toDoubleMatchesQString();
    void toIntMatchesQString_data();
    void toIntMatchesQString();
    void toDoubleIgnoresTheLocale();
    void utf8PathsRoundTrip();
    void latin1AndUtf8MatchQString();
    void toHexMatchesQByteArray();
    void base64MatchesQByteArray();
    void iso8601MatchesQDateTime_data();
    void iso8601MatchesQDateTime();
    void iso8601RejectsMalformedText();
    void executableDirIsTheApplicationDir();
    void resourcePathPrefersTheInstalledResources();
};

//=============================================================================================================

void TestUtilsStringUtils::trimmedMatchesQString_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("empty") << QString();
    QTest::newRow("blank") << QStringLiteral(" \t\r\n\v\f ");
    QTest::newRow("both sides") << QStringLiteral("\t MEG 0113 \r\n");
    QTest::newRow("inner kept") << QStringLiteral("a  b");
    QTest::newRow("none") << QStringLiteral("EEG001");
}

//=============================================================================================================

void TestUtilsStringUtils::trimmedMatchesQString()
{
    QFETCH(QString, text);
    QCOMPARE(QString::fromStdString(std::string(trimmed(text.toStdString()))), text.trimmed());
}

//=============================================================================================================

void TestUtilsStringUtils::splitMatchesQString_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("skipEmpty");
    for (const bool skip : {false, true}) {
        const char* suffix = skip ? " skip empty" : "";
        QTest::addRow("empty%s", suffix) << QString() << skip;
        QTest::addRow("one field%s", suffix) << QStringLiteral("Fp1") << skip;
        QTest::addRow("fields%s", suffix) << QStringLiteral("Fp1,Fp2,Cz") << skip;
        QTest::addRow("empty fields%s", suffix) << QStringLiteral(",Fp1,,Cz,") << skip;
        QTest::addRow("only separators%s", suffix) << QStringLiteral(",,") << skip;
    }
}

//=============================================================================================================

void TestUtilsStringUtils::splitMatchesQString()
{
    QFETCH(QString, text);
    QFETCH(bool, skipEmpty);
    const QStringList expected = text.split(QLatin1Char(','), skipEmpty ? Qt::SkipEmptyParts : Qt::KeepEmptyParts);
    const StringList actual = split(text.toStdString(), ',', skipEmpty);
    QCOMPARE(static_cast<qsizetype>(actual.size()), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        QCOMPARE(QString::fromStdString(actual[i]), expected[static_cast<qsizetype>(i)]);
    }
}

//=============================================================================================================

void TestUtilsStringUtils::splitWhitespaceMatchesQString()
{
    for (const char* text : {"", "   ", "MEG0113", "  9.000  0.07303\t-0.01983 \r\n", "a\v\fb\n\nc"}) {
        const QStringList expected = QString::fromLatin1(text).split(QRegularExpression(QStringLiteral("\\s+")),
                                                                     Qt::SkipEmptyParts);
        const std::vector<std::string_view> actual = split_whitespace(text);
        QCOMPARE(static_cast<qsizetype>(actual.size()), expected.size());
        for (std::size_t i = 0; i < actual.size(); ++i) {
            QCOMPARE(QString::fromLatin1(actual[i].data(), static_cast<qsizetype>(actual[i].size())),
                     expected[static_cast<qsizetype>(i)]);
        }
    }
}

//=============================================================================================================

void TestUtilsStringUtils::nameListHelpersMatchQStringList()
{
    const QStringList qnames{"MEG 0113", "EEG 001", "STI 014", "EEG 001"};
    const StringList names{"MEG 0113", "EEG 001", "STI 014", "EEG 001"};
    for (const char* name : {"MEG 0113", "EEG 001", "STI 014", "eeg 001", ""}) {
        QCOMPARE(index_of(names, name), static_cast<int>(qnames.indexOf(QString::fromUtf8(name))));
        QCOMPARE(std::ranges::contains(names, name), qnames.contains(QString::fromUtf8(name)));
    }
    QCOMPARE(QString::fromStdString(join(names, ", ")), qnames.join(", "));
    QCOMPARE(join({}, ","), std::string());
    QCOMPARE(join({"Cz"}, ","), std::string("Cz"));
}

//=============================================================================================================

void TestUtilsStringUtils::caseHelpersMatchQString()
{
    const QString mixed = QStringLiteral("Brain-Vision_V1.0 eeg");
    QCOMPARE(QString::fromStdString(to_lower(mixed.toStdString())), mixed.toLower());
    QVERIFY(iequals("NIFTI", "nifti"));
    QVERIFY(iequals("", ""));
    QVERIFY(!iequals("nii", "nii.gz"));
    QVERIFY(!iequals("abc", "abd"));
    QCOMPARE(iequals("Fp1", "fP1"), QString::compare(QStringLiteral("Fp1"), QStringLiteral("fP1"), Qt::CaseInsensitive) == 0);
    // Bytes outside ASCII are compared as they are, so UTF-8 text never changes length.
    QCOMPARE(to_lower("\xc3\x84"), std::string("\xc3\x84"));
}

//=============================================================================================================

void TestUtilsStringUtils::toDoubleMatchesQString_data()
{
    QTest::addColumn<QString>("text");
    for (const char* text : {"0", "-1.5", "1e-3", "  2.25 ", "+7", ".5", "1.", "3.4e38", "1e400", "nan", "inf", "-inf",
                             "", " ", "abc", "1.5x", "1,5", "--1", "0x10", "1e-320", "4.9e-324", "1e-400", "1.7976931348623157e308",
                             "2.2250738585072014e-308", "-0", "1e", "e5", "+-1"}) {
        QTest::newRow(text[0] ? text : "(empty)") << QString::fromLatin1(text);
    }
}

//=============================================================================================================

void TestUtilsStringUtils::toDoubleMatchesQString()
{
    QFETCH(QString, text);
    bool ok = false;
    const double expected = text.toDouble(&ok);
    const std::optional<double> actual = to_double(text.toStdString());
    QCOMPARE(actual.has_value(), ok);
    if (ok) {
        if (std::isnan(expected)) {
            QVERIFY(std::isnan(*actual));
        } else {
            QCOMPARE(*actual, expected);
        }
    }
}

//=============================================================================================================

void TestUtilsStringUtils::toIntMatchesQString_data()
{
    QTest::addColumn<QString>("text");
    for (const char* text : {"0", "-42", " 17 ", "+5", "2147483647", "-2147483648", "2147483648", "", "1.0", "12a", "0x1F"}) {
        QTest::newRow(text[0] ? text : "(empty)") << QString::fromLatin1(text);
    }
}

//=============================================================================================================

void TestUtilsStringUtils::toIntMatchesQString()
{
    QFETCH(QString, text);
    bool ok = false;
    const int expected = text.toInt(&ok);
    const std::optional<int> actual = to_int(text.toStdString());
    QCOMPARE(actual.has_value(), ok);
    if (ok) {
        QCOMPARE(*actual, expected);
    }
}

//=============================================================================================================

void TestUtilsStringUtils::toDoubleIgnoresTheLocale()
{
    // QCoreApplication adopts the environment's locale, where the decimal separator may be a comma.
    const std::string previous = std::setlocale(LC_ALL, nullptr);
    bool german = false;
    for (const char* name : {"de_DE.UTF-8", "de_DE.utf8", "German_Germany.1252"}) {
        german = german || std::setlocale(LC_ALL, name) != nullptr;
    }
    const std::optional<double> point = to_double("1.5");
    const std::optional<double> comma = to_double("1,5");
    std::setlocale(LC_ALL, previous.c_str());
    if (!german) {
        QSKIP("No German locale installed");
    }
    QCOMPARE(point, std::optional<double>(1.5));
    QVERIFY(!comma);
}

//=============================================================================================================

void TestUtilsStringUtils::utf8PathsRoundTrip()
{
    // On Windows std::filesystem::path(std::string) assumes the ANSI code page, not UTF-8.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString name = QStringLiteral("sub-\u00e4\u00f6_\u65e5\u672c.fif");
    const std::string utf8 = QDir(dir.path()).filePath(name).toStdString();

    const std::filesystem::path path = path_from_utf8(utf8);
    QCOMPARE(utf8_from_path(path), utf8);
    {
        std::ofstream out(path, std::ios::binary);
        out << "x";
    }
    QVERIFY(QFile::exists(QDir(dir.path()).filePath(name)));
    QCOMPARE(utf8_from_path(path.filename()), name.toStdString());
    QCOMPARE(utf8_from_path(path.extension()), std::string(".fif"));
}

//=============================================================================================================

void TestUtilsStringUtils::latin1AndUtf8MatchQString()
{
    std::string every;
    for (int byte = 0; byte < 256; ++byte) {
        every.push_back(static_cast<char>(byte));
    }
    QCOMPARE(latin1_to_utf8(every), QString::fromLatin1(every.data(), qsizetype(every.size())).toStdString());
    QCOMPARE(latin1_to_utf8("\xB5V"), std::string("\xC2\xB5V"));

    QVERIFY(is_utf8(""));
    QVERIFY(is_utf8("plain ASCII"));
    QVERIFY(is_utf8("\xC2\xB5V \xE6\x97\xA5 \xF0\x9F\x98\x80"));
    QVERIFY(!is_utf8("\xB5V"));            // Latin-1 micro sign
    QVERIFY(!is_utf8("\xC2"));             // cut short
    QVERIFY(!is_utf8("\xC0\xAF"));         // overlong
    QVERIFY(!is_utf8("\xED\xA0\x80"));     // surrogate
    QVERIFY(!is_utf8("\xF4\x90\x80\x80")); // above U+10FFFF
}

//=============================================================================================================

void TestUtilsStringUtils::toHexMatchesQByteArray()
{
    std::vector<std::byte> every;
    every.reserve(256);
    for (int byte = 0; byte < 256; ++byte) {
        every.push_back(static_cast<std::byte>(byte));
    }
    const QByteArray qt(reinterpret_cast<const char*>(every.data()), qsizetype(every.size()));
    QCOMPARE(to_hex(every), qt.toHex().toStdString());
    QCOMPARE(to_hex({}), std::string());
}

//=============================================================================================================

void TestUtilsStringUtils::base64MatchesQByteArray()
{
    std::vector<std::byte> data;
    data.reserve(300);
    for (std::size_t n = 0; n <= 300; ++n) {
        const QByteArray qt(reinterpret_cast<const char*>(data.data()), qsizetype(data.size()));
        const std::string standard = to_base64(data);
        const std::string url = to_base64(data, true);
        QCOMPARE(standard, qt.toBase64().toStdString());
        QCOMPARE(url, qt.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals).toStdString());
        QCOMPARE(from_base64(standard), std::optional(data));
        QCOMPARE(from_base64(url), std::optional(data));
        data.push_back(static_cast<std::byte>((n * 151) + (n >> 2)));
    }
    QCOMPARE(from_base64("QU JD\nRA=="), std::optional(std::vector<std::byte>{std::byte{'A'}, std::byte{'B'}, std::byte{'C'}, std::byte{'D'}}));
    QVERIFY(!from_base64("QUJD*"));
    QVERIFY(!from_base64("QUJDR"));
    QVERIFY(!from_base64("QQ==QQ=="));
}

//=============================================================================================================

void TestUtilsStringUtils::iso8601MatchesQDateTime_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("utc") << "2026-11-15T09:30:05Z";
    QTest::newRow("milliseconds") << "2026-11-15T09:30:05.250Z";
    QTest::newRow("microseconds") << "2026-11-15T09:30:05.250999Z";
    QTest::newRow("offset") << "2026-11-15T09:30:05+02:00";
    QTest::newRow("negative offset") << "1999-12-31T23:59:59.5-05:30";
    QTest::newRow("compact offset") << "2026-03-01T00:00:00+0100";
    QTest::newRow("no seconds") << "2026-11-15T09:30Z";
    QTest::newRow("leap day") << "2024-02-29T12:00:00Z";
    QTest::newRow("epoch") << "1970-01-01T00:00:00Z";
    QTest::newRow("date only") << "2026-11-15";
}

//=============================================================================================================

void TestUtilsStringUtils::iso8601MatchesQDateTime()
{
    QFETCH(QString, text);
    QDateTime qt = QDateTime::fromString(text, Qt::ISODateWithMs);
    QVERIFY(qt.isValid());
    if (qt.timeSpec() == Qt::LocalTime) {
        qt.setTimeZone(QTimeZone::UTC); // text without an offset is UTC here, local time in Qt
    }
    const std::optional<UtcTime> ours = parse_iso8601(text.toStdString());
    QVERIFY(ours);
    QCOMPARE(ours->time_since_epoch().count(), qt.toMSecsSinceEpoch());
    QCOMPARE(format_iso8601(*ours), qt.toUTC().toString(Qt::ISODate).toStdString());
    QCOMPARE(format_iso8601(*ours, true), qt.toUTC().toString(Qt::ISODateWithMs).toStdString());
    QCOMPARE(parse_iso8601(format_iso8601(*ours, true)), ours);
}

//=============================================================================================================

void TestUtilsStringUtils::iso8601RejectsMalformedText()
{
    for (const std::string_view text : {"", "2026-11", "2026-13-01T00:00:00Z", "2026-02-30T00:00:00Z", "2026-11-15T24:00:00Z",
                                        "2026-11-15T09:60:00Z", "2026-11-15T09:30:00X", "2026-11-15T09:30:00Z trailing", "26-11-15T09:30Z",
                                        "2026-11-15T09:30:00+25:00", "2026-11-15T09:30:00."}) {
        QVERIFY2(!parse_iso8601(text), std::string(text).c_str());
    }
    QCOMPARE(parse_iso8601("2026-11-15 09:30:00"), parse_iso8601("2026-11-15T09:30:00Z"));
}

//=============================================================================================================

void TestUtilsStringUtils::executableDirIsTheApplicationDir()
{
    const std::filesystem::path expected = path_from_utf8(QCoreApplication::applicationDirPath().toStdString());
    QCOMPARE(executable_dir(), std::filesystem::canonical(expected));
}

//=============================================================================================================

void TestUtilsStringUtils::resourcePathPrefersTheInstalledResources()
{
    const std::filesystem::path installed = executable_dir().parent_path() / "resources" / "general" / "coilDefinitions" / "coil_def.dat";
    if (!std::filesystem::exists(installed)) {
        QSKIP("No resources next to the test binary");
    }
    QCOMPARE(resource_path("general/coilDefinitions/coil_def.dat"), installed);
    // Without installed resources the path is relative to the working directory, as before.
    QCOMPARE(resource_path("general/no_such_file.dat"), std::filesystem::path("../resources/general/no_such_file.dat"));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestUtilsStringUtils)
#include "test_base_string_utils.moc"
