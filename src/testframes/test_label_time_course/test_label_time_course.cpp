//=============================================================================================================
/**
 * @file     test_label_time_course.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for InvLabelTimeCourse.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_label_time_course.h>
#include <inv/inv_source_estimate.h>
#include <fs/fs_label.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QList>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================

class TestLabelTimeCourse : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testMeanMode();
    void testMeanFlipMode();
    void testPcaFlipMode();
    void testMaxMode();
    void testAutoMode();
    void testOutputDimensions();
    void testMultipleLabels();
    void testEmptyLabel();
    void testAllowEmpty();
    void testNoOverlap();
    void testSignFlip();
    void cleanupTestCase();

private:
    InvSourceEstimate m_stc;
    QList<FsLabel> m_labels;
    int m_nVerts;
    int m_nTimes;
};

//=============================================================================================================

void TestLabelTimeCourse::initTestCase()
{
    m_nVerts = 20;
    m_nTimes = 50;

    // Create synthetic source estimate
    m_stc.data = MatrixXd::Random(m_nVerts, m_nTimes);
    m_stc.vertices = VectorXi::LinSpaced(m_nVerts, 0, m_nVerts - 1);
    m_stc.tmin = 0.0f;
    m_stc.tstep = 0.001f;
    m_stc.times = RowVectorXf::LinSpaced(m_nTimes, 0.0f, 0.049f);

    // Create two labels covering different vertex subsets
    FsLabel label1;
    label1.vertices = VectorXi::LinSpaced(5, 0, 4);  // vertices 0-4
    label1.pos = MatrixX3f::Zero(5, 3);
    label1.values = VectorXd::Ones(5);
    label1.hemi = 0;
    label1.name = "label1";

    FsLabel label2;
    label2.vertices = VectorXi::LinSpaced(8, 10, 17);  // vertices 10-17
    label2.pos = MatrixX3f::Zero(8, 3);
    label2.values = VectorXd::Ones(8);
    label2.hemi = 0;
    label2.name = "label2";

    m_labels.append(label1);
    m_labels.append(label2);
}

//=============================================================================================================

void TestLabelTimeCourse::testMeanMode()
{
    MatrixXd tc = InvLabelTimeCourse::extract(m_stc, m_labels, "mean");
    QCOMPARE(static_cast<int>(tc.rows()), 2);
    QCOMPARE(static_cast<int>(tc.cols()), m_nTimes);

    // Verify mean manually for label 1 (vertices 0-4)
    RowVectorXd expected = m_stc.data.topRows(5).colwise().mean();
    double diff = (tc.row(0) - expected).norm();
    QVERIFY2(diff < 1e-10, "Mean mode doesn't match manual computation.");
}

//=============================================================================================================

void TestLabelTimeCourse::testMeanFlipMode()
{
    MatrixXd tc = InvLabelTimeCourse::extract(m_stc, m_labels, "mean_flip");
    QCOMPARE(static_cast<int>(tc.rows()), 2);
    QCOMPARE(static_cast<int>(tc.cols()), m_nTimes);

    // Output should be finite
    QVERIFY(tc.allFinite());
}

//=============================================================================================================

void TestLabelTimeCourse::testPcaFlipMode()
{
    MatrixXd tc = InvLabelTimeCourse::extract(m_stc, m_labels, "pca_flip");
    QCOMPARE(static_cast<int>(tc.rows()), 2);
    QCOMPARE(static_cast<int>(tc.cols()), m_nTimes);
    QVERIFY(tc.allFinite());
}

//=============================================================================================================

void TestLabelTimeCourse::testMaxMode()
{
    MatrixXd tc = InvLabelTimeCourse::extract(m_stc, m_labels, "max");
    QCOMPARE(static_cast<int>(tc.rows()), 2);
    QCOMPARE(static_cast<int>(tc.cols()), m_nTimes);

    // At each time point, max mode should return the value with the largest absolute value
    for (int t = 0; t < m_nTimes; ++t) {
        double maxAbsVal = 0.0;
        double maxVal = 0.0;
        for (int v = 0; v < 5; ++v) {
            double absVal = std::abs(m_stc.data(v, t));
            if (absVal > maxAbsVal) {
                maxAbsVal = absVal;
                maxVal = m_stc.data(v, t);
            }
        }
        QVERIFY2(std::abs(tc(0, t) - maxVal) < 1e-10,
                 qPrintable(QString("Max mode mismatch at t=%1").arg(t)));
    }
}

//=============================================================================================================

void TestLabelTimeCourse::testAutoMode()
{
    // "auto" should behave like "mean_flip"
    MatrixXd tcAuto = InvLabelTimeCourse::extract(m_stc, m_labels, "auto");
    MatrixXd tcMeanFlip = InvLabelTimeCourse::extract(m_stc, m_labels, "mean_flip");

    double diff = (tcAuto - tcMeanFlip).norm();
    QVERIFY2(diff < 1e-10, "auto mode doesn't match mean_flip.");
}

//=============================================================================================================

void TestLabelTimeCourse::testOutputDimensions()
{
    QList<FsLabel> singleLabel;
    singleLabel.append(m_labels[0]);

    MatrixXd tc = InvLabelTimeCourse::extract(m_stc, singleLabel, "mean");
    QCOMPARE(static_cast<int>(tc.rows()), 1);
    QCOMPARE(static_cast<int>(tc.cols()), m_nTimes);
}

//=============================================================================================================

void TestLabelTimeCourse::testMultipleLabels()
{
    // Add a third label
    QList<FsLabel> labels = m_labels;
    FsLabel label3;
    label3.vertices = VectorXi::LinSpaced(3, 5, 7);
    label3.pos = MatrixX3f::Zero(3, 3);
    label3.values = VectorXd::Ones(3);
    label3.hemi = 0;
    label3.name = "label3";
    labels.append(label3);

    MatrixXd tc = InvLabelTimeCourse::extract(m_stc, labels, "mean");
    QCOMPARE(static_cast<int>(tc.rows()), 3);
}

//=============================================================================================================

void TestLabelTimeCourse::testEmptyLabel()
{
    // Label with vertices not in the STC
    FsLabel emptyLabel;
    emptyLabel.vertices = VectorXi::LinSpaced(3, 100, 102);  // out of range
    emptyLabel.pos = MatrixX3f::Zero(3, 3);
    emptyLabel.values = VectorXd::Ones(3);
    emptyLabel.hemi = 0;
    emptyLabel.name = "empty";

    QList<FsLabel> labels;
    labels.append(emptyLabel);

    // Without allowEmpty, should skip the label → 0 rows
    MatrixXd tc = InvLabelTimeCourse::extract(m_stc, labels, "mean", false);
    QCOMPARE(static_cast<int>(tc.rows()), 0);
}

//=============================================================================================================

void TestLabelTimeCourse::testAllowEmpty()
{
    FsLabel emptyLabel;
    emptyLabel.vertices = VectorXi::LinSpaced(3, 100, 102);
    emptyLabel.pos = MatrixX3f::Zero(3, 3);
    emptyLabel.values = VectorXd::Ones(3);
    emptyLabel.hemi = 0;
    emptyLabel.name = "empty";

    QList<FsLabel> labels;
    labels.append(m_labels[0]);
    labels.append(emptyLabel);

    MatrixXd tc = InvLabelTimeCourse::extract(m_stc, labels, "mean", true);
    QCOMPARE(static_cast<int>(tc.rows()), 2);

    // The empty label row should be all zeros
    QVERIFY(tc.row(1).norm() < 1e-15);
}

//=============================================================================================================

void TestLabelTimeCourse::testNoOverlap()
{
    // Labels don't overlap, so independent computations should hold
    MatrixXd tc = InvLabelTimeCourse::extract(m_stc, m_labels, "mean");

    // Label 1 mean and label 2 mean should be independent
    RowVectorXd mean1 = m_stc.data.block(0, 0, 5, m_nTimes).colwise().mean();
    RowVectorXd mean2 = m_stc.data.block(10, 0, 8, m_nTimes).colwise().mean();

    QVERIFY((tc.row(0) - mean1).norm() < 1e-10);
    QVERIFY((tc.row(1) - mean2).norm() < 1e-10);
}

//=============================================================================================================

void TestLabelTimeCourse::testSignFlip()
{
    // Create data where all vertices have the same sign pattern
    MatrixXd data(5, 10);
    for (int i = 0; i < 5; ++i)
        data.row(i) = RowVectorXd::LinSpaced(10, 1.0, 10.0) * (1.0 + 0.1 * i);

    VectorXd signs = InvLabelTimeCourse::computeSignFlip(data);
    QCOMPARE(static_cast<int>(signs.size()), 5);

    // All signs should be the same (all coherent), either all +1 or all -1
    double firstSign = signs[0];
    for (int i = 1; i < 5; ++i)
        QVERIFY2(signs[i] == firstSign,
                 qPrintable(QString("Sign[%1]=%2 differs from sign[0]=%3 for coherent data.")
                    .arg(i).arg(signs[i]).arg(firstSign)));

    // All values should be exactly +1 or -1
    for (int i = 0; i < 5; ++i)
        QVERIFY(std::abs(signs[i]) == 1.0);
}

//=============================================================================================================

void TestLabelTimeCourse::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestLabelTimeCourse)
#include "test_label_time_course.moc"
