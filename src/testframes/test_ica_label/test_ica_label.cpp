//=============================================================================================================
/**
 * @file     test_ica_label.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for MlIcaLabel (ICA component auto-classification).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <decoding/decoding_ica_label.h>

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
// STD INCLUDES
//=============================================================================================================

#include <cmath>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DECODINGLIB;
using namespace Eigen;

//=============================================================================================================

class TestIcaLabel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testEmptyInput();
    void testEogDetection();
    void testEcgDetection();
    void testBrainDefault();
    void testFindArtifacts();
    void testMaxAbsCorrelation();
    void testMuscleScore();
    void testLabelToString();
    void testNoReferences();
    void testMultipleArtifacts();
    void cleanupTestCase();

private:
    MatrixXd m_sources;
    MatrixXd m_eogRef;
    MatrixXd m_ecgRef;
    double m_sFreq;
};

//=============================================================================================================

void TestIcaLabel::initTestCase()
{
    m_sFreq = 250.0;
    int nSamples = 500;

    std::mt19937 gen(42);
    std::normal_distribution<double> noise(0.0, 0.1);

    // Component 0: brain-like (10 Hz alpha)
    // Component 1: EOG-like (correlated with EOG ref)
    // Component 2: ECG-like (correlated with ECG ref)
    m_sources = MatrixXd(3, nSamples);

    // EOG reference signal
    m_eogRef = MatrixXd(1, nSamples);
    // ECG reference signal
    m_ecgRef = MatrixXd(1, nSamples);

    for (int t = 0; t < nSamples; ++t) {
        double time = static_cast<double>(t) / m_sFreq;
        // Brain component: 10 Hz sine
        m_sources(0, t) = std::sin(2.0 * M_PI * 10.0 * time) + noise(gen);
        // EOG reference
        m_eogRef(0, t) = std::sin(2.0 * M_PI * 0.5 * time) + noise(gen) * 0.1;
        // EOG-like component (highly correlated)
        m_sources(1, t) = m_eogRef(0, t) * 2.0 + noise(gen) * 0.05;
        // ECG reference
        m_ecgRef(0, t) = std::sin(2.0 * M_PI * 1.2 * time) + noise(gen) * 0.1;
        // ECG-like component
        m_sources(2, t) = m_ecgRef(0, t) * 1.5 + noise(gen) * 0.05;
    }
}

//=============================================================================================================

void TestIcaLabel::testEmptyInput()
{
    MatrixXd empty;
    QList<IcaLabelResult> labels = MlIcaLabel::classify(empty, m_eogRef, m_ecgRef, m_sFreq);
    QVERIFY(labels.isEmpty());
}

//=============================================================================================================

void TestIcaLabel::testEogDetection()
{
    QList<IcaLabelResult> labels = MlIcaLabel::classify(m_sources, m_eogRef, m_ecgRef, m_sFreq);
    QCOMPARE(labels.size(), 3);

    // Component 1 should be classified as EOG
    QCOMPARE(labels[1].label, IcaComponentLabel::Eog);
    QVERIFY(labels[1].confidence > 0.3);
}

//=============================================================================================================

void TestIcaLabel::testEcgDetection()
{
    QList<IcaLabelResult> labels = MlIcaLabel::classify(m_sources, m_eogRef, m_ecgRef, m_sFreq);

    // Component 2 should be classified as ECG
    QCOMPARE(labels[2].label, IcaComponentLabel::Ecg);
    QVERIFY(labels[2].confidence > 0.3);
}

//=============================================================================================================

void TestIcaLabel::testBrainDefault()
{
    QList<IcaLabelResult> labels = MlIcaLabel::classify(m_sources, m_eogRef, m_ecgRef, m_sFreq);

    // Component 0 (10 Hz alpha) should be brain
    QCOMPARE(labels[0].label, IcaComponentLabel::Brain);
}

//=============================================================================================================

void TestIcaLabel::testFindArtifacts()
{
    QList<IcaLabelResult> labels = MlIcaLabel::classify(m_sources, m_eogRef, m_ecgRef, m_sFreq);
    QVector<int> artifacts = MlIcaLabel::findArtifactComponents(labels);

    // Should find components 1 (EOG) and 2 (ECG) as artifacts
    QVERIFY(artifacts.contains(1));
    QVERIFY(artifacts.contains(2));
    QVERIFY(!artifacts.contains(0));
}

//=============================================================================================================

void TestIcaLabel::testMaxAbsCorrelation()
{
    VectorXd a(100), b(100);
    for (int i = 0; i < 100; ++i) {
        a[i] = std::sin(2.0 * M_PI * i / 100.0);
        b[i] = a[i] * 2.0 + 0.01;
    }

    MatrixXd ref(1, 100);
    ref.row(0) = b.transpose();

    double corr = MlIcaLabel::maxAbsCorrelation(a, ref);
    QVERIFY2(corr > 0.99, qPrintable(QString("Correlation %1 < 0.99").arg(corr)));
}

//=============================================================================================================

void TestIcaLabel::testMuscleScore()
{
    // Low-frequency signal should have low muscle score
    VectorXd lowFreq(500);
    for (int i = 0; i < 500; ++i)
        lowFreq[i] = std::sin(2.0 * M_PI * 5.0 * i / 250.0);

    double score = MlIcaLabel::muscleScore(lowFreq, 250.0);
    QVERIFY2(score < 0.5, qPrintable(QString("Muscle score %1 >= 0.5 for low freq").arg(score)));
}

//=============================================================================================================

void TestIcaLabel::testLabelToString()
{
    QCOMPARE(IcaLabelResult::labelToString(IcaComponentLabel::Brain), QStringLiteral("brain"));
    QCOMPARE(IcaLabelResult::labelToString(IcaComponentLabel::Eog), QStringLiteral("eog"));
    QCOMPARE(IcaLabelResult::labelToString(IcaComponentLabel::Ecg), QStringLiteral("ecg"));
    QCOMPARE(IcaLabelResult::labelToString(IcaComponentLabel::Muscle), QStringLiteral("muscle"));
    QCOMPARE(IcaLabelResult::labelToString(IcaComponentLabel::Other), QStringLiteral("other"));
}

//=============================================================================================================

void TestIcaLabel::testNoReferences()
{
    MatrixXd emptyRef;
    QList<IcaLabelResult> labels = MlIcaLabel::classify(m_sources, emptyRef, emptyRef, m_sFreq);

    // Without references, all should be brain or other (no EOG/ECG detection possible)
    for (const auto& l : labels) {
        QVERIFY(l.label != IcaComponentLabel::Eog);
        QVERIFY(l.label != IcaComponentLabel::Ecg);
    }
}

//=============================================================================================================

void TestIcaLabel::testMultipleArtifacts()
{
    // Create data with 2 EOG components
    MatrixXd sources(4, 500);
    for (int t = 0; t < 500; ++t) {
        double time = static_cast<double>(t) / m_sFreq;
        sources(0, t) = std::sin(2.0 * M_PI * 10.0 * time);  // brain
        sources(1, t) = m_eogRef(0, t) * 3.0;                  // EOG 1
        sources(2, t) = m_eogRef(0, t) * -2.5;                 // EOG 2 (anti-correlated)
        sources(3, t) = m_ecgRef(0, t) * 2.0;                  // ECG
    }

    QList<IcaLabelResult> labels = MlIcaLabel::classify(sources, m_eogRef, m_ecgRef, m_sFreq);
    QVector<int> artifacts = MlIcaLabel::findArtifactComponents(labels);
    QVERIFY(artifacts.size() >= 2);
}

//=============================================================================================================

void TestIcaLabel::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestIcaLabel)
#include "test_ica_label.moc"
