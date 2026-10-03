//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_inv_beamformer_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates the LCMV / DICS filter kernel against mne-python's _compute_beamformer.
 *
 * Every weight normalisation, orientation pick and inversion mode is run on
 * the same deterministic leadfield and covariance in both libraries, once with
 * a full-rank and once with a rank-deficient covariance. Reference values:
 *
 *   from mne.beamformer._compute_beamformer import _compute_beamformer
 *   W, mpo = _compute_beamformer(G, Cm, 0.05, 3, weight_norm, pick_ori,
 *                                reduce_rank, None, inversion, nn,
 *                                np.ones(3 * nsrc), np.eye(nch))
 *   np.abs(W).sum(), (W * P[:len(W)]).sum(), np.abs(mpo).sum()
 *
 * with G, Cm, nn and the probe P built by the formulas in makeInputs(). The
 * signed probe sum catches sign and row-order errors an absolute sum misses.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/beamformer/inv_beamformer_compute.h>
#include <inv/beamformer/inv_beamformer_settings.h>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

Q_DECLARE_METATYPE(INVLIB::BeamformerPickOri)
Q_DECLARE_METATYPE(INVLIB::BeamformerWeightNorm)
Q_DECLARE_METATYPE(INVLIB::BeamformerInversion)

namespace
{

constexpr int kChannels = 12;
constexpr int kSources = 5;

struct Inputs
{
    MatrixXd G;
    MatrixXd Cm;
    MatrixX3d nn;
    MatrixXd P;
};

Inputs makeInputs(bool fullRank)
{
    Inputs in;
    in.G.resize(kChannels, 3 * kSources);
    for (int i = 0; i < kChannels; ++i) {
        for (int j = 0; j < 3 * kSources; ++j) {
            in.G(i, j) = std::sin(0.37 * i + 1.3 * j) + 0.1 * std::cos(0.05 * i * j);
        }
    }

    // Full rank: 20 columns plus a ridge. Rank deficient: 8 columns, no ridge.
    const int nCol = fullRank ? 20 : 8;
    const double ridge = fullRank ? 0.1 : 0.0;
    MatrixXd B(kChannels, nCol);
    for (int i = 0; i < kChannels; ++i) {
        for (int k = 0; k < nCol; ++k) {
            B(i, k) = std::cos(0.11 * i * k + 0.3 * i) + 0.2 * std::sin(0.7 * k + 0.05 * i * i);
        }
    }
    in.Cm = B * B.transpose() / nCol + ridge * MatrixXd::Identity(kChannels, kChannels);

    in.nn.resize(kSources, 3);
    for (int s = 0; s < kSources; ++s) {
        in.nn.row(s) = Vector3d(std::sin(s + 1.0), std::cos(2.0 * s), 0.5).normalized().transpose();
    }

    in.P.resize(3 * kSources, kChannels);
    for (int r = 0; r < 3 * kSources; ++r) {
        for (int c = 0; c < kChannels; ++c) {
            in.P(r, c) = std::sin(r + 2.0 * c);
        }
    }
    return in;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestInvBeamformerPython
 *
 * @brief Checks InvBeamformerCompute against mne-python for every filter mode.
 */
class TestInvBeamformerPython : public QObject
{
    Q_OBJECT

private slots:
    void computeBeamformer_data();
    void computeBeamformer();
    void computeBeamformer_invalidInput();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestInvBeamformerPython::computeBeamformer_data()
{
    QTest::addColumn<bool>("fullRank");
    QTest::addColumn<BeamformerPickOri>("pickOri");
    QTest::addColumn<BeamformerWeightNorm>("weightNorm");
    QTest::addColumn<bool>("reduceRank");
    QTest::addColumn<BeamformerInversion>("inversion");
    QTest::addColumn<int>("rows");
    QTest::addColumn<double>("absSum");
    QTest::addColumn<double>("probe");
    QTest::addColumn<double>("oriAbsSum");

    using O = BeamformerPickOri;
    using N = BeamformerWeightNorm;
    using I = BeamformerInversion;

    // clang-format off
    QTest::newRow("full ug")            << true  << O::None     << N::None             << false << I::Matrix << 15 << 520.9576591728969   << 3.5141834608279234    << 0.0;
    QTest::newRow("full ung")           << true  << O::None     << N::UnitNoiseGain    << false << I::Matrix << 15 << 40.99568969534945   << -0.00412670445861521  << 0.0;
    QTest::newRow("full nai")           << true  << O::None     << N::NAI              << false << I::Matrix << 15 << 129.63956490729578  << -0.013049766316688971 << 0.0;
    QTest::newRow("full ung-inv")       << true  << O::None     << N::UnitNoiseGainInv << false << I::Matrix << 15 << 45.36008219212596   << -1.0573726285108098   << 0.0;
    QTest::newRow("full single")        << true  << O::None     << N::None             << false << I::Single << 15 << 25.272433518815745  << -0.6684019393975631   << 0.0;
    QTest::newRow("full ung reduced")   << true  << O::None     << N::UnitNoiseGain    << true  << I::Matrix << 15 << 46.204718542487825  << -1.4089292638424533   << 0.0;
    QTest::newRow("full normal")        << true  << O::Normal   << N::UnitNoiseGain    << false << I::Matrix << 5  << 15.513600822992787  << -0.20691723483506785  << 0.0;
    QTest::newRow("full maxpower ug")   << true  << O::MaxPower << N::None             << false << I::Matrix << 5  << 309.6728666547095   << 16.252082210703787    << 8.386377153961682;
    QTest::newRow("full maxpower ung")  << true  << O::MaxPower << N::UnitNoiseGain    << false << I::Matrix << 5  << 14.578771110787995  << 0.7091519270662531    << 8.298274895208406;
    QTest::newRow("full maxpower nai")  << true  << O::MaxPower << N::NAI              << false << I::Matrix << 5  << 46.10205506312065   << 2.2425320310800467    << 8.298274895208406;
    QTest::newRow("def ug")             << false << O::None     << N::None             << false << I::Matrix << 15 << 1476.2454294038916  << 6.937409731236116     << 0.0;
    QTest::newRow("def ung")            << false << O::None     << N::UnitNoiseGain    << false << I::Matrix << 15 << 41.99837711422167   << -0.07203367801223684  << 0.0;
    QTest::newRow("def nai")            << false << O::None     << N::NAI              << false << I::Matrix << 15 << 241.4130837510289   << -0.4140605789497114   << 0.0;
    QTest::newRow("def ung-inv")        << false << O::None     << N::UnitNoiseGainInv << false << I::Matrix << 15 << 44.26352142931188   << -2.148073331238343    << 0.0;
    QTest::newRow("def single")         << false << O::None     << N::None             << false << I::Single << 15 << 40.155614474021235  << -0.9844414780555005   << 0.0;
    QTest::newRow("def ung reduced")    << false << O::None     << N::UnitNoiseGain    << true  << I::Matrix << 15 << 42.02598200882047   << -3.1948022632016073   << 0.0;
    QTest::newRow("def normal")         << false << O::Normal   << N::UnitNoiseGain    << false << I::Matrix << 5  << 15.843162125180708  << -0.06825111044758006  << 0.0;
    QTest::newRow("def maxpower ug")    << false << O::MaxPower << N::None             << false << I::Matrix << 5  << 878.6518472678716   << -118.77524212698101   << 8.378168780545613;
    QTest::newRow("def maxpower ung")   << false << O::MaxPower << N::UnitNoiseGain    << false << I::Matrix << 5  << 14.621122938496324  << 1.7745315303605105    << 8.417411891896926;
    QTest::newRow("def maxpower nai")   << false << O::MaxPower << N::NAI              << false << I::Matrix << 5  << 84.04444692911838   << 10.200278162955456    << 8.417411891896926;
    QTest::newRow("full ung-inv reduced")      << true  << O::None     << N::UnitNoiseGainInv << true << I::Matrix << 15 << 37.270999832781186 << -1.0822081129220278 << 0.0;
    QTest::newRow("full maxpower ung reduced") << true  << O::MaxPower << N::UnitNoiseGain    << true << I::Matrix << 5  << 15.28959800578124  << -1.2756865604458263 << 7.625717427438488;
    QTest::newRow("def ung-inv reduced")       << false << O::None     << N::UnitNoiseGainInv << true << I::Matrix << 15 << 35.338466905275176 << -2.007682959655285  << 0.0;
    QTest::newRow("def maxpower ung reduced")  << false << O::MaxPower << N::UnitNoiseGain    << true << I::Matrix << 5  << 14.457566864149884 << -2.7021680033358257 << 7.556108102997327;
    // clang-format on
}

//=============================================================================================================

void TestInvBeamformerPython::computeBeamformer()
{
    QFETCH(bool, fullRank);
    QFETCH(BeamformerPickOri, pickOri);
    QFETCH(BeamformerWeightNorm, weightNorm);
    QFETCH(bool, reduceRank);
    QFETCH(BeamformerInversion, inversion);
    QFETCH(int, rows);
    QFETCH(double, absSum);
    QFETCH(double, probe);
    QFETCH(double, oriAbsSum);

    const Inputs in = makeInputs(fullRank);
    MatrixXd W;
    MatrixX3d ori;
    QVERIFY(InvBeamformerCompute::computeBeamformer(in.G, in.Cm, 0.05, 3, weightNorm, pickOri, reduceRank, inversion, in.nn, W, ori));

    QCOMPARE(static_cast<int>(W.rows()), rows);
    QCOMPARE(static_cast<int>(W.cols()), kChannels);

    // Both sides work in double precision on identical inputs; they differ only
    // in eigensolver and summation order, which keeps them within ~1e-10
    // relative. 1e-7 leaves room for platform LAPACK differences and is far
    // below the change of any mode-specific formula.
    const double gotAbs = W.cwiseAbs().sum();
    QVERIFY2(std::fabs(gotAbs - absSum) <= 1e-7 * absSum, qPrintable(QString("|W| sum %1, mne-python %2").arg(gotAbs, 0, 'g', 17).arg(absSum, 0, 'g', 17)));

    // The signed probe can be small relative to |W|, so scale its tolerance by |W|.
    const double gotProbe = W.cwiseProduct(in.P.topRows(rows)).sum();
    QVERIFY2(std::fabs(gotProbe - probe) <= 1e-7 * absSum, qPrintable(QString("probe %1, mne-python %2").arg(gotProbe, 0, 'g', 17).arg(probe, 0, 'g', 17)));

    if (pickOri == BeamformerPickOri::MaxPower) {
        QCOMPARE(static_cast<int>(ori.rows()), kSources);
        const double gotOri = ori.cwiseAbs().sum();
        QVERIFY2(std::fabs(gotOri - oriAbsSum) <= 1e-7 * oriAbsSum, qPrintable(QString("|ori| sum %1, mne-python %2").arg(gotOri, 0, 'g', 17).arg(oriAbsSum, 0, 'g', 17)));
        // Orientations are unit vectors whose sign follows the surface normal.
        for (int s = 0; s < kSources; ++s) {
            QVERIFY(std::fabs(ori.row(s).norm() - 1.0) < 1e-12);
            QVERIFY(ori.row(s).dot(in.nn.row(s)) >= 0.0);
        }
    } else {
        QCOMPARE(static_cast<int>(ori.rows()), 0);
    }
}

//=============================================================================================================

void TestInvBeamformerPython::computeBeamformer_invalidInput()
{
    const Inputs in = makeInputs(true);
    MatrixXd W;
    MatrixX3d ori;

    // Columns not a multiple of the orientation count.
    QVERIFY(!InvBeamformerCompute::computeBeamformer(in.G.leftCols(14), in.Cm, 0.05, 3, BeamformerWeightNorm::None, BeamformerPickOri::None, false, BeamformerInversion::Matrix, in.nn, W, ori));
    // Covariance of the wrong size.
    QVERIFY(!InvBeamformerCompute::computeBeamformer(in.G, in.Cm.topLeftCorner(11, 11), 0.05, 3, BeamformerWeightNorm::None, BeamformerPickOri::None, false, BeamformerInversion::Matrix, in.nn, W, ori));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvBeamformerPython)
#include "test_inv_beamformer_python.moc"
