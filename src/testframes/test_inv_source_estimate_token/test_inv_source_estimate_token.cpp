//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_inv_source_estimate_token.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Round-trips every InvSourceEstimate layer through tokenize / fromTokens.
 *
 * The oracle is the estimate itself: every field the token vocabulary can carry
 * must come back unchanged (up to float32 rounding of values), including every
 * method, source-space, orientation and connectivity-measure label, windows
 * that start at time zero, and the time axis of a sub-sampled estimate.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_source_estimate.h>
#include <inv/inv_source_estimate_token.h>

#include <string>
#include <vector>

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

namespace
{

InvSourceEstimate makeEstimate()
{
    MatrixXd data(6, 5);
    for (int s = 0; s < data.rows(); ++s)
        for (int t = 0; t < data.cols(); ++t)
            data(s, t) = 1e-9 * (s + 1) * (t - 2);
    VectorXi verts(6);
    verts << 3, 17, 42, 101, 250, 999;
    InvSourceEstimate est(data, verts, -0.05f, 0.002f);
    est.method = InvEstimateMethod::dSPM;
    est.sourceSpaceType = InvSourceSpaceType::Volume;
    est.orientationType = InvOrientationType::Loose;
    est.positions = MatrixX3f(6, 3);
    for (int s = 0; s < 6; ++s)
        est.positions.row(s) = RowVector3f(0.01f * s, -0.02f * s, 0.03f + 0.001f * s);

    // Windows starting at zero are the common case for evoked responses.
    InvSourceCoupling grp;
    grp.gridIndices = {1, 4, 5};
    grp.moments = {Vector3d(1, 0, 0), Vector3d(0, 1, 0), Vector3d(0.6, 0, 0.8)};
    grp.correlations = MatrixXd(3, 3);
    grp.correlations << 1.0, 0.5, -0.25, 0.5, 1.0, 0.75, -0.25, 0.75, 1.0;
    grp.tmin = 0.0f;
    grp.tmax = 0.1f;
    est.couplings.push_back(grp);
    InvSourceCoupling grp2 = grp;
    grp2.gridIndices = {0, 2, 3};
    grp2.tmin = 0.02f;
    grp2.tmax = 0.04f;
    est.couplings.push_back(grp2);

    InvFocalDipole dip;
    dip.position = Vector3f(0.01f, 0.02f, 0.06f);
    dip.moment = Vector3f(1e-8f, -2e-8f, 3e-8f);
    dip.gridIndex = 4;
    dip.goodness = 0.93f;
    dip.khi2 = 123.5f;
    dip.nfree = 300;
    dip.valid = true;
    dip.tmin = 0.0f;
    dip.tmax = 0.01f;
    est.focalDipoles.push_back(dip);

    InvConnectivity conn;
    conn.measure = "granger";
    conn.directed = true;
    conn.fmin = 8.0f;
    conn.fmax = 12.0f;
    conn.tmin = 0.0f;
    conn.tmax = 0.5f;
    conn.matrix = MatrixXd(2, 2);
    conn.matrix << 0.0, 0.3, 0.1, 0.0;
    est.connectivity.push_back(conn);
    return est;
}

} // namespace

//=============================================================================================================
/**
 * Round-trips InvSourceEstimate layers through the token vocabulary.
 */
class TestInvSourceEstimateToken : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsAllLayers();
    void roundTripsLabels_data();
    void roundTripsLabels();
    void roundTripsMeasures();
    void subSamplingKeepsTimeAxis();
};

//=============================================================================================================

void TestInvSourceEstimateToken::roundTripsAllLayers()
{
    const InvSourceEstimate est = makeEstimate();
    const InvSourceEstimate back = fromTokens(tokenize(est));

    QCOMPARE(back.method, est.method);
    QCOMPARE(back.sourceSpaceType, est.sourceSpaceType);
    QCOMPARE(back.orientationType, est.orientationType);
    QCOMPARE(back.vertices, est.vertices);
    QVERIFY((back.data - est.data).cwiseAbs().maxCoeff() < 1e-7 * est.data.cwiseAbs().maxCoeff());
    QCOMPARE(back.tmin, est.tmin);
    QCOMPARE(back.tstep, est.tstep);
    QCOMPARE(back.times.size(), est.times.size());
    QVERIFY((back.times - est.times).cwiseAbs().maxCoeff() < 1e-6f);
    QCOMPARE(back.positions, est.positions);

    QCOMPARE(back.couplings.size(), est.couplings.size());
    for (size_t g = 0; g < est.couplings.size(); ++g) {
        const InvSourceCoupling& a = est.couplings[g];
        const InvSourceCoupling& b = back.couplings[g];
        QCOMPARE(b.tmin, a.tmin);
        QCOMPARE(b.tmax, a.tmax);
        QCOMPARE(b.gridIndices, a.gridIndices);
        QCOMPARE(b.moments.size(), a.moments.size());
        for (size_t k = 0; k < a.moments.size(); ++k)
            QVERIFY((b.moments[k] - a.moments[k]).norm() < 1e-7);
        QVERIFY((b.correlations - a.correlations).cwiseAbs().maxCoeff() < 1e-7);
    }

    QCOMPARE(back.focalDipoles.size(), size_t(1));
    const InvFocalDipole& d = back.focalDipoles[0];
    const InvFocalDipole& e = est.focalDipoles[0];
    QCOMPARE(d.position, e.position);
    QCOMPARE(d.moment, e.moment);
    QCOMPARE(d.gridIndex, e.gridIndex);
    QCOMPARE(d.goodness, e.goodness);
    QCOMPARE(d.khi2, e.khi2);
    QCOMPARE(d.nfree, e.nfree);
    QCOMPARE(d.valid, e.valid);
    QCOMPARE(d.tmin, e.tmin);
    QCOMPARE(d.tmax, e.tmax);

    QCOMPARE(back.connectivity.size(), size_t(1));
    const InvConnectivity& c = back.connectivity[0];
    const InvConnectivity& f = est.connectivity[0];
    QCOMPARE(c.measure, f.measure);
    QCOMPARE(c.directed, f.directed);
    QCOMPARE(c.fmin, f.fmin);
    QCOMPARE(c.fmax, f.fmax);
    QCOMPARE(c.tmin, f.tmin);
    QCOMPARE(c.tmax, f.tmax);
    QVERIFY((c.matrix - f.matrix).cwiseAbs().maxCoeff() < 1e-7);
}

