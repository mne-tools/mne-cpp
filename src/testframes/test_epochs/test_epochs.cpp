//=============================================================================================================
/**
 * @file     test_epochs.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for FiffEpochs (epoch convenience utilities).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_epochs.h>
#include <fiff/fiff_evoked.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================

class TestEpochs : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testFixedLengthBasic();
    void testFixedLengthCount();
    void testFixedLengthOverlap();
    void testFixedLengthDropLast();
    void testFixedLengthKeepLast();
    void testConcatenate();
    void testAverage();
    void testToMatrixList();
    void testTimestamps();
    void testEmptyInput();
    void testInvalidParams();
    void cleanupTestCase();

private:
    MatrixXd m_data;
    double m_sFreq;
    int m_nCh;
    int m_nTimes;
};

//=============================================================================================================

void TestEpochs::initTestCase()
{
    m_nCh = 4;
    m_sFreq = 100.0;
    m_nTimes = 1000;  // 10 seconds

    // Create simple ramp data for easy verification
    m_data = MatrixXd(m_nCh, m_nTimes);
    for (int ch = 0; ch < m_nCh; ++ch)
        m_data.row(ch) = RowVectorXd::LinSpaced(m_nTimes, 0.0, 9.99);
}

//=============================================================================================================

void TestEpochs::testFixedLengthBasic()
{
    auto epochs = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 2.0);

    // 10 seconds / 2 second epochs = 5 epochs
    QCOMPARE(epochs.size(), 5);

    // Each epoch should have 200 samples
    for (const auto& ep : epochs) {
        QCOMPARE(static_cast<int>(ep.data.rows()), m_nCh);
        QCOMPARE(static_cast<int>(ep.data.cols()), 200);
    }
}

//=============================================================================================================

void TestEpochs::testFixedLengthCount()
{
    // 10 seconds, 3 second epochs → 3 full epochs, remainder dropped
    auto epochs = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 3.0);
    QCOMPARE(epochs.size(), 3);
}

//=============================================================================================================

void TestEpochs::testFixedLengthOverlap()
{
    // 10 seconds, 2 second epochs, 1 second overlap → step = 1 second
    // Epochs: 0-2, 1-3, 2-4, 3-5, 4-6, 5-7, 6-8, 7-9, 8-10 = 9 epochs
    auto epochs = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 2.0, 1.0);
    QCOMPARE(epochs.size(), 9);
}

//=============================================================================================================

void TestEpochs::testFixedLengthDropLast()
{
    // 10 seconds, 3 second epochs, drop last (default)
    auto epochsDrop = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 3.0, 0.0, true);
    QCOMPARE(epochsDrop.size(), 3);
}

//=============================================================================================================

void TestEpochs::testFixedLengthKeepLast()
{
    // 10 seconds, 3 second epochs, keep last
    auto epochsKeep = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 3.0, 0.0, false);
    QCOMPARE(epochsKeep.size(), 4);  // 3 full + 1 partial

    // Last epoch should be shorter (100 samples = 1 second)
    QCOMPARE(static_cast<int>(epochsKeep.last().data.cols()), 100);
}

//=============================================================================================================

void TestEpochs::testConcatenate()
{
    auto set1 = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 2.0);
    auto set2 = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 5.0);

    QList<QList<FiffEpochData>> sets;
    sets.append(set1);
    sets.append(set2);

    auto combined = FiffEpochs::concatenateEpochs(sets);
    QCOMPARE(combined.size(), set1.size() + set2.size());
}

//=============================================================================================================

void TestEpochs::testAverage()
{
    // Create epochs with known values
    auto epochs = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 2.0);

    FiffEvoked evoked = FiffEpochs::averageEpochs(epochs, m_sFreq);
    QCOMPARE(static_cast<int>(evoked.data.rows()), m_nCh);
    QCOMPARE(static_cast<int>(evoked.data.cols()), 200);
    QVERIFY(evoked.data.allFinite());

    // Verify evoked metadata
    QCOMPARE(evoked.nave, epochs.size());
    QCOMPARE(evoked.aspect_kind, static_cast<fiff_int_t>(FIFFV_ASPECT_AVERAGE));
    QCOMPARE(evoked.comment, QString("Average"));

    // Verify times vector
    QCOMPARE(static_cast<int>(evoked.times.size()), 200);
    QVERIFY(std::abs(evoked.times(0) - static_cast<float>(epochs[0].tmin)) < 1e-5f);
}

//=============================================================================================================

void TestEpochs::testToMatrixList()
{
    auto epochs = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 2.0);
    auto matrices = FiffEpochs::toMatrixList(epochs);

    QCOMPARE(matrices.size(), epochs.size());
    for (int i = 0; i < matrices.size(); ++i) {
        QCOMPARE(matrices[i].rows(), epochs[i].data.rows());
        QCOMPARE(matrices[i].cols(), epochs[i].data.cols());
    }
}

//=============================================================================================================

void TestEpochs::testTimestamps()
{
    auto epochs = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 2.0);

    // First epoch: 0.0 to ~1.99
    QVERIFY(std::abs(epochs[0].tmin - 0.0) < 1e-10);

    // Second epoch starts at 2.0 seconds
    QVERIFY(std::abs(epochs[1].tmin - 2.0) < 1e-10);

    // Third epoch starts at 4.0 seconds
    QVERIFY(std::abs(epochs[2].tmin - 4.0) < 1e-10);
}

//=============================================================================================================

void TestEpochs::testEmptyInput()
{
    MatrixXd empty;
    auto epochs = FiffEpochs::makeFixedLengthEpochs(empty, m_sFreq, 2.0);
    QVERIFY(epochs.isEmpty());
}

//=============================================================================================================

void TestEpochs::testInvalidParams()
{
    // Zero duration
    auto e1 = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 0.0);
    QVERIFY(e1.isEmpty());

    // Negative sFreq
    auto e2 = FiffEpochs::makeFixedLengthEpochs(m_data, -1.0, 2.0);
    QVERIFY(e2.isEmpty());

    // Overlap >= duration
    auto e3 = FiffEpochs::makeFixedLengthEpochs(m_data, m_sFreq, 2.0, 3.0);
    QVERIFY(e3.isEmpty());
}

//=============================================================================================================

void TestEpochs::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestEpochs)
#include "test_epochs.moc"
