//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_inv_lcmv_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates the LCMV and DICS beamformer pipelines against mne-python.
 *
 * test_inv_beamformer_python checks the filter kernel on synthetic whitened
 * inputs. This test runs the whole pipeline on the sample data: noise
 * covariance preparation (SSP + whitening), filter construction from the
 * oct-6 forward and application to data and covariances. Expected values
 * come from mne-python on the same files (the 305 good MEG channels):
 *
 *   fwd = mne.pick_channels_forward(fwd, good, ordered=True)
 *   nc = mne.pick_channels_cov(read_cov(...), good); nc['projs'] = info['projs']
 *   G = fwd['sol']['data']
 *   dc = nc + (20 nAm)^2 g_300 g_300^T + (30 nAm)^2 g_15002 g_15002^T
 *   f = mne.beamformer.make_lcmv(info, fwd, dc, reg=0.05, noise_cov=nc,
 *                                pick_ori=..., weight_norm=...)   # reg=2 for NAI
 *   mne.beamformer.apply_lcmv_cov(dc, f), mne.beamformer.apply_lcmv(evoked, f)
 *
 * with evoked = [20 nAm g_300, 30 nAm g_15002, their sum] (sources 100 and
 * 5000, x and z components). DICS gets the same matrices as a single real
 * cross-spectral density, mne.beamformer.make_dics(..., noise_csd=nc,
 * real_filter=True, depth=None), apply_dics_csd and apply_dics. The
 * resolution matrix uses the forward restricted to lh.V1 (70 sources):
 *
 *   fv = mne.forward.restrict_forward_to_label(fwd, v1)
 *   f = make_lcmv(info, fv, dc, reg=0.05, noise_cov=nc, pick_ori='vector',
 *                 weight_norm='unit-noise-gain')
 *   mne.beamformer.make_lcmv_resolution_matrix(f, fv, info).reshape(210, 210)
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/beamformer/inv_lcmv.h>
#include <inv/beamformer/inv_dics.h>
#include <inv/beamformer/inv_beamformer.h>
#include <inv/inv_source_estimate.h>

#include <mne/mne_forward_solution.h>

#include <fs/fs_label.h>

#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>
#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * Cross validates the LCMV beamformer pipeline against mne-python.
 */
class TestInvLcmvPython : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void powerMatches_data();
    void powerMatches();
    void timeCoursesMatch_data();
    void timeCoursesMatch();
    void dicsPowerMatches_data();
    void dicsPowerMatches();
    void resolutionMatrixMatches();
    void rejectsInvalidInput();

private:
    const InvBeamformer& makeFilter(BeamformerPickOri pickOri, BeamformerWeightNorm weightNorm, double reg = 0.05);
    const InvBeamformer& makeDicsFilter(BeamformerPickOri pickOri);

    QMap<QString, InvBeamformer> m_filters; /**< Each 7928-source filter takes seconds, so build each once. */
    QMap<int, InvBeamformer> m_dicsFilters;
    MNEForwardSolution m_fwd;
    FiffInfo m_info;
    FiffCov m_noiseCov;
    FiffCov m_dataCov;
    FiffEvoked m_evoked;
};

//=============================================================================================================

