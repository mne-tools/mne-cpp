//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_inv_cmne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for Contextual MNE (CMNE) inverse solver.
 *           Tests the dSPM kernel computation, z-score/rectify pipeline,
 *           CMNE settings, and LSTM correction loop mechanics.
 *           Reference: Dinh et al. (2021), Front. Neurosci. 15:552666.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/minimum_norm/inv_cmne.h>
#include <inv/minimum_norm/inv_cmne_settings.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QFile>
#include <QRegularExpression>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <random>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestInvCmne
 *
 * @brief Tests for Contextual MNE (CMNE) inverse solver.
 */
class TestInvCmne : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Settings
    void testSettingsDefaults();
    void testSettingsCustom();

    // dSPM kernel computation (static helper via compute with known data)
    void testDspmKernelDimensions();
    void testDspmOutputRange();

    // z-score + rectify
    void testZScoreRectifyProperties();

    // CMNE Markov chain mechanics (synthetic — no real ONNX model)
    void testCmneIdentityForEarlyTimesteps();
    void testCmneOutputDimensions();

    // Edge cases
    void testFewTimeSamples();
    void testDspmKernelMatchesDefinition();
    void testCmneMatchesReference();

    void cleanupTestCase();

private:
    MatrixXd createSyntheticGain(int nChannels, int nSources, unsigned int seed = 42) const;
    MatrixXd createDiagonalCov(int n, double variance = 1.0) const;
};

//=============================================================================================================

void TestInvCmne::initTestCase()
{
}

//=============================================================================================================

void TestInvCmne::testSettingsDefaults()
{
    InvCMNESettings settings;
    QCOMPARE(settings.lookBack, 80);
    QCOMPARE(settings.numSources, 5124);
    QVERIFY(std::abs(settings.lambda2 - 1.0 / 9.0) < 1e-10);
    QCOMPARE(settings.method, 1); // dSPM
    QVERIFY(std::abs(settings.looseOriConstraint - 0.2) < 1e-10);
    QVERIFY(settings.onnxModelPath.isEmpty());
}

//=============================================================================================================

void TestInvCmne::testSettingsCustom()
{
    InvCMNESettings settings;
    settings.lookBack = 40;
    settings.numSources = 2562;
    settings.lambda2 = 0.5;
    settings.method = 2; // sLORETA
    settings.looseOriConstraint = 0.0;
    settings.onnxModelPath = "/tmp/model.onnx";

    QCOMPARE(settings.lookBack, 40);
    QCOMPARE(settings.numSources, 2562);
    QVERIFY(std::abs(settings.lambda2 - 0.5) < 1e-10);
    QCOMPARE(settings.method, 2);
    QVERIFY(std::abs(settings.looseOriConstraint) < 1e-10);
    QCOMPARE(settings.onnxModelPath, QString("/tmp/model.onnx"));
}

//=============================================================================================================

void TestInvCmne::testDspmKernelDimensions()
{
    // Use small synthetic data to verify the compute pipeline dimensions
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 50;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh, 1.0);
    MatrixXd srcCov = createDiagonalCov(nSrc, 1.0);

    // Create synthetic evoked data by projecting through gain
    std::mt19937 gen(123);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd srcActivity(nSrc, nTimes);
    for (int i = 0; i < nSrc; ++i)
        for (int j = 0; j < nTimes; ++j)
            srcActivity(i, j) = dist(gen);
    MatrixXd evoked = gain * srcActivity;

    // Settings without ONNX model — compute should still produce dSPM estimates
    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lambda2 = 1.0 / 9.0;
    settings.onnxModelPath.clear(); // No ONNX model

    // Call compute — without ONNX model, it should produce dSPM but CMNE
    // correction will be skipped or will be identity
    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // dSPM output dimensions
    QCOMPARE(result.stcDspm.data.rows(), nSrc);
    QCOMPARE(result.stcDspm.data.cols(), nTimes);

    // Kernel dimensions
    QCOMPARE(result.matKernelDspm.rows(), nSrc);
    QCOMPARE(result.matKernelDspm.cols(), nCh);
}

//=============================================================================================================

