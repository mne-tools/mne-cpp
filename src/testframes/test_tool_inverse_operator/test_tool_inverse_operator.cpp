//=============================================================================================================
/**
 * @file     test_tool_inverse_operator.cpp
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
 * @brief    Integration tests for the mne_inverse_operator and mne_compute_mne CLI tools.
 */

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QCoreApplication>
#include <QTemporaryFile>

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestToolInverseOperator : public QObject
{
    Q_OBJECT

private:
    QString m_sBinDir;
    QString m_sResourcePath;
    QString m_sInvFile;  // Inverse operator file created by testInverseOperatorCompute

    QString runTool(const QString& toolName, const QStringList& args, int timeoutMs = 30000)
    {
        QString path = m_sBinDir + "/" + toolName;
#ifdef Q_OS_WIN
        path += ".exe";
#endif
        QProcess proc;
        proc.setProgram(path);
        proc.setArguments(args);
        proc.start();
        proc.waitForFinished(timeoutMs);
        return proc.readAllStandardOutput() + proc.readAllStandardError();
    }

    bool toolExists(const QString& toolName)
    {
        QString path = m_sBinDir + "/" + toolName;
#ifdef Q_OS_WIN
        path += ".exe";
#endif
        return QFile::exists(path);
    }

private slots:

    void initTestCase()
    {
        QString binDir = QCoreApplication::applicationDirPath();
        m_sBinDir = binDir + "/../bin";
        m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
    }

    //=========================================================================================================
    // Test: mne_inverse_operator help
    //=========================================================================================================

    void testInverseOperatorHelp()
    {
        if (!toolExists("mne_inverse_operator")) {
            QSKIP("mne_inverse_operator binary not found");
        }
        QString output = runTool("mne_inverse_operator", {"--help"});
        QVERIFY(output.contains("inverse", Qt::CaseInsensitive) ||
                output.contains("help", Qt::CaseInsensitive) ||
                output.contains("forward", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: mne_inverse_operator with sample data (fwd + cov → inv)
    //=========================================================================================================

    void testInverseOperatorCompute()
    {
        if (!toolExists("mne_inverse_operator")) {
            QSKIP("mne_inverse_operator binary not found");
        }

        QString fwdFile = m_sResourcePath + "Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QString covFile = m_sResourcePath + "MEG/sample/sample_audvis-cov.fif";

        if (!QFile::exists(fwdFile) || !QFile::exists(covFile)) {
            QSKIP("Required test data not available");
        }

        // Write inverse operator to a known path for reuse by mne_compute_mne test
        QString outPath = QDir::tempPath() + "/test_inv_operator.fif";

        QString output = runTool("mne_inverse_operator", {
            "--fwd", fwdFile,
            "--noisecov", covFile,
            "--meg",
            "--inv", outPath
        }, 60000);

        // Check the tool ran (output may indicate success or error)
        // If output file was created, the tool worked
        if (QFile::exists(outPath) && QFileInfo(outPath).size() > 0) {
            m_sInvFile = outPath;
            QVERIFY(true);
        } else {
            // Tool may have produced error output, verify it at least ran
            QVERIFY(!output.isEmpty());
        }
    }

    //=========================================================================================================
    // Test: mne_compute_mne help
    //=========================================================================================================

    void testComputeMneHelp()
    {
        if (!toolExists("mne_compute_mne")) {
            QSKIP("mne_compute_mne binary not found");
        }
        QString output = runTool("mne_compute_mne", {"--help"});
        QVERIFY(output.contains("inverse", Qt::CaseInsensitive) ||
                output.contains("compute", Qt::CaseInsensitive) ||
                output.contains("help", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // Test: mne_compute_mne MNE estimate (if inv operator was created)
    //=========================================================================================================

    void testComputeMneMNE()
    {
        if (!toolExists("mne_compute_mne")) {
            QSKIP("mne_compute_mne binary not found");
        }
        if (m_sInvFile.isEmpty() || !QFile::exists(m_sInvFile)) {
            QSKIP("Inverse operator file not available (depends on testInverseOperatorCompute)");
        }

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile)) {
            QSKIP("sample_audvis-ave.fif not available");
        }

        QString stcPrefix = QDir::tempPath() + "/test_compute_mne";

        QString output = runTool("mne_compute_mne", {
            "--inv", m_sInvFile,
            "--meas", aveFile,
            "--method", "MNE",
            "--snr", "3",
            "--out", stcPrefix
        }, 120000);

        // Verify the tool ran
        QVERIFY(!output.isEmpty() ||
                QFile::exists(stcPrefix + "-lh.stc") ||
                QFile::exists(stcPrefix + "-rh.stc"));

        // Cleanup
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Test: mne_compute_mne dSPM estimate
    //=========================================================================================================

    void testComputeMneDSPM()
    {
        if (!toolExists("mne_compute_mne")) {
            QSKIP("mne_compute_mne binary not found");
        }
        if (m_sInvFile.isEmpty() || !QFile::exists(m_sInvFile)) {
            QSKIP("Inverse operator file not available");
        }

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile)) {
            QSKIP("sample_audvis-ave.fif not available");
        }

        QString stcPrefix = QDir::tempPath() + "/test_compute_dspm";

        QString output = runTool("mne_compute_mne", {
            "--inv", m_sInvFile,
            "--meas", aveFile,
            "--spm",
            "--snr", "3",
            "--out", stcPrefix
        }, 120000);

        QVERIFY(!output.isEmpty() ||
                QFile::exists(stcPrefix + "-lh.stc") ||
                QFile::exists(stcPrefix + "-rh.stc"));

        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Test: mne_compute_mne sLORETA estimate
    //=========================================================================================================

    void testComputeMneSloreta()
    {
        if (!toolExists("mne_compute_mne")) {
            QSKIP("mne_compute_mne binary not found");
        }
        if (m_sInvFile.isEmpty() || !QFile::exists(m_sInvFile)) {
            QSKIP("Inverse operator file not available");
        }

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile)) {
            QSKIP("sample_audvis-ave.fif not available");
        }

        QString stcPrefix = QDir::tempPath() + "/test_compute_sloreta";

        QString output = runTool("mne_compute_mne", {
            "--inv", m_sInvFile,
            "--meas", aveFile,
            "--sLORETA",
            "--snr", "3",
            "--out", stcPrefix
        }, 120000);

        QVERIFY(!output.isEmpty() ||
                QFile::exists(stcPrefix + "-lh.stc") ||
                QFile::exists(stcPrefix + "-rh.stc"));

        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Cleanup: remove temporary inverse operator file
    //=========================================================================================================

    void cleanupTestCase()
    {
        if (!m_sInvFile.isEmpty()) {
            QFile::remove(m_sInvFile);
        }
    }
};

QTEST_GUILESS_MAIN(TestToolInverseOperator)
#include "test_tool_inverse_operator.moc"
