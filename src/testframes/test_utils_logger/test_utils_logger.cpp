//=============================================================================================================
/**
 * @file     test_utils_logger.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Tests the file logging of MNELogger.
 *
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
