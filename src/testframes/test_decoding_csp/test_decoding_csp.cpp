//=============================================================================================================
/**
 * @file     test_decoding_csp.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for DecodingCsp (mne.decoding.CSP equivalent).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <decoding/decoding_csp.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <random>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DECODINGLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Helper: generate synthetic 2-class BCI epochs.
 */
static void generateBciData(std::vector<MatrixXd>& epochs,
                            VectorXi& y,
                            int nEpochsPerClass = 30,
                            int nChannels = 8,
                            int nTimes = 200)
{
    std::mt19937 rng(42);
    std::normal_distribution<double> noise(0.0, 1.0);

    const int nTotal = 2 * nEpochsPerClass;
    epochs.resize(static_cast<size_t>(nTotal));
    y.resize(nTotal);

    // Class 0: strong signal in channel 0
    // Class 1: strong signal in channel 7
    for (int i = 0; i < nTotal; ++i) {
        epochs[static_cast<size_t>(i)] = MatrixXd(nChannels, nTimes);
        for (int ch = 0; ch < nChannels; ++ch) {
            for (int t = 0; t < nTimes; ++t) {
                epochs[static_cast<size_t>(i)](ch, t) = noise(rng);
            }
        }

        if (i < nEpochsPerClass) {
            y(i) = 0;
            epochs[static_cast<size_t>(i)].row(0) *= 5.0;
        } else {
            y(i) = 1;
            epochs[static_cast<size_t>(i)].row(nChannels - 1) *= 5.0;
        }
    }
}

//=============================================================================================================

class TestDecodingCsp : public QObject
{
    Q_OBJECT

private slots:
    void testConstruction();
    void testFitAndIsFitted();
    void testTransformAveragePower();
    void testTransformCspSpace();
    void testFitTransform();
    void testInverseTransform();
    void testClassSeparability();
    void testNotFittedThrows();
    void testMismatchedLabelsThrows();
    void testNonBinaryClassThrows();
};

//=============================================================================================================

void TestDecodingCsp::testConstruction()
{
    DecodingCsp csp;
    QVERIFY(!csp.isFitted());

    DecodingCsp csp2(6, DecodingCsp::TransformMode::CspSpace, false);
    QVERIFY(!csp2.isFitted());
}

//=============================================================================================================

void TestDecodingCsp::testFitAndIsFitted()
{
    std::vector<MatrixXd> epochs;
    VectorXi y;
    generateBciData(epochs, y);

    DecodingCsp csp(4);
    csp.fit(epochs, y);
    QVERIFY(csp.isFitted());

    QCOMPARE(csp.filters().rows(), 4);
    QCOMPARE(csp.filters().cols(), 8);
    QCOMPARE(csp.patterns().rows(), 8);
    QCOMPARE(csp.patterns().cols(), 4);
    QCOMPARE(csp.mean().size(), 4);
    QCOMPARE(csp.stddev().size(), 4);
}

//=============================================================================================================

void TestDecodingCsp::testTransformAveragePower()
{
    std::vector<MatrixXd> epochs;
    VectorXi y;
    generateBciData(epochs, y);

    DecodingCsp csp(4, DecodingCsp::TransformMode::AveragePower, true);
    csp.fit(epochs, y);

    MatrixXd features = csp.transform(epochs);
    QCOMPARE(features.rows(), static_cast<int>(epochs.size()));
    QCOMPARE(features.cols(), 4);

    // Log-transformed features should be finite
    QVERIFY(features.allFinite());
}

//=============================================================================================================

void TestDecodingCsp::testTransformCspSpace()
{
    std::vector<MatrixXd> epochs;
    VectorXi y;
    generateBciData(epochs, y, 10, 8, 100);

    DecodingCsp csp(4, DecodingCsp::TransformMode::CspSpace);
    csp.fit(epochs, y);

    MatrixXd result = csp.transform(epochs);
    // CspSpace: (n_epochs * n_components, n_times)
    QCOMPARE(result.rows(), 20 * 4);
    QCOMPARE(result.cols(), 100);
}

//=============================================================================================================

void TestDecodingCsp::testFitTransform()
{
    std::vector<MatrixXd> epochs;
    VectorXi y;
    generateBciData(epochs, y);

    DecodingCsp csp(4);
    MatrixXd features = csp.fitTransform(epochs, y);
    QVERIFY(csp.isFitted());
    QCOMPARE(features.rows(), static_cast<int>(epochs.size()));
    QCOMPARE(features.cols(), 4);
}

//=============================================================================================================

void TestDecodingCsp::testInverseTransform()
{
    std::vector<MatrixXd> epochs;
    VectorXi y;
    generateBciData(epochs, y);

    DecodingCsp csp(4, DecodingCsp::TransformMode::AveragePower, true);
    csp.fit(epochs, y);

    MatrixXd features = csp.transform(epochs);
    MatrixXd reconstructed = csp.inverseTransform(features);

    QCOMPARE(reconstructed.rows(), static_cast<int>(epochs.size()));
    QCOMPARE(reconstructed.cols(), 8);  // n_channels
}

//=============================================================================================================

void TestDecodingCsp::testClassSeparability()
{
    std::vector<MatrixXd> epochs;
    VectorXi y;
    generateBciData(epochs, y, 30, 8, 200);

    DecodingCsp csp(4, DecodingCsp::TransformMode::AveragePower, true);
    csp.fit(epochs, y);
    MatrixXd features = csp.transform(epochs);

    // Class means should differ on first component
    double mean0 = 0.0, mean1 = 0.0;
    int n0 = 0, n1 = 0;
    for (int i = 0; i < y.size(); ++i) {
        if (y(i) == 0) { mean0 += features(i, 0); ++n0; }
        else           { mean1 += features(i, 0); ++n1; }
    }
    mean0 /= n0;
    mean1 /= n1;

    QVERIFY(std::abs(mean0 - mean1) > 0.1);
}

//=============================================================================================================

void TestDecodingCsp::testNotFittedThrows()
{
    DecodingCsp csp;
    std::vector<MatrixXd> epochs = { MatrixXd::Random(8, 100) };

    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
        static_cast<void>(csp.transform(epochs)));
    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
        static_cast<void>(csp.filters()));
    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
        static_cast<void>(csp.patterns()));
}

//=============================================================================================================

void TestDecodingCsp::testMismatchedLabelsThrows()
{
    std::vector<MatrixXd> epochs = { MatrixXd::Random(8, 100),
                                     MatrixXd::Random(8, 100) };
    VectorXi y(3);
    y << 0, 1, 0;

    DecodingCsp csp;
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
        csp.fit(epochs, y));
}

//=============================================================================================================

void TestDecodingCsp::testNonBinaryClassThrows()
{
    std::vector<MatrixXd> epochs = { MatrixXd::Random(8, 100),
                                     MatrixXd::Random(8, 100),
                                     MatrixXd::Random(8, 100) };
    VectorXi y(3);
    y << 0, 1, 2;

    DecodingCsp csp;
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
        csp.fit(epochs, y));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDecodingCsp)
#include "test_decoding_csp.moc"