void TestInvLcmvPython::initTestCase()
{
    const QString base = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
    QFile fwdFile(base + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    QFile covFile(base + "/MEG/sample/sample_audvis-cov.fif");
    QFile aveFile(base + "/MEG/sample/sample_audvis-ave.fif");
    QVERIFY2(fwdFile.exists() && covFile.exists() && aveFile.exists(), "test data missing");

    FiffEvoked sampleEvoked(aveFile, 0, QPair<float, float>(-1.0f, -1.0f), false);
    QStringList good;
    for (const QString& name : sampleEvoked.info.ch_names) {
        if (name.startsWith("MEG") && !sampleEvoked.info.bads.contains(name))
            good << name;
    }
    QCOMPARE(good.size(), 305);

    // The forward names channels without the space ("MEG0113"); the evoked and
    // covariance files use "MEG 0113". Both list them in the same order.
    MNEForwardSolution fullFwd(fwdFile);
    QStringList fwdGood;
    for (const QString& name : fullFwd.sol->row_names) {
        if (name.startsWith("MEG") && name != "MEG2443")
            fwdGood << name;
    }
    m_fwd = fullFwd.pick_channels(fwdGood);
    QCOMPARE(static_cast<int>(m_fwd.sol->data.rows()), 305);
    QCOMPARE(m_fwd.source_ori, FIFFV_MNE_FREE_ORI);
    m_fwd.sol->row_names = good;

    m_evoked = sampleEvoked.pick_channels(good);
    m_evoked.info.bads.clear();
    m_info = m_evoked.info;

    FiffCov cov(covFile);
    m_noiseCov = cov.pick_channels(good).prepare_noise_cov(m_info, good);
    QCOMPARE(static_cast<int>(m_noiseCov.eig.size()), 305);

    const MatrixXd& G = m_fwd.sol->data;
    const VectorXd g1 = G.col(300) * 20e-9;
    const VectorXd g2 = G.col(15002) * 30e-9;
    m_dataCov = m_noiseCov;
    m_dataCov.data += g1 * g1.transpose() + g2 * g2.transpose();

    MatrixXd data(305, 3);
    data << g1, g2, g1 + g2;
    m_evoked.data = data;
    m_evoked.times = RowVectorXf::LinSpaced(3, 0.0f, 2.0f / static_cast<float>(m_info.sfreq));
}

//=============================================================================================================

const InvBeamformer& TestInvLcmvPython::makeFilter(BeamformerPickOri pickOri, BeamformerWeightNorm weightNorm, double reg)
{
    const QString key = QStringLiteral("%1/%2/%3").arg(static_cast<int>(pickOri)).arg(static_cast<int>(weightNorm)).arg(reg);
    auto it = m_filters.find(key);
    if (it == m_filters.end())
        it = m_filters.insert(key, InvLCMV::makeLCMV(m_info, m_fwd, m_dataCov, reg, m_noiseCov, pickOri, weightNorm));
    return *it;
}

//=============================================================================================================

const InvBeamformer& TestInvLcmvPython::makeDicsFilter(BeamformerPickOri pickOri)
{
    auto it = m_dicsFilters.find(static_cast<int>(pickOri));
    if (it == m_dicsFilters.end())
        it = m_dicsFilters.insert(static_cast<int>(pickOri),
                                  InvDICS::makeDICS(m_info, m_fwd, {m_dataCov.data}, VectorXd::Constant(1, 10.0), 0.05, true, m_noiseCov,
                                                    pickOri, BeamformerWeightNorm::UnitNoiseGain));
    return *it;
}

//=============================================================================================================

void TestInvLcmvPython::powerMatches_data()
{
    QTest::addColumn<int>("pickOri");
    QTest::addColumn<int>("weightNorm");
    QTest::addColumn<double>("reg");
    QTest::addColumn<int>("filterRows");
    QTest::addColumn<double>("powerSum");
    QTest::addColumn<double>("power100");
    QTest::addColumn<double>("power5000");
    QTest::addColumn<int>("argmax");

    const int none = static_cast<int>(BeamformerPickOri::None);
    const int maxPower = static_cast<int>(BeamformerPickOri::MaxPower);
    QTest::newRow("free, unnormalised") << none << static_cast<int>(BeamformerWeightNorm::None) << 0.05 << 23784
                                        << 8.834115290796594e-11 << 4.7926169658469376e-15 << 2.032980225149012e-14 << 6133;
    QTest::newRow("free, unit-noise-gain") << none << static_cast<int>(BeamformerWeightNorm::UnitNoiseGain) << 0.05 << 23784
                                           << 23943.98187560067 << 3.87168888569237 << 3.0750761366811536 << 119;
    QTest::newRow("max-power, unit-noise-gain") << maxPower << static_cast<int>(BeamformerWeightNorm::UnitNoiseGain) << 0.05 << 7928
                                                << 8286.943166029216 << 23.582274970244725 << 3.56500089692165 << 100;
    // With reg = 2 the NAI noise level is the loading factor. At reg = 0.05 it is
    // the smallest signal eigenvalue of the whitened covariance, which
    // mne-python's whitener leaves at 0.99999 instead of 1.
    QTest::newRow("max-power, NAI") << maxPower << static_cast<int>(BeamformerWeightNorm::NAI) << 2.0 << 7928
                                    << 4253.91188058609 << 11.001400403194454 << 1.662518843119221 << 100;
}

void TestInvLcmvPython::powerMatches()
{
    QFETCH(int, pickOri);
    QFETCH(int, weightNorm);
    QFETCH(double, reg);
    QFETCH(int, filterRows);
    QFETCH(double, powerSum);
    QFETCH(double, power100);
    QFETCH(double, power5000);
    QFETCH(int, argmax);

    const InvBeamformer& filters = makeFilter(static_cast<BeamformerPickOri>(pickOri), static_cast<BeamformerWeightNorm>(weightNorm), reg);
    QVERIFY(filters.isValid());
    QCOMPARE(static_cast<int>(filters.weights[0].rows()), filterRows);

    const VectorXd power = InvLCMV::applyLCMVCov(m_dataCov, filters).data.col(0);
    QCOMPARE(static_cast<int>(power.size()), 7928);
    QVERIFY2(std::abs(power.sum() - powerSum) < 1e-6 * powerSum,
             qPrintable(QStringLiteral("power sum %1").arg(power.sum(), 0, 'g', 12)));
    QVERIFY2(std::abs(power(100) - power100) < 1e-6 * power100,
             qPrintable(QStringLiteral("power[100] %1").arg(power(100), 0, 'g', 12)));
    QVERIFY2(std::abs(power(5000) - power5000) < 1e-6 * power5000,
             qPrintable(QStringLiteral("power[5000] %1").arg(power(5000), 0, 'g', 12)));
    Index best = 0;
    power.maxCoeff(&best);
    QCOMPARE(static_cast<int>(best), argmax);
}

//=============================================================================================================

void TestInvLcmvPython::timeCoursesMatch_data()
{
    QTest::addColumn<int>("pickOri");
    QTest::addColumn<Vector3d>("source100");
    QTest::addColumn<Vector3d>("source5000");
    QTest::addColumn<double>("absSum");

    QTest::newRow("free, combined") << static_cast<int>(BeamformerPickOri::None)
                                    << Vector3d(0.9317371581675588, 0.05962183003068565, 0.9026704338521776)
                                    << Vector3d(0.07574167387977182, 0.2633238075477357, 0.3146747177227996)
                                    << 1879.888976234267;
    QTest::newRow("max-power") << static_cast<int>(BeamformerPickOri::MaxPower)
                               << Vector3d(4.7520149375907605, 0.025059300341290825, 4.777074330060717)
                               << Vector3d(0.011339320536895105, 1.601521818080431, 1.6128611678629459)
                               << 2550.7834908739705;
}

void TestInvLcmvPython::timeCoursesMatch()
{
    QFETCH(int, pickOri);
    QFETCH(Vector3d, source100);
    QFETCH(Vector3d, source5000);
    QFETCH(double, absSum);

    const BeamformerPickOri ori = static_cast<BeamformerPickOri>(pickOri);
    const InvBeamformer& filters = makeFilter(ori, BeamformerWeightNorm::UnitNoiseGain);
    const InvBeamformer& dics = makeDicsFilter(ori);
    const float tstep = 1.0f / static_cast<float>(m_info.sfreq);

    // mne-python's apply_dics on the same matrices gives the LCMV values too.
    QList<InvSourceEstimate> stcs{InvLCMV::applyLCMV(m_evoked, filters),
                                  InvLCMV::applyLCMVRaw(m_evoked.data, 0.5f, tstep, filters),
                                  InvDICS::applyDICS(m_evoked.data, 0.5f, tstep, dics)};
    stcs << InvLCMV::applyLCMVEpochs({m_evoked.data, m_evoked.data}, 0.5f, tstep, filters)
         << InvDICS::applyDICSEpochs({m_evoked.data, m_evoked.data}, 0.5f, tstep, dics);
    QCOMPARE(stcs.size(), 7);

    for (int i = 0; i < stcs.size(); ++i) {
        const InvSourceEstimate& stc = stcs[i];
        QCOMPARE(static_cast<int>(stc.data.rows()), 7928);
        QCOMPARE(static_cast<int>(stc.data.cols()), 3);
        QCOMPARE(stc.tmin, i == 0 ? 0.0f : 0.5f);
        QCOMPARE(stc.tstep, tstep);

        const Vector3d s100 = stc.data.row(100).transpose();
        const Vector3d s5000 = stc.data.row(5000).transpose();
        QVERIFY2((s100 - source100).norm() < 1e-6 * source100.norm(),
                 qPrintable(QStringLiteral("%1 source 100: %2 %3 %4").arg(i).arg(s100[0], 0, 'g', 10).arg(s100[1], 0, 'g', 10).arg(s100[2], 0, 'g', 10)));
        QVERIFY2((s5000 - source5000).norm() < 1e-6 * source5000.norm(),
                 qPrintable(QStringLiteral("%1 source 5000: %2 %3 %4").arg(i).arg(s5000[0], 0, 'g', 10).arg(s5000[1], 0, 'g', 10).arg(s5000[2], 0, 'g', 10)));
        const double sum = stc.data.cwiseAbs().sum();
        QVERIFY2(std::abs(sum - absSum) < 1e-6 * absSum, qPrintable(QStringLiteral("%1 abs sum %2").arg(i).arg(sum, 0, 'g', 12)));
    }

    QTest::ignoreMessage(QtWarningMsg, "InvLCMV::applyLCMVEpochs - Invalid or non-LCMV filters!");
    QVERIFY(InvLCMV::applyLCMVEpochs({m_evoked.data}, 0.0f, tstep, dics).isEmpty());
    QTest::ignoreMessage(QtWarningMsg, "InvDICS::applyDICS - freqIdx 1 out of range (0..0)!");
    QVERIFY(InvDICS::applyDICS(m_evoked.data, 0.0f, tstep, dics, 1).isEmpty());
}

//=============================================================================================================

void TestInvLcmvPython::dicsPowerMatches_data()
{
    QTest::addColumn<int>("pickOri");
    QTest::addColumn<double>("powerSum");
    QTest::addColumn<double>("power100");
    QTest::addColumn<int>("argmax");

    QTest::newRow("free") << static_cast<int>(BeamformerPickOri::None) << 23943.98187563626 << 3.871688885703522 << 119;
    QTest::newRow("max-power") << static_cast<int>(BeamformerPickOri::MaxPower) << 8286.94316605092 << 23.58227497083058 << 100;
}

void TestInvLcmvPython::dicsPowerMatches()
{
    QFETCH(int, pickOri);
    QFETCH(double, powerSum);
    QFETCH(double, power100);
    QFETCH(int, argmax);

    const InvBeamformer& filters = makeDicsFilter(static_cast<BeamformerPickOri>(pickOri));
    QVERIFY(filters.isValid());

    const VectorXd power = InvDICS::applyDICSCsd({m_dataCov.data}, VectorXd::Constant(1, 10.0), filters).data.col(0);
    QCOMPARE(static_cast<int>(power.size()), 7928);
    QVERIFY2(std::abs(power.sum() - powerSum) < 1e-6 * powerSum,
             qPrintable(QStringLiteral("power sum %1").arg(power.sum(), 0, 'g', 12)));
    QVERIFY2(std::abs(power(100) - power100) < 1e-6 * power100,
             qPrintable(QStringLiteral("power[100] %1").arg(power(100), 0, 'g', 12)));
    Index best = 0;
    power.maxCoeff(&best);
    QCOMPARE(static_cast<int>(best), argmax);

    // Same matrices as LCMV, so the signed max-power orientations must agree.
    if (filters.pickOri == BeamformerPickOri::MaxPower) {
        const InvBeamformer& lcmv = makeFilter(BeamformerPickOri::MaxPower, BeamformerWeightNorm::UnitNoiseGain);
        QVERIFY((filters.maxPowerOri - lcmv.maxPowerOri).cwiseAbs().maxCoeff() < 1e-6);
    }
}

//=============================================================================================================

void TestInvLcmvPython::resolutionMatrixMatches()
{
    FsLabel v1;
    QVERIFY(FsLabel::read(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/label/lh.V1.label", v1));
    const MNEForwardSolution fwd = m_fwd.pick_regions({v1});
    QCOMPARE(static_cast<int>(fwd.nsource), 70);

    const MatrixXd R = InvLCMV::makeLCMVResolutionMatrix(fwd, m_info, m_dataCov, 0.05, m_noiseCov);
    QCOMPARE(static_cast<int>(R.rows()), 210);
    QCOMPARE(static_cast<int>(R.cols()), 210);

    const double trace = R.trace();
    const double absSum = R.cwiseAbs().sum();
    QVERIFY2(std::abs(trace - 18618495563.21897) < 1e-6 * 18618495563.21897, qPrintable(QStringLiteral("trace %1").arg(trace, 0, 'g', 12)));
    QVERIFY2(std::abs(absSum - 1267800906128.2031) < 1e-6 * 1267800906128.2031, qPrintable(QStringLiteral("abs sum %1").arg(absSum, 0, 'g', 12)));
    QVERIFY(std::abs(R(0, 0) - 380180129.19977117) < 1e-6 * 380180129.19977117);
    QVERIFY(std::abs(R(4, 7) - 51239160.0045701) < 1e-6 * 51239160.0045701);
    Index row = 0;
    Index col = 0;
    R.cwiseAbs().maxCoeff(&row, &col);
    QCOMPARE(static_cast<int>(row * 210 + col), 23400);
}

//=============================================================================================================

void TestInvLcmvPython::rejectsInvalidInput()
{
    QTest::ignoreMessage(QtWarningMsg, "InvLCMV::makeLCMV - Forward solution has no gain matrix!");
    QVERIFY(!InvLCMV::makeLCMV(m_info, MNEForwardSolution(), m_dataCov, 0.05, m_noiseCov).isValid());
    FiffCov smallCov = m_dataCov;
    smallCov.data = smallCov.data.topLeftCorner(10, 10).eval(); // resizing frees the block being read
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Data covariance dimension \\(10 x 10\\)"));
    QVERIFY(!InvLCMV::makeLCMV(m_info, m_fwd, smallCov, 0.05, m_noiseCov).isValid());

    const VectorXd f10 = VectorXd::Constant(1, 10.0);
    QTest::ignoreMessage(QtWarningMsg, "InvDICS::makeDICS - No CSD matrices provided!");
    QVERIFY(!InvDICS::makeDICS(m_info, m_fwd, {}, f10).isValid());
    QTest::ignoreMessage(QtWarningMsg, "InvDICS::makeDICS - Frequency vector size mismatch with CSD count!");
    QVERIFY(!InvDICS::makeDICS(m_info, m_fwd, {m_dataCov.data, m_dataCov.data}, f10).isValid());
    QTest::ignoreMessage(QtWarningMsg, "InvDICS::makeDICS - Forward solution has no gain matrix!");
    QVERIFY(!InvDICS::makeDICS(m_info, MNEForwardSolution(), {m_dataCov.data}, f10).isValid());
    QTest::ignoreMessage(QtWarningMsg, "InvDICS::makeDICS - CSD[0] dimension mismatch!");
    QVERIFY(!InvDICS::makeDICS(m_info, m_fwd, {smallCov.data}, f10).isValid());

    // A DICS filter for one frequency rejects two CSDs, a bad frequency index and LCMV filters.
    const InvBeamformer& dics = makeDicsFilter(BeamformerPickOri::MaxPower);
    QTest::ignoreMessage(QtWarningMsg, "InvDICS::applyDICSCsd - CSD count (2) does not match filter count (1)!");
    QCOMPARE(InvDICS::applyDICSCsd({m_dataCov.data, m_dataCov.data}, VectorXd::Constant(2, 10.0), dics).data.size(), Index(0));
    QTest::ignoreMessage(QtWarningMsg, "InvDICS::applyDICS - freqIdx 1 out of range (0..0)!");
    QCOMPARE(InvDICS::applyDICS(m_evoked.data, 0.0f, 0.01f, dics, 1).data.size(), Index(0));
    const InvBeamformer& lcmv = makeFilter(BeamformerPickOri::MaxPower, BeamformerWeightNorm::UnitNoiseGain);
    QTest::ignoreMessage(QtWarningMsg, "InvDICS::applyDICS - Invalid or non-DICS filters!");
    QCOMPARE(InvDICS::applyDICS(m_evoked.data, 0.0f, 0.01f, lcmv, 0).data.size(), Index(0));
    QTest::ignoreMessage(QtWarningMsg, "InvDICS::applyDICSCsd - Invalid or non-DICS filters!");
    QCOMPARE(InvDICS::applyDICSCsd({m_dataCov.data}, f10, lcmv).data.size(), Index(0));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvLcmvPython)
#include "test_inv_lcmv_python.moc"