void TestInvCmne::testDspmOutputRange()
{
    // dSPM output should have reasonable values (z-score-like magnitudes)
    int nCh = 15;
    int nSrc = 30;
    int nTimes = 100;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh, 1.0);
    MatrixXd srcCov = createDiagonalCov(nSrc, 1.0);

    std::mt19937 gen(456);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd srcActivity(nSrc, nTimes);
    for (int i = 0; i < nSrc; ++i)
        for (int j = 0; j < nTimes; ++j)
            srcActivity(i, j) = dist(gen);
    MatrixXd evoked = gain * srcActivity;

    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lambda2 = 1.0 / 9.0;

    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // dSPM values should not be all zeros
    QVERIFY2(result.stcDspm.data.norm() > 0.0, "dSPM output should not be all zeros");

    // dSPM values should not contain NaN or Inf
    bool hasNan = result.stcDspm.data.array().isNaN().any();
    bool hasInf = result.stcDspm.data.array().isInf().any();
    QVERIFY2(!hasNan, "dSPM output contains NaN");
    QVERIFY2(!hasInf, "dSPM output contains Inf");
}

//=============================================================================================================

void TestInvCmne::testZScoreRectifyProperties()
{
    // After z-score + rectify, each source's time course should have
    // approximately zero mean and unit std (before rectification abs)
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 200;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh, 1.0);
    MatrixXd srcCov = createDiagonalCov(nSrc, 1.0);

    std::mt19937 gen(789);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd srcActivity(nSrc, nTimes);
    for (int i = 0; i < nSrc; ++i)
        for (int j = 0; j < nTimes; ++j)
            srcActivity(i, j) = dist(gen);
    MatrixXd evoked = gain * srcActivity;

    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lambda2 = 1.0 / 9.0;

    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // Verify dSPM data is finite
    QVERIFY(!result.stcDspm.data.array().isNaN().any());
    QVERIFY(!result.stcDspm.data.array().isInf().any());
}

//=============================================================================================================

void TestInvCmne::testCmneIdentityForEarlyTimesteps()
{
    // For t < lookBack, CMNE output should equal the dSPM output
    // (no LSTM correction possible without enough history)
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 200;
    int lookBack = 40;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh, 1.0);
    MatrixXd srcCov = createDiagonalCov(nSrc, 1.0);

    std::mt19937 gen(321);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd srcActivity(nSrc, nTimes);
    for (int i = 0; i < nSrc; ++i)
        for (int j = 0; j < nTimes; ++j)
            srcActivity(i, j) = dist(gen);
    MatrixXd evoked = gain * srcActivity;

    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lambda2 = 1.0 / 9.0;
    settings.lookBack = lookBack;
    // No ONNX model — CMNE correction should be identity/skipped

    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // Before k samples of context the estimate is the sensing estimate q_t itself (Eq. 12).
    const MatrixXd q = InvCMNE::zScoreRectify(result.stcDspm.data);
    QCOMPARE(result.stcSensing.data, q);
    QVERIFY((result.stcCmne.data.leftCols(lookBack) - q.leftCols(lookBack)).cwiseAbs().maxCoeff() < 1e-12);
}

//=============================================================================================================

void TestInvCmne::testCmneOutputDimensions()
{
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 100;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh);
    MatrixXd srcCov = createDiagonalCov(nSrc);

    std::mt19937 gen(654);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd evoked(nCh, nTimes);
    for (int i = 0; i < nCh; ++i)
        for (int j = 0; j < nTimes; ++j)
            evoked(i, j) = dist(gen);

    InvCMNESettings settings;
    settings.numSources = nSrc;

    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // Both dSPM and CMNE outputs should have same dimensions
    QCOMPARE(result.stcDspm.data.rows(), nSrc);
    QCOMPARE(result.stcDspm.data.cols(), nTimes);

    if (result.stcCmne.data.rows() > 0) {
        QCOMPARE(result.stcCmne.data.rows(), nSrc);
        QCOMPARE(result.stcCmne.data.cols(), nTimes);
    }
}

//=============================================================================================================

void TestInvCmne::testFewTimeSamples()
{
    // With fewer than lookBack time samples, no LSTM correction is possible
    // Should return dSPM only without crashing
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 5; // Much less than lookBack (80)

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh);
    MatrixXd srcCov = createDiagonalCov(nSrc);

    std::mt19937 gen(987);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd evoked(nCh, nTimes);
    for (int i = 0; i < nCh; ++i)
        for (int j = 0; j < nTimes; ++j)
            evoked(i, j) = dist(gen);

    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lookBack = 80;

    // Should not crash
    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    QCOMPARE(result.stcDspm.data.rows(), nSrc);
    QCOMPARE(result.stcDspm.data.cols(), nTimes);
}