//=============================================================================================================

void TestInvSourceEstimateToken::roundTripsLabels_data()
{
    QTest::addColumn<int>("method");
    QTest::addColumn<int>("space");
    QTest::addColumn<int>("orient");
    // Every value of each enum, cycled together.
    for (int m = 0; m <= static_cast<int>(InvEstimateMethod::PwlRapMusic); ++m)
        QTest::addRow("method %d", m) << m << (m % 5) << (m % 4);
}

void TestInvSourceEstimateToken::roundTripsLabels()
{
    QFETCH(int, method);
    QFETCH(int, space);
    QFETCH(int, orient);
    InvSourceEstimate est;
    est.method = static_cast<InvEstimateMethod>(method);
    est.sourceSpaceType = static_cast<InvSourceSpaceType>(space);
    est.orientationType = static_cast<InvOrientationType>(orient);
    const InvSourceEstimate back = fromTokens(tokenize(est));
    QCOMPARE(back.method, est.method);
    QCOMPARE(back.sourceSpaceType, est.sourceSpaceType);
    QCOMPARE(back.orientationType, est.orientationType);
}

//=============================================================================================================

void TestInvSourceEstimateToken::roundTripsMeasures()
{
    const std::vector<std::string> measures{"coh", "imcoh", "plv", "pli", "wpli", "granger", "pdc", "dtf", "correlation", "crosscorrelation"};
    InvSourceEstimate est;
    for (const std::string& m : measures) {
        InvConnectivity conn;
        conn.measure = m;
        conn.matrix = MatrixXd::Identity(2, 2);
        est.connectivity.push_back(conn);
    }
    InvConnectivity other;
    other.measure = "envelope";
    other.matrix = MatrixXd::Identity(2, 2);
    est.connectivity.push_back(other);

    const InvSourceEstimate back = fromTokens(tokenize(est));
    QCOMPARE(back.connectivity.size(), est.connectivity.size());
    for (size_t k = 0; k < measures.size(); ++k)
        QCOMPARE(back.connectivity[k].measure, measures[k]);
    // Documented: names outside the vocabulary are not preserved.
    QCOMPARE(back.connectivity.back().measure, std::string());
}

//=============================================================================================================

void TestInvSourceEstimateToken::subSamplingKeepsTimeAxis()
{
    MatrixXd data(10, 9);
    for (int s = 0; s < 10; ++s)
        for (int t = 0; t < 9; ++t)
            data(s, t) = s * 100 + t;
    VectorXi verts = VectorXi::LinSpaced(10, 0, 90);
    InvSourceEstimate est(data, verts, 0.1f, 0.01f);

    InvTokenizeOptions opts;
    opts.maxSources = 5;    // every 2nd source
    opts.maxTimePoints = 3; // every 3rd sample
    const InvSourceEstimate back = fromTokens(tokenize(est, opts));

    QCOMPARE(back.data.rows(), 5);
    QCOMPARE(back.data.cols(), 3);
    for (int s = 0; s < 5; ++s) {
        QCOMPARE(back.vertices[s], verts[2 * s]);
        for (int t = 0; t < 3; ++t)
            QCOMPARE(back.data(s, t), data(2 * s, 3 * t));
    }
    // Each kept sample must keep its original time.
    QCOMPARE(back.times.size(), 3);
    for (int t = 0; t < 3; ++t)
        QVERIFY2(std::abs(back.times[t] - est.times[3 * t]) < 1e-6f,
                 qPrintable(QStringLiteral("sample %1 at %2 s, expected %3 s").arg(t).arg(back.times[t]).arg(est.times[3 * t])));

    // The limits are maxima also when they do not divide the size: 10 of 421 samples, 4 of 10 sources.
    InvSourceEstimate wide(MatrixXd::Ones(10, 421), verts, 0.0f, 0.001f);
    opts.maxSources = 4;
    opts.maxTimePoints = 10;
    const InvSourceEstimate capped = fromTokens(tokenize(wide, opts));
    QCOMPARE(capped.data.rows(), 4);
    QCOMPARE(capped.data.cols(), 10);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvSourceEstimateToken)
#include "test_inv_source_estimate_token.moc"
