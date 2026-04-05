//=============================================================================================================
/**
 * @file     test_tool_show_fiff.cpp
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
 * @brief    Integration tests for the mne_show_fiff CLI tool.
 *           Runs the tool binary against sample FIFF files and validates
 *           that expected FIFF block/tag names appear in the output.
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

class TestToolShowFiff : public QObject
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
        // Tests live in out/<config>/tests/, tools in out/<config>/bin/
        m_sToolPath = binDir + "/../bin/mne_show_fiff";
#ifdef Q_OS_WIN
        m_sToolPath += ".exe";
#endif
        m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";

        if (!QFile::exists(m_sToolPath)) {
            QSKIP("mne_show_fiff binary not found — build tools first");
        }
    }

    //=========================================================================================================
    // Test: running with no arguments prints usage/help (exit code != 0 is fine)
    //=========================================================================================================

    void testNoArgs()
    {
        QProcess proc;
        proc.setProgram(m_sToolPath);
        proc.start();
        proc.waitForFinished(5000);
        QString output = proc.readAllStandardOutput() + proc.readAllStandardError();

        // Should mention usage or the tool name
        QVERIFY(output.contains("show_fiff", Qt::CaseInsensitive) ||
                output.contains("usage", Qt::CaseInsensitive) ||
                output.contains("file", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: show_fiff on a raw FIFF file should list measurement blocks
    //=========================================================================================================

    void testShowRawFiff()
    {
        QString rawFile = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile)) {
            QSKIP("sample_audvis_trunc_raw.fif not available");
        }

        QString output = runTool({rawFile});

        // Output should contain FIFF block names
        QVERIFY(output.contains("meas", Qt::CaseInsensitive) ||
                output.contains("FIFF_BLOCK", Qt::CaseInsensitive) ||
                output.contains("raw", Qt::CaseInsensitive));

        // Should mention channel info or nchan
        QVERIFY(output.contains("ch_info", Qt::CaseInsensitive) ||
                output.contains("nchan", Qt::CaseInsensitive) ||
                output.contains("FIFF_NCHAN", Qt::CaseInsensitive) ||
                output.contains("MEG", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: show_fiff on covariance file
    //=========================================================================================================

    void testShowCovFiff()
    {
        QString covFile = m_sResourcePath + "MEG/sample/sample_audvis-cov.fif";
        if (!QFile::exists(covFile)) {
            QSKIP("sample_audvis-cov.fif not available");
        }

        QString output = runTool({covFile});
        // Should contain covariance-related tags
        QVERIFY(output.contains("cov", Qt::CaseInsensitive) ||
                output.contains("noise", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: show_fiff on BEM file
    //=========================================================================================================

    void testShowBemFiff()
    {
        QString bemFile = m_sResourcePath + "subjects/sample/bem/sample-5120-bem.fif";
        if (!QFile::exists(bemFile)) {
            QSKIP("sample-5120-bem.fif not available");
        }

        QString output = runTool({bemFile});
        // Should contain BEM-related tags
        QVERIFY(output.contains("bem", Qt::CaseInsensitive) ||
                output.contains("surf", Qt::CaseInsensitive) ||
                output.contains("surface", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: show_fiff on source space file
    //=========================================================================================================

    void testShowSourceSpaceFiff()
    {
        QString srcFile = m_sResourcePath + "subjects/sample/bem/sample-oct-6-src.fif";
        if (!QFile::exists(srcFile)) {
            QSKIP("sample-oct-6-src.fif not available");
        }

        QString output = runTool({srcFile});
        QVERIFY(output.contains("source", Qt::CaseInsensitive) ||
                output.contains("src", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: --blocks mode shows tree structure
    //=========================================================================================================

    void testBlocksMode()
    {
        QString rawFile = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile)) {
            QSKIP("sample_audvis_trunc_raw.fif not available");
        }

        QString output = runTool({"--in", rawFile, "--blocks"});
        // Block mode should show tree-like block structure
        QVERIFY(!output.isEmpty());
        // Should contain measurement block indicators
        QVERIFY(output.contains("meas", Qt::CaseInsensitive) ||
                output.contains("block", Qt::CaseInsensitive) ||
                output.contains("root", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: --verbose mode produces more output than terse
    //=========================================================================================================

    void testVerboseMode()
    {
        QString rawFile = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile)) {
            QSKIP("sample_audvis_trunc_raw.fif not available");
        }

        QString terseOutput = runTool({"--in", rawFile});
        QString verboseOutput = runTool({"--in", rawFile, "--verbose"});

        // Verbose should produce at least as much output
        QVERIFY(verboseOutput.length() >= terseOutput.length());
    }

    //=========================================================================================================
    // Test: --tag mode to show specific tag numbers (exercises mne_fiff_exp_set lookup)
    //=========================================================================================================

    void testTagMode()
    {
        QString rawFile = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile)) {
            QSKIP("sample_audvis_trunc_raw.fif not available");
        }

        // Tag 100 = FIFF_BLOCK_START, Tag 200 = FIFF_NCHAN — common tags
        QString output = runTool({"--in", rawFile, "--tag", "100", "--tag", "200"});
        // Should produce some filtered output
        QVERIFY(!output.isEmpty());
    }

    //=========================================================================================================
    // Test: show_fiff on forward solution file
    //=========================================================================================================

    void testShowForwardSolutionFiff()
    {
        QString fwdFile = m_sResourcePath + "Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdFile)) {
            QSKIP("forward solution file not available");
        }

        QString output = runTool({"--in", fwdFile, "--verbose"});
        QVERIFY(!output.isEmpty());
        // Forward solution should contain forward-related tags
        QVERIFY(output.contains("forward", Qt::CaseInsensitive) ||
                output.contains("fwd", Qt::CaseInsensitive) ||
                output.contains("source", Qt::CaseInsensitive) ||
                output.contains("mne", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: show_fiff on averaged data file
    //=========================================================================================================

    void testShowAveragedFiff()
    {
        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile)) {
            QSKIP("sample_audvis-ave.fif not available");
        }

        QString output = runTool({"--in", aveFile, "--verbose"});
        QVERIFY(!output.isEmpty());
        // Averaged data should contain evoked-related info
        QVERIFY(output.contains("evoked", Qt::CaseInsensitive) ||
                output.contains("average", Qt::CaseInsensitive) ||
                output.contains("meas", Qt::CaseInsensitive) ||
                output.contains("nave", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: show_fiff on coordinate transform file
    //=========================================================================================================

    void testShowTransformFiff()
    {
        QString transFile = m_sResourcePath + "Result/icp-trans.fif";
        if (!QFile::exists(transFile)) {
            QSKIP("icp-trans.fif not available");
        }

        QString output = runTool({"--in", transFile, "--verbose"});
        QVERIFY(!output.isEmpty());
        QVERIFY(output.contains("coord", Qt::CaseInsensitive) ||
                output.contains("trans", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: --indent option exercises indentation formatting code
    //=========================================================================================================

    void testIndentOption()
    {
        QString rawFile = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile)) {
            QSKIP("sample_audvis_trunc_raw.fif not available");
        }

        QString output = runTool({"--in", rawFile, "--indent", "4"});
        QVERIFY(!output.isEmpty());
    }

    //=========================================================================================================
    // Test: --long option for full string output
    //=========================================================================================================

    void testLongStrings()
    {
        QString rawFile = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile)) {
            QSKIP("sample_audvis_trunc_raw.fif not available");
        }

        QString output = runTool({"--in", rawFile, "--verbose", "--long"});
        QVERIFY(!output.isEmpty());
    }

    //=========================================================================================================
    // Test: show_fiff on BEM solution file (exercises different FIFF block types)
    //=========================================================================================================

    void testShowBemSolutionFiff()
    {
        QString bemSolFile = m_sResourcePath + "subjects/sample/bem/sample-5120-bem-sol.fif";
        if (!QFile::exists(bemSolFile)) {
            QSKIP("sample-5120-bem-sol.fif not available");
        }

        QString output = runTool({"--in", bemSolFile, "--verbose"});
        QVERIFY(!output.isEmpty());
        // BEM solution should have solver-related info
        QVERIFY(output.contains("bem", Qt::CaseInsensitive) ||
                output.contains("surf", Qt::CaseInsensitive) ||
                output.contains("sol", Qt::CaseInsensitive));
    }
};

QTEST_GUILESS_MAIN(TestToolShowFiff)
#include "test_tool_show_fiff.moc"
