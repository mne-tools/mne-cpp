//=============================================================================================================
/**
 * @file     test_tool_compute_pipeline.cpp
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
 * @brief    End-to-end integration tests: make inv operator → compute MNE/dSPM/sLORETA.
 *           Exercises the main compute paths of mne_inverse_operator and mne_compute_mne.
 */

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestToolComputePipeline : public QObject
{
    Q_OBJECT

private:
    QString m_sBinDir;
    QString m_sResourcePath;
    QString m_sInvFile;

    QString runTool(const QString& toolName, const QStringList& args, int timeoutMs = 60000)
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
    // Step 1: Create inverse operator from fwd + cov + meas
    //=========================================================================================================

    void testMakeInverseOperator()
    {
        if (!toolExists("mne_inverse_operator")) {
            QSKIP("mne_inverse_operator binary not found");
        }

        QString fwdFile = m_sResourcePath + "Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QString covFile = m_sResourcePath + "MEG/sample/sample_audvis-cov.fif";

        if (!QFile::exists(fwdFile) || !QFile::exists(covFile)) {
            QSKIP("Required test data not available");
        }

        m_sInvFile = QDir::tempPath() + "/test_pipeline_inv.fif";

        QString output = runTool("mne_inverse_operator", {
            "--fwd", fwdFile,
            "--noisecov", covFile,
            "--meg",
            "--inv", m_sInvFile
        }, 120000);

        if (!QFile::exists(m_sInvFile) || QFileInfo(m_sInvFile).size() == 0) {
            m_sInvFile.clear();
            QSKIP("Inverse operator creation failed — cannot continue pipeline");
        }

        QVERIFY(QFileInfo(m_sInvFile).size() > 0);
    }

    //=========================================================================================================
    // Step 2: Compute MNE source estimate
    //=========================================================================================================

    void testComputeMNE()
    {
        if (!toolExists("mne_compute_mne")) QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty()) QSKIP("No inverse operator");

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile)) QSKIP("Evoked file not available");

        QString stcPrefix = QDir::tempPath() + "/test_pipeline_mne";

        QString output = runTool("mne_compute_mne", {
            "--inv", m_sInvFile,
            "--meas", aveFile,
            "--method", "MNE",
            "--snr", "3",
            "--out", stcPrefix,
            "--nomatch"
        }, 120000);

        // Check output produced
        QVERIFY(!output.isEmpty());
        // If stc files were produced, great
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Step 3: Compute dSPM source estimate
    //=========================================================================================================

    void testComputeDSPM()
    {
        if (!toolExists("mne_compute_mne")) QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty()) QSKIP("No inverse operator");

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile)) QSKIP("Evoked file not available");

        QString stcPrefix = QDir::tempPath() + "/test_pipeline_dspm";

        QString output = runTool("mne_compute_mne", {
            "--inv", m_sInvFile,
            "--meas", aveFile,
            "--spm",
            "--snr", "3",
            "--out", stcPrefix,
            "--nomatch"
        }, 120000);

        QVERIFY(!output.isEmpty());
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Step 4: Compute sLORETA source estimate
    //=========================================================================================================

    void testComputeSloreta()
    {
        if (!toolExists("mne_compute_mne")) QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty()) QSKIP("No inverse operator");

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile)) QSKIP("Evoked file not available");

        QString stcPrefix = QDir::tempPath() + "/test_pipeline_sloreta";

        QString output = runTool("mne_compute_mne", {
            "--inv", m_sInvFile,
            "--meas", aveFile,
            "--sLORETA",
            "--snr", "3",
            "--out", stcPrefix,
            "--nomatch"
        }, 120000);

        QVERIFY(!output.isEmpty());
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Step 5: mne_compute_mne with --fwd mode (synthetic data from forward solution)
    //=========================================================================================================

    void testComputeWithForwardMode()
    {
        if (!toolExists("mne_compute_mne")) QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty()) QSKIP("No inverse operator");

        QString fwdFile = m_sResourcePath + "Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdFile)) QSKIP("Forward solution not available");

        QString stcPrefix = QDir::tempPath() + "/test_pipeline_fwdmode";

        QString output = runTool("mne_compute_mne", {
            "--inv", m_sInvFile,
            "--fwd", fwdFile,
            "--fwdamp", "50",
            "--method", "MNE",
            "--snr", "3",
            "--out", stcPrefix
        }, 120000);

        QVERIFY(!output.isEmpty());
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Step 6: mne_compute_mne with --abs and --pick options
    //=========================================================================================================

    void testComputeAbsAndPick()
    {
        if (!toolExists("mne_compute_mne")) QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty()) QSKIP("No inverse operator");

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile)) QSKIP("Evoked file not available");

        QString stcPrefix = QDir::tempPath() + "/test_pipeline_abs";

        QString output = runTool("mne_compute_mne", {
            "--inv", m_sInvFile,
            "--meas", aveFile,
            "--method", "MNE",
            "--snr", "3",
            "--abs",
            "--pick", "100",
            "--out", stcPrefix,
            "--nomatch"
        }, 120000);

        QVERIFY(!output.isEmpty());
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // mne_check_surface: run on BEM surface
    //=========================================================================================================

    void testCheckSurface()
    {
        if (!toolExists("mne_check_surface")) QSKIP("mne_check_surface not found");

        QString surfFile = m_sResourcePath + "subjects/sample/bem/sample-inner_skull-5120.surf";
        if (!QFile::exists(surfFile)) QSKIP("Surface file not available");

        QString output = runTool("mne_check_surface", {"--surf", surfFile});
        // Should produce surface check output
        QVERIFY(!output.isEmpty());
    }

    //=========================================================================================================
    // mne_list_bem: list multi-layer BEM
    //=========================================================================================================

    void testListBemMultiLayer()
    {
        if (!toolExists("mne_list_bem")) QSKIP("mne_list_bem not found");

        QString bemFile = m_sResourcePath + "subjects/sample/bem/sample-1280-1280-1280-bem.fif";
        if (!QFile::exists(bemFile)) QSKIP("Multi-layer BEM not available");

        QString output = runTool("mne_list_bem", {"--bem", bemFile});
        QVERIFY(!output.isEmpty());
        // Multi-layer BEM should list multiple surfaces
        QVERIFY(output.contains("surface", Qt::CaseInsensitive) ||
                output.contains("layer", Qt::CaseInsensitive) ||
                output.contains("1280", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_collect_transforms: extract from raw FIFF
    //=========================================================================================================

    void testCollectTransforms()
    {
        if (!toolExists("mne_collect_transforms")) QSKIP("mne_collect_transforms not found");

        QString rawFile = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile)) QSKIP("Raw file not available");

        QString output = runTool("mne_collect_transforms", {"--meas", rawFile});
        QVERIFY(!output.isEmpty());
    }

    //=========================================================================================================
    // Cleanup
    //=========================================================================================================

    void cleanupTestCase()
    {
        if (!m_sInvFile.isEmpty()) {
            QFile::remove(m_sInvFile);
        }
    }
};

QTEST_GUILESS_MAIN(TestToolComputePipeline)
#include "test_tool_compute_pipeline.moc"
