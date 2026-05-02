//=============================================================================================================
/**
 * @file     test_decoding_spoc.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for DecodingSpoc (mne.decoding.SPoC equivalent).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <decoding/decoding_spoc.h>

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
 * @brief Helper: generate epochs whose channel-0 power covaries with target.
 */
static void generateSpocData(std::vector<MatrixXd>& epochs,
                             VectorXd& target,
                             int nEpochs = 40,
                             int nChannels = 6,
                             int nTimes = 150)
{
    std::mt19937 rng(123);
    std::normal_distribution<double> noise(0.0, 1.0);

    epochs.resize(static_cast<size_t>(nEpochs));
    target.resize(nEpochs);

    for (int i = 0; i < nEpochs; ++i) {
        double t = static_cast<double>(i) / nEpochs;
        target(i) = t;

        epochs[static_cast<size_t>(i)] = MatrixXd(nChannels, nTimes);
        for (int ch = 0; ch < nChannels; ++ch) {
            for (int s = 0; s < nTimes; ++s) {
                epochs[static_cast<size_t>(i)](ch, s) = noise(rng);
            }
        }
        // Scale channel 0 by target → power covaries with target
        epochs[static_cast<size_t>(i)].row(0) *= (1.0 + 5.0 * t);
    }
}

//=============================================================================================================

class TestDecodingSpoc : public QObject
{
    Q_OBJECT

private slots:
    void testConstruction();
    void testFitAndIsFitted();
    void testTransformAveragePower();
    void testFitTransform();
    void testCorrelationWithTarget();
    void testNotFittedThrows();
    void testMismatchedSizeThrows();
};

//=============================================================================================================

void TestDecodingSpoc::testConstruction()
{
    DecodingSpoc spoc;
    QVERIFY(!spoc.isFitted());
}

//=============================================================================================================

void TestDecodingSpoc::testFitAndIsFitted()
{
    std::vector<MatrixXd> epochs;
    VectorXd target;
    generateSpocData(epochs, target);

    DecodingSpoc spoc(4);
    spoc.fit(epochs, target);
    QVERIFY(spoc.isFitted());

    QCOMPARE(spoc.filters().rows(), 4);
    QCOMPARE(spoc.filters().cols(), 6);
}

//=============================================================================================================

void TestDecodingSpoc::testTransformAveragePower()
{
    std::vector<MatrixXd> epochs;
    VectorXd target;
    generateSpocData(epochs, target);

    DecodingSpoc spoc(3, DecodingSpoc::TransformMode::AveragePower, true);
    spoc.fit(epochs, target);

    MatrixXd features = spoc.transform(epochs);
    QCOMPARE(features.rows(), static_cast<int>(epochs.size()));
    QCOMPARE(features.cols(), 3);
    QVERIFY(features.allFinite());
}

//=============================================================================================================

void TestDecodingSpoc::testFitTransform()
{
    std::vector<MatrixXd> epochs;
    VectorXd target;
    generateSpocData(epochs, target);

    DecodingSpoc spoc(4);
    MatrixXd features = spoc.fitTransform(epochs, target);
    QVERIFY(spoc.isFitted());
    QCOMPARE(features.rows(), static_cast<int>(epochs.size()));
}

//=============================================================================================================

void TestDecodingSpoc::testCorrelationWithTarget()
{
    std::vector<MatrixXd> epochs;
    VectorXd target;
    generateSpocData(epochs, target, 60, 6, 200);

    DecodingSpoc spoc(4, DecodingSpoc::TransformMode::AveragePower, false);
    spoc.fit(epochs, target);
    MatrixXd features = spoc.transform(epochs);

    // First component should correlate with target
    VectorXd comp = features.col(0);
    double mean_c = comp.mean();
    double mean_t = target.mean();
    double cov = 0, var_c = 0, var_t = 0;
    for (int i = 0; i < comp.size(); ++i) {
        double dc = comp(i) - mean_c;
        double dt = target(i) - mean_t;
        cov += dc * dt;
        var_c += dc * dc;
        var_t += dt * dt;
    }
    double corr = std::abs(cov / std::sqrt(var_c * var_t));

    QVERIFY2(corr > 0.3,
             qPrintable(QString("Correlation %1 too low").arg(corr)));
}

//=============================================================================================================

void TestDecodingSpoc::testNotFittedThrows()
{
    DecodingSpoc spoc;
    std::vector<MatrixXd> epochs = { MatrixXd::Random(6, 100) };

    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
        static_cast<void>(spoc.transform(epochs)));
}

//=============================================================================================================

void TestDecodingSpoc::testMismatchedSizeThrows()
{
    std::vector<MatrixXd> epochs = { MatrixXd::Random(6, 100),
                                     MatrixXd::Random(6, 100) };
    VectorXd target(3);
    target << 0.1, 0.5, 0.9;

    DecodingSpoc spoc;
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
        spoc.fit(epochs, target));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDecodingSpoc)
#include "test_decoding_spoc.moc"
