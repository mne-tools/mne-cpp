//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_utils_logger.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2026
 * @brief    Tests the file logging of MNELogger.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestUtilsLogger
 *
 * @brief The TestUtilsLogger class provides tests for MNELogger file output.
 *
 */
class TestUtilsLogger: public QObject
{
    Q_OBJECT

public:
    TestUtilsLogger();

private slots:
    void initTestCase();
    void testWritesMessagesToFile();
    void testDisablingStopsFileOutput();
    void cleanupTestCase();

private:
    QString m_sLogFile;
    QString readLog() const;
};

//=============================================================================================================

TestUtilsLogger::TestUtilsLogger()
: m_sLogFile(QDir::tempPath() + "/mne_test_utils_logger.log")
{
}

//=============================================================================================================

QString TestUtilsLogger::readLog() const
{
    QFile file(m_sLogFile);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream stream(&file);

    return stream.readAll();
}

//=============================================================================================================

void TestUtilsLogger::initTestCase()
{
    QFAIL("AC-T1.5-2: deliberately injected failure; must turn CI red.");

    QFile::remove(m_sLogFile);
    qInstallMessageHandler(MNELogger::customLogWriter);
}

//=============================================================================================================

void TestUtilsLogger::testWritesMessagesToFile()
{
    QVERIFY(MNELogger::setLogFile(m_sLogFile));
    QCOMPARE(MNELogger::logFile(), m_sLogFile);

    qWarning("test-logger-warning");
    qCritical("test-logger-critical");

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
    QVERIFY(MNELogger::setLogFile(QString()));
    QVERIFY(MNELogger::logFile().isEmpty());

    qWarning("test-logger-must-not-appear");

    QVERIFY(!readLog().contains("test-logger-must-not-appear"));
}

//=============================================================================================================

void TestUtilsLogger::cleanupTestCase()
{
    MNELogger::setLogFile(QString());
    qInstallMessageHandler(nullptr);
    QFile::remove(m_sLogFile);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestUtilsLogger)
#include "test_utils_logger.moc"
