//=============================================================================================================
/**
 * @file     test_decoding_ssd.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for DecodingSsd (mne.decoding.SSD equivalent).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <decoding/decoding_ssd.h>

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

#include <cmath>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DECODINGLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Helper: generate data with a 10 Hz oscillation on channel 0.
 */
static MatrixXd generateSsdData(int nChannels = 6, double sfreq = 256.0,
                                double durationSec = 4.0)
{
    const int nTimes = static_cast<int>(sfreq * durationSec);
    MatrixXd data(nChannels, nTimes);

    std::mt19937 rng(99);
    std::normal_distribution<double> noise(0.0, 1.0);

    for (int ch = 0; ch < nChannels; ++ch) {
        for (int t = 0; t < nTimes; ++t) {
            data(ch, t) = noise(rng);
        }
    }

    // Inject stronger 10 Hz signal on channel 0
    for (int t = 0; t < nTimes; ++t) {
        double time = static_cast<double>(t) / sfreq;
        data(0, t) += 10.0 * std::sin(2.0 * M_PI * 10.0 * time);
    }

    return data;
}

//=============================================================================================================

class TestDecodingSsd : public QObject
{
    Q_OBJECT

private slots:
    void testConstruction();
    void testFitAndIsFitted();
    void testTransform();
    void testFitTransform();
    void testApply();
    void testSignalEnhancement();
    void testNotFittedThrows();
    void testInvalidBandsThrows();
};

//=============================================================================================================

void TestDecodingSsd::testConstruction()
{
    DecodingSsd ssd;
    QVERIFY(!ssd.isFitted());

    DecodingSsd ssd2(4, 0.1);
    QVERIFY(!ssd2.isFitted());
}

//=============================================================================================================

void TestDecodingSsd::testFitAndIsFitted()
{
    MatrixXd data = generateSsdData();

    DecodingSsd ssd(4);
    ssd.fit(data, 256.0, 8.0, 12.0, 6.0, 14.0);
    QVERIFY(ssd.isFitted());

    QCOMPARE(ssd.filters().rows(), 4);
    QCOMPARE(ssd.filters().cols(), 6);
    QCOMPARE(ssd.patterns().rows(), 6);   // n_channels
    QCOMPARE(ssd.patterns().cols(), 4);   // n_components
    QCOMPARE(ssd.eigenvalues().size(), 4);
}

//=============================================================================================================

void TestDecodingSsd::testTransform()
{
    MatrixXd data = generateSsdData();

    DecodingSsd ssd(3);
    ssd.fit(data, 256.0, 8.0, 12.0, 6.0, 14.0);

    MatrixXd result = ssd.transform(data);
    QCOMPARE(result.rows(), 3);
    QCOMPARE(result.cols(), data.cols());
}

//=============================================================================================================

void TestDecodingSsd::testFitTransform()
{
    MatrixXd data = generateSsdData();

    DecodingSsd ssd(4);
    MatrixXd result = ssd.fitTransform(data, 256.0, 8.0, 12.0, 6.0, 14.0);
    QVERIFY(ssd.isFitted());
    QCOMPARE(result.rows(), 4);
}

//=============================================================================================================

void TestDecodingSsd::testApply()
{
    MatrixXd data = generateSsdData();

    DecodingSsd ssd(3);
    ssd.fit(data, 256.0, 8.0, 12.0, 6.0, 14.0);

    MatrixXd denoised = ssd.apply(data);
    QCOMPARE(denoised.rows(), data.rows());
    QCOMPARE(denoised.cols(), data.cols());
}

//=============================================================================================================

void TestDecodingSsd::testSignalEnhancement()
{
    MatrixXd data = generateSsdData(6, 256.0, 4.0);

    DecodingSsd ssd(2);
    ssd.fit(data, 256.0, 8.0, 12.0, 6.0, 14.0);

    // Top eigenvalue should indicate signal enhancement
    double topEval = ssd.eigenvalues()(0);
    QVERIFY2(topEval > 0.5,
             qPrintable(QString("Top eigenvalue %1 too low").arg(topEval)));
}

//=============================================================================================================

void TestDecodingSsd::testNotFittedThrows()
{
    DecodingSsd ssd;
    MatrixXd data = MatrixXd::Random(6, 500);

    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
        static_cast<void>(ssd.transform(data)));
    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
        static_cast<void>(ssd.apply(data)));
    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
        static_cast<void>(ssd.filters()));
}

//=============================================================================================================

void TestDecodingSsd::testInvalidBandsThrows()
{
    MatrixXd data = generateSsdData();
    DecodingSsd ssd;

    // noise_low > signal_low → invalid
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
        ssd.fit(data, 256.0, 8.0, 12.0, 9.0, 14.0));

    // signal_high > noise_high → invalid
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
        ssd.fit(data, 256.0, 8.0, 15.0, 6.0, 14.0));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDecodingSsd)
#include "test_decoding_ssd.moc"
