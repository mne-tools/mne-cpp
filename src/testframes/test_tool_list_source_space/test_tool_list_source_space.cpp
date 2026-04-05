//=============================================================================================================
/**
 * @file     test_tool_list_source_space.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     April, 2026
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
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Integration tests for the mne_list_source_space CLI tool.
 */

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QCoreApplication>

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestToolListSourceSpace : public QObject
{
    Q_OBJECT

private:
    QString m_sToolPath;
    QString m_sResourcePath;

    QString runTool(const QStringList& args, int timeoutMs = 10000)
    {
        QProcess proc;
        proc.setProgram(m_sToolPath);
        proc.setArguments(args);
        proc.start();
        proc.waitForFinished(timeoutMs);
        return proc.readAllStandardOutput() + proc.readAllStandardError();
    }

private slots:

    void initTestCase()
    {
        QString binDir = QCoreApplication::applicationDirPath();
        m_sToolPath = binDir + "/../bin/mne_list_source_space";
#ifdef Q_OS_WIN
        m_sToolPath += ".exe";
#endif
        m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";

        if (!QFile::exists(m_sToolPath)) {
            QSKIP("mne_list_source_space binary not found — build tools first");
        }
    }

    //=========================================================================================================
    // Test: running with --help shows usage
    //=========================================================================================================

    void testHelp()
    {
        QString output = runTool({"--help"});
        QVERIFY(output.contains("source", Qt::CaseInsensitive) ||
                output.contains("help", Qt::CaseInsensitive) ||
                output.contains("usage", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: list source space from oct-6 file
    // Reference: mne-python's mne.read_source_spaces reports 2 hemispheres,
    // ~4098 vertices per hemisphere for oct-6.
    //=========================================================================================================

    void testListOct6()
    {
        QString srcFile = m_sResourcePath + "subjects/sample/bem/sample-oct-6-src.fif";
        if (!QFile::exists(srcFile)) {
            QSKIP("sample-oct-6-src.fif not available");
        }

        QString output = runTool({"--src", srcFile});

        // Should report source space information
        QVERIFY(!output.isEmpty());

        // Should mention vertices or source points
        QVERIFY(output.contains("vert", Qt::CaseInsensitive) ||
                output.contains("source", Qt::CaseInsensitive) ||
                output.contains("point", Qt::CaseInsensitive) ||
                output.contains("4098", Qt::CaseInsensitive));
    }
};

QTEST_GUILESS_MAIN(TestToolListSourceSpace)
#include "test_tool_list_source_space.moc"
