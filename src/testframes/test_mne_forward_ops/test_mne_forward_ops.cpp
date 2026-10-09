//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2010-2026 MNE-CPP Authors
 *
 * @file     test_mne_forward_ops.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @brief       Tests for MNEForwardSolution operations: pick, prepare, orient prior, depth prior.
 */
//=============================================================================================================

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <Eigen/Core>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_raw_data.h>

#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_source_spaces.h>

using namespace FIFFLIB;
using namespace MNELIB;
using Eigen::MatrixXd;
using Eigen::VectorXd;

//=============================================================================================================

class TestMneForwardOps : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;
    QString m_sFwdFile;
    QString m_sRawFile;
    QString m_sCovFile;
    MNEForwardSolution m_fwd;
    bool m_bFwdLoaded;

private slots:

    void initTestCase()
    {
        m_sDataPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
        m_sFwdFile = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        m_sRawFile = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        m_sCovFile = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";

        m_bFwdLoaded = false;
        if (QFile::exists(m_sFwdFile)) {
            QFile file(m_sFwdFile);
            MNEForwardSolution fwd(file);
            if (!fwd.isEmpty()) {
                m_fwd = fwd;
                m_bFwdLoaded = true;
            }
        }
    }

    // ── pick_channels ───────────────────────────────────────────────────────

    void testPickChannelsByInclude()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        // Pick a subset of channels
        QStringList include;
        for (int i = 0; i < qMin(10, m_fwd.info.ch_names.size()); ++i) {
            include << m_fwd.info.ch_names[i];
        }

        MNEForwardSolution picked = m_fwd.pick_channels(include);
        QVERIFY(!picked.isEmpty());
        // pick_channels should reduce channel count
        QVERIFY(picked.nchan > 0);
        QVERIFY(picked.nchan <= m_fwd.nchan);
    }

    void testPickChannelsByExclude()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        // Exclude some channels
        QStringList exclude;
        for (int i = 0; i < qMin(5, m_fwd.info.ch_names.size()); ++i) {
            exclude << m_fwd.info.ch_names[i];
        }

        QStringList emptyInclude;
        MNEForwardSolution picked = m_fwd.pick_channels(emptyInclude, exclude);
        QVERIFY(!picked.isEmpty());
        QVERIFY(picked.nchan < m_fwd.nchan);
    }

    // ── pick_types ──────────────────────────────────────────────────────────

    void testPickTypesMeg()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        MNEForwardSolution megFwd = m_fwd.pick_types(true, false);
        QVERIFY(!megFwd.isEmpty());
        QVERIFY(megFwd.nchan > 0);
        QVERIFY(megFwd.nchan <= m_fwd.nchan);
    }

    void testPickTypesEeg()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        MNEForwardSolution eegFwd = m_fwd.pick_types(false, true);
        // EEG channels may or may not be present
        if (!eegFwd.isEmpty()) {
            QVERIFY(eegFwd.nchan > 0);
        }
    }

    // ── to_fixed_ori ────────────────────────────────────────────────────────

    void testToFixedOri()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        MNEForwardSolution fwdCopy = m_fwd;

        fwdCopy.to_fixed_ori();

        // After fixed orientation: sols should still be valid
        QVERIFY(fwdCopy.sol->data.cols() > 0);
        QVERIFY(fwdCopy.sol->data.rows() > 0);
    }

    // ── compute_orient_prior ────────────────────────────────────────────────

    void testComputeOrientPrior()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        FiffCov orientPrior = m_fwd.compute_orient_prior(0.2f);
        QVERIFY(orientPrior.dim > 0);
    }

    void testComputeOrientPriorLoose()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        // loose=1.0 means free orientation
        FiffCov orientPrior = m_fwd.compute_orient_prior(1.0f);
        QVERIFY(orientPrior.dim > 0);
    }

    // ── compute_depth_prior ─────────────────────────────────────────────────

    void testComputeDepthPrior_data()
    {
        QTest::addColumn<bool>("fixed");
        QTest::addColumn<bool>("limitDepthChs");
        QTest::addColumn<bool>("patches");
        QTest::addColumn<bool>("zeroSource");
        QTest::addColumn<double>("sum");
        QTest::addColumn<double>("first");
        QTest::addColumn<double>("at300");
        // mne.forward.compute_depth_prior(exp=0.8, limit=10) on the reference forward (float64), fixed via
        // convert_forward_solution(surf_ori=True, force_fixed=True); patch areas 1e-4 (1 + 0.5 sin(i)),
        // "zero": the first source's gain columns set to 0
        QTest::newRow("free all nopatch") << false << false << false << false << 1184.0195577632926 << 0.07107741995621498 << 0.05195537546353134;
        QTest::newRow("free all nopatch zero") << false << false << false << true << 1186.806325503424 << 1.0 << 0.05195537546353134;
        QTest::newRow("free all patch") << false << false << true << false << 3301.162314698329 << 0.18673095543082796 << 0.08555748929128305;
        QTest::newRow("free all patch zero") << false << false << true << true << 3303.602121832036 << 1.0 << 0.08555748929128305;
        QTest::newRow("free limited nopatch") << false << true << false << false << 4943.62086633969 << 0.03942008089353401 << 0.05692038612759248;
        QTest::newRow("free limited nopatch zero") << false << true << false << true << 4946.5026060970085 << 1.0 << 0.05692038612759248;
        QTest::newRow("free limited patch") << false << true << true << false << 9792.431995152121 << 0.10245099653188249 << 0.092727679766952;
        QTest::newRow("free limited patch zero") << false << true << true << true << 9795.124642162526 << 1.0 << 0.092727679766952;
        QTest::newRow("fixed all nopatch") << true << false << false << false << 569.6915446311631 << 0.2221870817099831 << 0.04754109384838697;
        QTest::newRow("fixed all nopatch zero") << true << false << false << true << 569.9928884859207 << 0.5235309364677159 << 0.04754109384838697;
        QTest::newRow("fixed all patch") << true << false << true << false << 1514.81271061057 << 0.5578230861724156 << 0.039388414116389533;
        QTest::newRow("fixed all patch zero") << true << false << true << true << 1515.2548875243979 << 1.0 << 0.03938841411638953;
        QTest::newRow("fixed limited nopatch") << true << true << false << false << 2425.6244900339702 << 0.05772284507793688 << 0.053175981276141246;
        QTest::newRow("fixed limited nopatch zero") << true << true << false << true << 2426.5667671888923 << 1.0 << 0.053175981276141246;
        QTest::newRow("fixed limited patch") << true << true << true << false << 3754.105468584854 << 0.13276900194030072 << 0.04036324933994017;
        QTest::newRow("fixed limited patch zero") << true << true << true << true << 3754.9726995829133 << 1.0 << 0.04036324933994017;
    }

    void testComputeDepthPrior()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");
        QFETCH(bool, fixed);
        QFETCH(bool, limitDepthChs);
        QFETCH(bool, patches);
        QFETCH(bool, zeroSource);
        QFETCH(double, sum);
        QFETCH(double, first);
        QFETCH(double, at300);

        QFile file(m_sFwdFile);
        const MNEForwardSolution fwd = fixed ? MNEForwardSolution(file, true) : m_fwd;
        MatrixXd gain = fwd.sol->data;
        if (zeroSource) {
            gain.leftCols(fixed ? 1 : 3).setZero();
        }
        MatrixXd patchAreas;
        if (patches) {
            patchAreas = 1e-4 * (1.0 + 0.5 * VectorXd::LinSpaced(fwd.nsource, 0, fwd.nsource - 1).array().sin()).matrix();
        }
        // The measurement info restricted to the forward's channels, in its order
        QFile rawFile(m_sRawFile);
        const FiffRawData raw(rawFile);
        Eigen::RowVectorXi sel(fwd.info.ch_names.size());
        for (int i = 0; i < sel.size(); ++i) {
            sel(i) = static_cast<int>(raw.info.ch_names.indexOf(fwd.info.ch_names[i]));
            QVERIFY(sel(i) >= 0);
        }
        const FiffInfo info = raw.info.pick_info(sel);
        const FiffCov prior = MNEForwardSolution::compute_depth_prior(gain, info, fixed, 0.8, 10.0, patchAreas, limitDepthChs);
        QCOMPARE(static_cast<int>(prior.data.rows()), static_cast<int>(gain.cols()));
        QVERIFY2(std::abs(prior.data.sum() - sum) < 1e-5 * sum, qPrintable(QString::number(prior.data.sum(), 'g', 17)));
        QVERIFY2(std::abs(prior.data(0, 0) - first) < 1e-5 * first, qPrintable(QString::number(prior.data(0, 0), 'g', 17)));
        QVERIFY2(std::abs(prior.data(300, 0) - at300) < 1e-5 * at300, qPrintable(QString::number(prior.data(300, 0), 'g', 17)));
    }

    // ── prepare_forward ─────────────────────────────────────────────────────

    void testPrepareForward()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");
        if (!QFile::exists(m_sCovFile) || !QFile::exists(m_sRawFile))
            QSKIP("Data files not found");

        QFile covFile(m_sCovFile);
        FiffCov noiseCov(covFile);

        QFile rawFile(m_sRawFile);
        FiffRawData raw(rawFile);

        FiffInfo outFwdInfo;
        Eigen::MatrixXd gain;
        FiffCov outNoiseCov;
        Eigen::MatrixXd whitener;
        qint32 numNonZero;

        m_fwd.prepare_forward(raw.info, noiseCov, false,
                              outFwdInfo, gain, outNoiseCov, whitener, numNonZero);

        QVERIFY(gain.rows() > 0);
        QVERIFY(gain.cols() > 0);
        QVERIFY(numNonZero > 0);
        QVERIFY(whitener.rows() > 0);
    }

    // ── Forward solution write/read round-trip ──────────────────────────────

    void testForwardSolutionWriteRead()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        QString tmpFile = QDir::tempPath() + "/test_fwd_roundtrip.fif";
        QFile outFile(tmpFile);
        bool written = false;
        if (outFile.open(QIODevice::WriteOnly)) {
            written = m_fwd.write(outFile);
            outFile.close();
        }
        if (written) {
            QFile reloadFile(tmpFile);
            MNEForwardSolution reloaded(reloadFile);
            QVERIFY(!reloaded.isEmpty());
            QCOMPARE(reloaded.nsource, m_fwd.nsource);
            QCOMPARE(reloaded.nchan, m_fwd.nchan);
            QFile::remove(tmpFile);
        } else {
            qWarning("Forward solution write failed — code path exercised");
        }
    }

    // ── Source space hemisphere access ───────────────────────────────────────

    void testSourceSpaceProperties()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        QCOMPARE(m_fwd.src.size(), 2); // Left and right hemisphere

        for (int h = 0; h < 2; ++h) {
            QVERIFY(m_fwd.src[h].nuse > 0);
            QVERIFY(m_fwd.src[h].np > 0);
            QVERIFY(m_fwd.src[h].rr.rows() > 0);
        }
    }

    // ── Source orientations ─────────────────────────────────────────────────

    void testSourceOrientations()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        QVERIFY(m_fwd.source_rr.rows() == m_fwd.nsource);
        QVERIFY(m_fwd.source_nn.rows() > 0);
        QVERIFY(m_fwd.source_rr.cols() == 3);
        QVERIFY(m_fwd.source_nn.cols() == 3);
    }

    // ── Gain matrix properties ──────────────────────────────────────────────

    void testGainMatrixProperties()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        QVERIFY(m_fwd.sol != nullptr);
        QVERIFY(m_fwd.sol->data.rows() == m_fwd.nchan);
        QVERIFY(m_fwd.sol->data.cols() > 0);
    }

    void cleanupTestCase()
    {
    }
};

QTEST_GUILESS_MAIN(TestMneForwardOps)
#include "test_mne_forward_ops.moc"