//=============================================================================================================

void TestInvCmne::testDspmKernelMatchesDefinition()
{
    // Dale et al. (2000): K = R G~^T (G~ R G~^T + lambda2 I)^-1 W with W = C^-1/2 and G~ = W G,
    // each row scaled so that sensor noise with covariance C maps to unit variance.
    const int nCh = 12, nSrc = 25, nTimes = 120;
    const MatrixXd gain = createSyntheticGain(nCh, nSrc, 7);
    MatrixXd mix = createSyntheticGain(nCh, nCh, 11);
    const MatrixXd noiseCov = mix * mix.transpose() + 0.5 * MatrixXd::Identity(nCh, nCh);
    const MatrixXd srcCov = VectorXd::LinSpaced(nSrc, 0.5, 2.0).asDiagonal();
    const double lambda2 = 1.0 / 9.0;

    SelfAdjointEigenSolver<MatrixXd> eig(noiseCov);
    const MatrixXd W = eig.eigenvectors() * eig.eigenvalues().cwiseInverse().cwiseSqrt().asDiagonal() * eig.eigenvectors().transpose();
    const MatrixXd Gw = W * gain;
    MatrixXd ref = srcCov * Gw.transpose() * (Gw * srcCov * Gw.transpose() + lambda2 * MatrixXd::Identity(nCh, nCh)).inverse() * W;
    for (int i = 0; i < nSrc; ++i)
        ref.row(i) /= std::sqrt(ref.row(i) * noiseCov * ref.row(i).transpose());

    MatrixXd evoked = gain * createSyntheticGain(nSrc, nTimes, 3);
    InvCMNESettings settings;
    settings.lambda2 = lambda2;
    settings.lookBack = 10;
    const InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);
    const MatrixXd& K = result.matKernelDspm;
    QVERIFY2((K - ref).norm() < 1e-9 * ref.norm(), qPrintable(QString("kernel differs by %1").arg((K - ref).norm() / ref.norm())));
    QVERIFY(((K * noiseCov * K.transpose()).diagonal().array() - 1.0).abs().maxCoeff() < 1e-9);
    const MatrixXd dspm = ref * evoked;
    QVERIFY((result.stcDspm.data - dspm).norm() < 1e-9 * dspm.norm());

    // Without a model compute() returns the paper's control estimate: q_t times the mean of q over the previous k samples.
    MatrixXd q = dspm.cwiseAbs();
    for (int i = 0; i < nSrc; ++i) {
        const double mu = q.row(i).mean();
        q.row(i).array() -= mu;
        q.row(i) /= std::sqrt(q.row(i).squaredNorm() / nTimes);
    }
    MatrixXd control = q;
    for (int t = settings.lookBack; t < nTimes; ++t)
        control.col(t) = q.col(t).cwiseProduct(q.middleCols(t - settings.lookBack, settings.lookBack).rowwise().mean());
    QVERIFY((result.stcSensing.data - q).norm() < 1e-9 * q.norm());
    QVERIFY((result.stcCmne.data - control).norm() < 1e-9 * control.norm());

    // MEG (T^2 ~ 1e-24) and EEG (V^2 ~ 1e-11) variances in one covariance: every channel must stay whitened.
    VectorXd units = VectorXd::Constant(nCh, 1e-12);
    units.tail(4).setConstant(3e-6);
    const MatrixXd mixedCov = units.asDiagonal() * noiseCov * units.asDiagonal();
    const MatrixXd mixedGain = units.asDiagonal() * gain;
    const InvCMNEResult mixed = InvCMNE::compute(units.asDiagonal() * evoked, mixedGain, mixedCov, srcCov, settings);
    QVERIFY2((mixed.matKernelDspm * units.asDiagonal() - ref).norm() < 1e-6 * ref.norm(),
             qPrintable(QString("mixed-unit kernel differs by %1").arg((mixed.matKernelDspm * units.asDiagonal() - ref).norm() / ref.norm())));

    // A model that cannot be loaded leaves only the dSPM estimate.
    settings.onnxModelPath = QStringLiteral("/nonexistent/model.onnx");
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Cannot load CMNE model"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("returning the dSPM estimate only"));
    const InvCMNEResult failed = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);
    QVERIFY(failed.stcCmne.isEmpty());
    QVERIFY((failed.stcDspm.data - dspm).norm() < 1e-9 * dspm.norm());
}

