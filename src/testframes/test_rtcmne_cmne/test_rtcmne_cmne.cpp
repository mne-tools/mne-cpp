//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_rtcmne_cmne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     April, 2026
 * @brief    Tests for the CMNE (Contextual MNE) routing inside the rtcmne real-time
 *           inverse plugin. The plugin itself can only be exercised in a full MNE Scan
 *           runtime, so this test focuses on the algorithmic dispatch: it drives the
 *           InvCMNE solver through the same compute() entry-point used by the plugin
 *           and checks it against the cmne reference implementation (make_cmne_reference.py).
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
#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>
#include <QString>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestRtcMneCmne
 *
 * @brief Tests for the CMNE routing in the rtcmne real-time inverse plugin.
 */
class TestRtcMneCmne : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testSettingsRoundTrip();
    void testCmneMatchesReference();

    void cleanupTestCase();
};

//=============================================================================================================
// IMPLEMENTATION
//=============================================================================================================

void TestRtcMneCmne::initTestCase()
{
}

//=============================================================================================================

void TestRtcMneCmne::cleanupTestCase()
{
}

//=============================================================================================================

void TestRtcMneCmne::testSettingsRoundTrip()
{
    InvCMNESettings settings;
    settings.onnxModelPath = QStringLiteral("/tmp/imaginary_checkpoint.onnx");
    settings.lambda2 = 1.0 / 9.0;
    settings.numSources = 16;
    settings.lookBack = 4;
    settings.method = 1;
    settings.looseOriConstraint = 0.2;

    QCOMPARE(settings.onnxModelPath, QStringLiteral("/tmp/imaginary_checkpoint.onnx"));
    QCOMPARE(settings.numSources, 16);
    QCOMPARE(settings.lookBack, 4);
    QCOMPARE(settings.method, 1);
    QVERIFY(std::abs(settings.lambda2 - 1.0 / 9.0) < 1e-12);
    QVERIFY(std::abs(settings.looseOriConstraint - 0.2) < 1e-12);
}

//=============================================================================================================

void TestRtcMneCmne::testCmneMatchesReference()
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
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestRtcMneCmne)
#include "test_rtcmne_cmne.moc"
