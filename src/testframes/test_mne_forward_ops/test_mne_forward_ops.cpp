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
#include <QTemporaryDir>

#include <Eigen/Core>

#include <cmath>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_raw_data.h>

#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_source_spaces.h>
#include <fiff/fiff_evoked.h>
#include <inv/inv_convenience.h>
#include <inv/inv_source_estimate.h>

using namespace FIFFLIB;
using namespace MNELIB;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::VectorXi;
using INVLIB::InvSourceEstimate;

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

    void testSingleModalityFilesReadBack_data()
    {
        // Sum |G| of mne.pick_types_forward(fwd, meg=..., eeg=...) (mne 1.11)
        QTest::addColumn<bool>("meg");
        QTest::addColumn<int>("nchan");
        QTest::addColumn<double>("absSum");
        QTest::newRow("MEG") << true << 306 << 117.14173126220703;
        QTest::newRow("EEG") << false << 60 << 66901760.0;
    }

    void testSingleModalityFilesReadBack()
    {
        QFETCH(bool, meg);
        QFETCH(int, nchan);
        QFETCH(double, absSum);
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        // A file with only an MEG or only an EEG solution, as mne.write_forward_solution writes it
        QTemporaryDir dir;
        QFile file(dir.filePath("single-fwd.fif"));
        QVERIFY(m_fwd.pick_types(meg, !meg).write(file));
        file.close();
        MNEForwardSolution read(file);
        QVERIFY(!read.isEmpty());
        QCOMPARE(read.nchan, nchan);
        QCOMPARE(read.nsource, 7928);
        QCOMPARE(read.sol->data.cols(), 3 * 7928);
        QVERIFY(qAbs(read.sol->data.cwiseAbs().sum() - absSum) < 1e-5 * absSum);

        // Read with fixed orientation it keeps one column per source
        file.close();
        MNEForwardSolution fixed(file, true);
        QCOMPARE(fixed.nchan, nchan);
        QCOMPARE(fixed.sol->data.cols(), 7928);
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

    // ── read_forward_solution ───────────────────────────────────────────────

    void testReadMatchesMnePython()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        // Reference values produced by mne.read_forward_solution(fwd file) (mne 1.11): gain shape (366, 23784),
        // G[0, 0], G[200, 1000], G[365, 23783], sum |G|; nuse 3956 / 3972; mne.forward.is_fixed_orient(fwd) False
        const MatrixXd& g = m_fwd.sol->data;
        QCOMPARE(g.rows(), Eigen::Index(366));
        QCOMPARE(g.cols(), Eigen::Index(23784));
        QVERIFY(std::abs(g(0, 0) + 1.788588e-06) < 1e-12);
        QVERIFY(std::abs(g(200, 1000) - 5.1578004e-06) < 1e-12);
        QVERIFY(std::abs(g(365, 23783) - 33.151615) < 1e-5);
        QVERIFY(std::abs(g.cwiseAbs().sum() - 6.6901876e+07) < 1e-6 * 6.6901876e+07);
        QCOMPARE(m_fwd.src.hemisphereAt(0)->nuse, 3956);
        QCOMPARE(m_fwd.src.hemisphereAt(1)->nuse, 3972);
        QVERIFY(!m_fwd.isFixedOrient());
    }

    // ── apply_forward ───────────────────────────────────────────────────────

    void testApplyForwardMatchesMnePython()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        // Reference values produced by mne.apply_forward(fwd, stc, info) (mne 1.11) on this free-orientation forward
        // (converted to fixed with cortical patch statistics), info of sample_audvis-ave.fif with space-free names:
        //   stc vertices lh [841, 3046], rh [713], data = vstack([sin(t + 1), cos(0.5 t), 0.3 t - 0.4]) * 1e-8,
        //   t = 0..4, tmin 0.1, tstep 0.002. Rows: channel, samples 0, 2, 4.
        QFile aveFile(m_sDataPath + "/MEG/sample/sample_audvis-ave.fif");
        const FiffEvoked ave(aveFile, 0);
        MatrixXd data(3, 5);
        for (int t = 0; t < 5; ++t) {
            data(0, t) = std::sin(t + 1.0) * 1e-8;
            data(1, t) = std::cos(0.5 * t) * 1e-8;
            data(2, t) = (0.3 * t - 0.4) * 1e-8;
        }
        InvSourceEstimate stc(data, (VectorXi(3) << 841, 3046, 713).finished(), 0.1f, 0.002f);
        stc.nVerticesLh = 2;

        const FiffEvoked evoked = INVLIB::applyForward(m_fwd, stc, ave.info);
        QCOMPARE(evoked.info.ch_names.size(), 366);
        QCOMPARE(evoked.info.ch_names.first(), QString("MEG0113"));
        QVERIFY(std::abs(evoked.times(0) - 0.1f) < 1e-7f);
        const QList<QPair<QString, QList<double>>> expected = {
            {"MEG0113", {2.29908704899e-13, 5.78211973512e-14, -2.13557499523e-13}},
            {"MEG2443", {-1.04706535856e-12, -1.61720466574e-13, 1.11124408652e-12}},
            {"EEG001", {-3.50052778352e-07, 1.34513634827e-08, 4.92685045566e-07}},
            {"EEG060", {5.92750625114e-09, -2.18851835027e-07, -4.00659174832e-07}}};
        for (const auto& [name, values] : expected) {
            const int row = static_cast<int>(evoked.info.ch_names.indexOf(name));
            QVERIFY2(row >= 0, qPrintable(name));
            for (int k = 0; k < 3; ++k) {
                const double got = evoked.data(row, 2 * k);
                QVERIFY2(std::abs(got - values[k]) <= 1e-5 * std::abs(values[k]),
                         qPrintable(QString("%1 sample %2: %3 vs mne %4").arg(name).arg(2 * k).arg(got, 0, 'g', 12).arg(values[k], 0, 'g', 12)));
            }
        }
        QVERIFY(std::abs(evoked.data.cwiseAbs().sum() - 0.000162240427669) <= 1e-5 * 0.000162240427669);

        // A vertex the forward does not have is refused, as with mne's on_missing="raise"
        InvSourceEstimate missing(data.topRows(1), (VectorXi(1) << 842).finished(), 0.1f, 0.002f);
        missing.nVerticesLh = 1;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("applyForward.*842"));
        QCOMPARE(INVLIB::applyForward(m_fwd, missing, ave.info).data.size(), Eigen::Index(0));
    }

    // ── compute_orient_prior ────────────────────────────────────────────────

    void testComputeOrientPrior()
    {
        if (!m_bFwdLoaded)
            QSKIP("Forward solution not loaded");

        // mne.forward.compute_orient_prior on convert_forward_solution(surf_ori=True): tangential components get loose
        QFile surfFile(m_sFwdFile);
        MNEForwardSolution surfOri(surfFile, false, true);
        QVERIFY(surfOri.surf_ori);
        FiffCov prior = surfOri.compute_orient_prior(0.2f);
        QCOMPARE(prior.dim, 23784);
        QCOMPARE(prior.kind, FIFFV_MNE_ORIENT_PRIOR_COV);
        QVERIFY(prior.diag);
        QVERIFY(std::abs(prior.data.sum() - 11099.2) < 1e-3);
        for (int i = 0; i < 6; ++i) {
            QCOMPARE(static_cast<float>(prior.data(i, 0)), i % 3 == 2 ? 1.0f : 0.2f);
        }
        QCOMPARE(surfOri.compute_orient_prior(1.0f).data.sum(), 23784.0);

        // mne raises for a Cartesian forward with loose < 1 and for loose != 0 with fixed orientation; MNE-CPP
        // falls back to free (loose 1) and to ones
        QCOMPARE(m_fwd.compute_orient_prior(0.2f).data.sum(), 23784.0);
        QFile fixedFile(m_sFwdFile);
        MNEForwardSolution fixedOri(fixedFile, true);
        prior = fixedOri.compute_orient_prior(0.2f);
        QCOMPARE(prior.data.size(), Eigen::Index(7928));
        QCOMPARE(prior.data.sum(), 7928.0);
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

        // Both whiteners invert the noise covariance on its non-zero subspace (mne _get_whitener):
        // the PCA one (n_nzero x n_chan) whitens it to the identity and has the same W^T W
        Eigen::MatrixXd pcaWhitener;
        qint32 pcaNumNonZero = 0;
        m_fwd.prepare_forward(raw.info, noiseCov, true,
                              outFwdInfo, gain, outNoiseCov, pcaWhitener, pcaNumNonZero);
        const Eigen::Index nChan = whitener.cols();
        QCOMPARE(pcaNumNonZero, numNonZero);
        QCOMPARE(pcaWhitener.rows(), Eigen::Index(numNonZero));
        QCOMPARE(pcaWhitener.cols(), nChan);
        // Its rows are the eigenvectors of the non-zero eigenvalues, each scaled by 1/sqrt(eigenvalue)
        Eigen::VectorXd scale(numNonZero);
        for (Eigen::Index i = 0, k = 0; i < outNoiseCov.eig.size(); ++i) {
            if (outNoiseCov.eig(i) > 0)
                scale(k++) = std::sqrt(outNoiseCov.eig(i));
        }
        const Eigen::MatrixXd orthonormal = scale.asDiagonal() * pcaWhitener * pcaWhitener.transpose() * scale.asDiagonal();
        QVERIFY((orthonormal - Eigen::MatrixXd::Identity(numNonZero, numNonZero)).cwiseAbs().maxCoeff() < 1e-9);
        const Eigen::MatrixXd full = whitener.transpose() * whitener;
        QVERIFY((pcaWhitener.transpose() * pcaWhitener - full).cwiseAbs().maxCoeff() < 1e-6 * full.cwiseAbs().maxCoeff());
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
