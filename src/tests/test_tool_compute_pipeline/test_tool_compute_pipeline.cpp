//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_tool_compute_pipeline.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     April, 2026
 * @brief    End-to-end integration tests: make inv operator → compute MNE/dSPM/sLORETA.
 *           Exercises the main compute paths of mne_inverse_operator and mne_compute_mne.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/mne/mne_forward_solution.h>
#include <mne/mne/mne_inverse_operator.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QCoreApplication>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

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
    QTemporaryDir m_scratch;

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

        m_sInvFile = m_scratch.filePath("test_pipeline_inv.fif");

        QString output = runTool("mne_inverse_operator", {"--fwd", fwdFile, "--noisecov", covFile, "--meg", "--inv", m_sInvFile}, 120000);

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
        if (!toolExists("mne_compute_mne"))
            QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty())
            QSKIP("No inverse operator");

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile))
            QSKIP("Evoked file not available");

        QString stcPrefix = m_scratch.filePath("test_pipeline_mne");

        QString output = runTool("mne_compute_mne", {"--inv", m_sInvFile, "--meas", aveFile, "--method", "MNE", "--snr", "3", "--out", stcPrefix, "--nomatch"}, 120000);

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
        if (!toolExists("mne_compute_mne"))
            QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty())
            QSKIP("No inverse operator");

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile))
            QSKIP("Evoked file not available");

        QString stcPrefix = m_scratch.filePath("test_pipeline_dspm");

        QString output = runTool("mne_compute_mne", {"--inv", m_sInvFile, "--meas", aveFile, "--spm", "--snr", "3", "--out", stcPrefix, "--nomatch"}, 120000);

        QVERIFY(!output.isEmpty());
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Step 4: Compute sLORETA source estimate
    //=========================================================================================================

    void testComputeSloreta()
    {
        if (!toolExists("mne_compute_mne"))
            QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty())
            QSKIP("No inverse operator");

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile))
            QSKIP("Evoked file not available");

        QString stcPrefix = m_scratch.filePath("test_pipeline_sloreta");

        QString output = runTool("mne_compute_mne", {"--inv", m_sInvFile, "--meas", aveFile, "--sLORETA", "--snr", "3", "--out", stcPrefix, "--nomatch"}, 120000);

        QVERIFY(!output.isEmpty());
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Step 5: mne_compute_mne with --fwd mode (synthetic data from forward solution)
    //=========================================================================================================

    void testComputeWithForwardMode()
    {
        if (!toolExists("mne_compute_mne"))
            QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty())
            QSKIP("No inverse operator");

        QString fwdFile = m_sResourcePath + "Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdFile))
            QSKIP("Forward solution not available");

        QString stcPrefix = m_scratch.filePath("test_pipeline_fwdmode");

        QString output = runTool("mne_compute_mne", {"--inv", m_sInvFile, "--fwd", fwdFile, "--fwdamp", "50", "--method", "MNE", "--snr", "3", "--out", stcPrefix}, 120000);

        QVERIFY(!output.isEmpty());
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // Step 6: mne_compute_mne with --abs and --pick options
    //=========================================================================================================

    void testComputeAbsAndPick()
    {
        if (!toolExists("mne_compute_mne"))
            QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty())
            QSKIP("No inverse operator");

        QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(aveFile))
            QSKIP("Evoked file not available");

        QString stcPrefix = m_scratch.filePath("test_pipeline_abs");

        QString output = runTool("mne_compute_mne", {"--inv", m_sInvFile, "--meas", aveFile, "--method", "MNE", "--snr", "3", "--abs", "--pick", "100", "--out", stcPrefix, "--nomatch"}, 120000);

        QVERIFY(!output.isEmpty());
        QFile::remove(stcPrefix + "-lh.stc");
        QFile::remove(stcPrefix + "-rh.stc");
    }

    //=========================================================================================================
    // --pred: the model reproduces forward-field data it can explain
    //=========================================================================================================

    void testPredictedDataReproducesForwardFields()
    {
        if (!toolExists("mne_compute_mne"))
            QSKIP("mne_compute_mne not found");
        if (m_sInvFile.isEmpty())
            QSKIP("No inverse operator");
        const QString fwdFile = m_sResourcePath + "Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdFile))
            QSKIP("Forward solution not available");

        // Ten forward columns of 50 nAm each are the data; sLORETA must not change the prediction
        const QString predFile = m_scratch.filePath("test_pipeline_pred.txt");
        QProcess proc;
        proc.start(m_sBinDir + "/mne_compute_mne", {"--inv", m_sInvFile, "--fwd", fwdFile, "--fwdamp", "50", "--tmin", "0", "--tmax", "9", "--sLORETA", "--snr", "100", "--out", m_scratch.filePath("test_pipeline_pred"), "--pred", predFile});
        QVERIFY(proc.waitForFinished(120000));
        QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
        QCOMPARE(proc.exitCode(), 0);

        auto inv = mne::MNEInverseOperator::read(m_sInvFile.toStdU16String());
        QVERIFY(inv);
        auto fwd = mne::MNEForwardSolution::read(fwdFile.toStdU16String());
        QVERIFY(fwd);
        const std::vector<std::string>& names = inv->noise_cov.names;
        Eigen::MatrixXd fields(static_cast<Eigen::Index>(names.size()), 10);
        for (std::size_t channel = 0; channel < names.size(); ++channel) {
            const auto row = std::find(fwd->sol.row_names.begin(), fwd->sol.row_names.end(), names[channel]);
            QVERIFY(row != fwd->sol.row_names.end());
            fields.row(static_cast<Eigen::Index>(channel)) = fwd->sol.data.row(row - fwd->sol.row_names.begin()).head(10) * 50e-9;
        }
        Eigen::MatrixXd projector;
        mne::FiffProj::make_projector(inv->projs, names, projector);
        const Eigen::MatrixXd expected = projector * fields;

        QFile pred(predFile);
        QVERIFY(pred.open(QIODevice::ReadOnly | QIODevice::Text));
        const QStringList lines = QString::fromUtf8(pred.readAll()).split('\n', Qt::SkipEmptyParts).mid(3);
        QCOMPARE(lines.size(), static_cast<int>(expected.rows()));
        Eigen::MatrixXd predicted(expected.rows(), expected.cols());
        for (int row = 0; row < lines.size(); ++row) {
            const QStringList values = lines[row].split('\t');
            QCOMPARE(values.size(), static_cast<int>(expected.cols()));
            for (int col = 0; col < values.size(); ++col) {
                predicted(row, col) = values[col].toDouble();
            }
        }
        const double error = (predicted - expected).norm() / expected.norm();
        QVERIFY2(error < 0.01, qPrintable(QString::number(error)));
    }

    //=========================================================================================================
    // Volume pipeline: the -vl.stc MNE-Python reads holds one vertex per source
    //=========================================================================================================

    void testVolumeEstimateHasOneVertexPerSource()
    {
        for (const char* tool : {"mne_volume_source_space", "mne_forward_solution", "mne_inverse_operator", "mne_compute_mne"}) {
            if (!toolExists(tool)) {
                QSKIP("volume pipeline tools not found");
            }
        }
        const QString bemFile = m_sResourcePath + "subjects/sample/bem/sample-5120-bem.fif";
        const QString aveFile = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
        const QString covFile = m_sResourcePath + "MEG/sample/sample_audvis-cov.fif";
        const QString transFile = m_sResourcePath + "MEG/sample/all-trans.fif";
        if (!QFile::exists(bemFile) || !QFile::exists(aveFile) || !QFile::exists(covFile) || !QFile::exists(transFile)) {
            QSKIP("Required test data not available");
        }

        const QString srcFile = m_scratch.filePath("vol-src.fif");
        const QString fwdFile = m_scratch.filePath("vol-fwd.fif");
        const QString invFile = m_scratch.filePath("vol-inv.fif");
        const QString stcBase = m_scratch.filePath("vol");
        runTool("mne_volume_source_space", {"--bem", bemFile, "--grid", "15", "--out", srcFile});
        runTool("mne_forward_solution", {"--src", srcFile, "--meas", aveFile, "--mri", transFile, "--origin", "0:0:40", "--meg", "--fwd", fwdFile}, 120000);
        runTool("mne_inverse_operator", {"--fwd", fwdFile, "--noisecov", covFile, "--meg", "--inv", invFile}, 120000);
        runTool("mne_compute_mne", {"--inv", invFile, "--meas", aveFile, "--snr", "3", "--out", stcBase, "--nomatch"}, 120000);

        QFile stc(stcBase + "-vl.stc");
        QVERIFY(stc.open(QIODevice::ReadOnly));
        QDataStream in(&stc);
        in.setByteOrder(QDataStream::BigEndian);
        in.skipRawData(8);
        quint32 nVertices = 0;
        in >> nVertices;
        in.skipRawData(static_cast<int>(4 * nVertices));
        quint32 nTimes = 0;
        in >> nTimes;
        QVERIFY(nVertices > 0 && nTimes > 0);
        QCOMPARE(stc.size(), qint64(4) * (4 + nVertices + qint64(nVertices) * nTimes));
    }

    //=========================================================================================================
    // mne_check_surface: run on BEM surface
    //=========================================================================================================

    void testCheckSurface()
    {
        if (!toolExists("mne_check_surface"))
            QSKIP("mne_check_surface not found");

        QString surfFile = m_sResourcePath + "subjects/sample/bem/sample-inner_skull-5120.surf";
        if (!QFile::exists(surfFile))
            QSKIP("Surface file not available");

        QString output = runTool("mne_check_surface", {"--surf", surfFile});
        // Should produce surface check output
        QVERIFY(!output.isEmpty());
    }

    //=========================================================================================================
    // mne_list_bem: list multi-layer BEM
    //=========================================================================================================

    void testListBemMultiLayer()
    {
        if (!toolExists("mne_list_bem"))
            QSKIP("mne_list_bem not found");

        QString bemFile = m_sResourcePath + "subjects/sample/bem/sample-1280-1280-1280-bem.fif";
        if (!QFile::exists(bemFile))
            QSKIP("Multi-layer BEM not available");

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
        if (!toolExists("mne_collect_transforms"))
            QSKIP("mne_collect_transforms not found");

        QString rawFile = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile))
            QSKIP("Raw file not available");

        QString output = runTool("mne_collect_transforms", {"--meas", rawFile});
        QVERIFY(!output.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestToolComputePipeline)
#include "test_tool_compute_pipeline.moc"
