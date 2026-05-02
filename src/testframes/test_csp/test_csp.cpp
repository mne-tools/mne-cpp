//=============================================================================================================
/**
 * @file     test_csp.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for Common Spatial Patterns (CSP) via Skigen.
 */

#include <Skigen/Decomposition>

#include <QtTest>
#include <QObject>
#include <Eigen/Core>
#include <cmath>
#include <vector>

using namespace Eigen;

class TestCsp : public QObject
{
    Q_OBJECT

private:
    std::vector<MatrixXd> generateClassData(int nEpochs, int nChannels, int nTimes,
                                             int activeChannel, double amplitude)
    {
        std::vector<MatrixXd> epochs;
        epochs.reserve(nEpochs);
        for (int e = 0; e < nEpochs; ++e) {
            MatrixXd epoch = MatrixXd::Random(nChannels, nTimes) * 0.1;
            for (int t = 0; t < nTimes; ++t) {
                epoch(activeChannel, t) += amplitude * std::sin(2.0 * M_PI * 10.0 * t / 200.0);
            }
            epochs.push_back(std::move(epoch));
        }
        return epochs;
    }

private slots:
    void testFit()
    {
        const int nCh = 8, nTimes = 100;
        auto class1 = generateClassData(10, nCh, nTimes, 0, 5.0);
        auto class2 = generateClassData(10, nCh, nTimes, 7, 5.0);

        Skigen::CSP<double> csp(4);
        csp.fit(class1, class2);

        QCOMPARE(csp.filters().rows(), static_cast<Index>(4));
        QCOMPARE(csp.filters().cols(), static_cast<Index>(nCh));
        QCOMPARE(csp.eigenvalues().size(), static_cast<Index>(4));
    }

    void testTransform()
    {
        const int nCh = 8, nTimes = 100;
        auto class1 = generateClassData(10, nCh, nTimes, 0, 5.0);
        auto class2 = generateClassData(10, nCh, nTimes, 7, 5.0);

        Skigen::CSP<double> csp(4);
        csp.fit(class1, class2);

        MatrixXd features = csp.transform(class1);
        QCOMPARE(features.rows(), static_cast<Index>(10));
        QCOMPARE(features.cols(), static_cast<Index>(4));
        QVERIFY(features.allFinite());
    }

    void testFitTransform()
    {
        const int nCh = 8, nTimes = 100;
        auto class1 = generateClassData(5, nCh, nTimes, 0, 5.0);
        auto class2 = generateClassData(5, nCh, nTimes, 7, 5.0);

        Skigen::CSP<double> csp(4);
        MatrixXd features = csp.fit_transform(class1, class2);

        QCOMPARE(features.rows(), static_cast<Index>(10));
        QCOMPARE(features.cols(), static_cast<Index>(4));
    }

    void testClassSeparability()
    {
        const int nCh = 6, nTimes = 200;
        auto class1 = generateClassData(20, nCh, nTimes, 0, 10.0);
        auto class2 = generateClassData(20, nCh, nTimes, 5, 10.0);

        Skigen::CSP<double> csp(4);
        csp.fit(class1, class2);

        MatrixXd feat1 = csp.transform(class1);
        MatrixXd feat2 = csp.transform(class2);

        double mean1 = feat1.col(0).mean();
        double mean2 = feat2.col(0).mean();
        QVERIFY2(std::abs(mean1 - mean2) > 0.1,
                 qPrintable(QString("mean1=%1, mean2=%2").arg(mean1).arg(mean2)));
    }

    void testNotFitted()
    {
        Skigen::CSP<double> csp(4);
        std::vector<MatrixXd> epochs{MatrixXd::Random(4, 50)};
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, static_cast<void>(csp.transform(epochs)));
    }

    void testEmptyInput()
    {
        Skigen::CSP<double> csp(4);
        std::vector<MatrixXd> empty;
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, csp.fit(empty, empty));
    }

    void testPatterns()
    {
        const int nCh = 8, nTimes = 100;
        auto class1 = generateClassData(10, nCh, nTimes, 0, 5.0);
        auto class2 = generateClassData(10, nCh, nTimes, 7, 5.0);

        Skigen::CSP<double> csp(4);
        csp.fit(class1, class2);

        // patterns: pinv(filters)^T → (n_components × n_channels)
        QCOMPARE(csp.patterns().rows(), static_cast<Index>(4));
        QCOMPARE(csp.patterns().cols(), static_cast<Index>(nCh));
        QVERIFY(csp.patterns().allFinite());
    }
};

QTEST_GUILESS_MAIN(TestCsp)
#include "test_csp.moc"
