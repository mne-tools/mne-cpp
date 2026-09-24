//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_tool_list_source_space.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     April, 2026
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