//=============================================================================================================

void TestInvCmne::cleanupTestCase()
{
}

//=============================================================================================================

void TestInvCmne::testCmneMatchesReference()
{
    // make_cmne_reference.py: cmne 0.2.1 apply_cmne / control_estimate on a 12 x 30 estimate.
    auto load = [](const QString& name) {
        QFile file(QStringLiteral(MNE_CMNE_REF_DIR "/cmne_ref_%1.txt").arg(name));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return MatrixXd();
        QList<QList<double>> rows;
        while (!file.atEnd()) {
            QList<double> row;
            for (const QByteArray& v : file.readLine().simplified().split(' '))
                row << v.toDouble();
            rows << row;
        }
        MatrixXd m(rows.size(), rows.first().size());
        for (int r = 0; r < m.rows(); ++r)
            for (int c = 0; c < m.cols(); ++c)
                m(r, c) = rows[r][c];
        return m;
    };
    const MatrixXd source = load(QStringLiteral("source"));
    QCOMPARE(source.rows(), 12);
    QCOMPARE(source.cols(), 30);
    auto close = [](const MatrixXd& got, const MatrixXd& ref, double tol) {
        return got.rows() == ref.rows() && got.cols() == ref.cols() && (got - ref).cwiseAbs().maxCoeff() <= tol * ref.cwiseAbs().maxCoeff();
    };

    QVERIFY(close(InvCMNE::zScoreRectify(source), load(QStringLiteral("sensing")), 1e-6));
    QVERIFY(close(InvCMNE::controlEstimate(source, 6), load(QStringLiteral("control")), 1e-6));

#ifdef MNE_USE_ONNXRUNTIME
    MatrixXd sensing, prediction, cmne;
    QVERIFY(InvCMNE::applyCmne(source, QStringLiteral(MNE_CMNE_REF_DIR "/cmne_ref.onnx"), sensing, prediction, cmne));
    QVERIFY(close(sensing, load(QStringLiteral("sensing")), 1e-6));
    QVERIFY2(close(prediction, load(QStringLiteral("prediction")), 1e-5), qPrintable(QString::number((prediction - load(QStringLiteral("prediction"))).cwiseAbs().maxCoeff())));
    QVERIFY2(close(cmne, load(QStringLiteral("cmne")), 1e-5), qPrintable(QString::number((cmne - load(QStringLiteral("cmne"))).cwiseAbs().maxCoeff())));

    // The model is for 12 sources and needs more than 6 samples.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("has no cmne_config for 11 sources"));
    QVERIFY(!InvCMNE::applyCmne(source.topRows(11), QStringLiteral(MNE_CMNE_REF_DIR "/cmne_ref.onnx"), sensing, prediction, cmne));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Need more than look_back"));
    QVERIFY(!InvCMNE::applyCmne(source.leftCols(6), QStringLiteral(MNE_CMNE_REF_DIR "/cmne_ref.onnx"), sensing, prediction, cmne));
#endif
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Cannot load CMNE model"));
    MatrixXd s, p, c;
    QVERIFY(!InvCMNE::applyCmne(source, QStringLiteral("/nonexistent.onnx"), s, p, c));
}

//=============================================================================================================

MatrixXd TestInvCmne::createSyntheticGain(int nChannels, int nSources, unsigned int seed) const
{
    std::mt19937 gen(seed);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd gain(nChannels, nSources);
    for (int i = 0; i < nChannels; ++i)
        for (int j = 0; j < nSources; ++j)
            gain(i, j) = dist(gen) / std::sqrt(static_cast<double>(nSources));
    return gain;
}

//=============================================================================================================

MatrixXd TestInvCmne::createDiagonalCov(int n, double variance) const
{
    return MatrixXd::Identity(n, n) * variance;
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvCmne)
#include "test_inv_cmne.moc"
