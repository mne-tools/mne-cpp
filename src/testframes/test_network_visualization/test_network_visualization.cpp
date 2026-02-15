//=============================================================================================================
/**
 * @file     test_network_visualization.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Tests for the ex_brain_view connectivity network visualization.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <connectivity/network/network.h>
#include <connectivity/network/networknode.h>
#include <connectivity/network/networkedge.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace CONNECTIVITYLIB;
using namespace UTILSLIB;

//=============================================================================================================
// HELPERS
//=============================================================================================================

/**
 * Build a synthetic fully-connected network with nNodes nodes
 * arranged on a circle in the XY plane at z=0, with edge weights
 * proportional to the reciprocal of Euclidean distance.
 */
static Network buildTestNetwork(int nNodes, const QString &method = "TestMethod")
{
    Network net;
    net.setConnectivityMethod(method);

    // Create nodes on a circle (radius 50 mm, simulating brain scale)
    const float radius = 0.05f;  // 50 mm in metres
    QList<QSharedPointer<NetworkNode>> nodes;
    for (int i = 0; i < nNodes; ++i) {
        float angle = 2.0f * M_PI * i / nNodes;
        RowVectorXf vert(3);
        vert << radius * std::cos(angle), radius * std::sin(angle), 0.0f;
        auto node = QSharedPointer<NetworkNode>::create(static_cast<qint16>(i), vert);
        nodes.append(node);
        net.append(node);
    }

    // Create edges between all pairs (fully connected, upper triangle)
    for (int i = 0; i < nNodes; ++i) {
        for (int j = i + 1; j < nNodes; ++j) {
            const RowVectorXf &pi = nodes[i]->getVert();
            const RowVectorXf &pj = nodes[j]->getVert();
            float dist = (pi - pj).norm();

            // Weight = 1/distance (normalised so max edge ≈ 1.0)
            double weight = static_cast<double>(1.0f / dist);

            MatrixXd matWeight(1, 1);
            matWeight(0, 0) = weight;

            auto edge = QSharedPointer<NetworkEdge>::create(i, j, matWeight);
            net.append(edge);
            nodes[i]->append(edge);
            nodes[j]->append(edge);
        }
    }

    return net;
}

/**
 * Build a simple 3-node linear chain:  0 -- 1 -- 2
 * with explicit weights.
 */
static Network buildLinearNetwork(double w01, double w12)
{
    Network net;
    net.setConnectivityMethod("Linear");

    RowVectorXf v0(3); v0 << -0.05f, 0.0f, 0.0f;
    RowVectorXf v1(3); v1 <<  0.0f,  0.0f, 0.0f;
    RowVectorXf v2(3); v2 <<  0.05f, 0.0f, 0.0f;

    auto n0 = QSharedPointer<NetworkNode>::create(0, v0);
    auto n1 = QSharedPointer<NetworkNode>::create(1, v1);
    auto n2 = QSharedPointer<NetworkNode>::create(2, v2);

    net.append(n0);
    net.append(n1);
    net.append(n2);

    MatrixXd mw01(1, 1); mw01(0, 0) = w01;
    auto e01 = QSharedPointer<NetworkEdge>::create(0, 1, mw01);
    net.append(e01);
    n0->append(e01);
    n1->append(e01);

    MatrixXd mw12(1, 1); mw12(0, 0) = w12;
    auto e12 = QSharedPointer<NetworkEdge>::create(1, 2, mw12);
    net.append(e12);
    n1->append(e12);
    n2->append(e12);

    return net;
}

//=============================================================================================================
/**
 * DECLARE CLASS TestNetworkVisualization
 *
 * @brief Tests for the connectivity network visualization pipeline in ex_brain_view.
 */
class TestNetworkVisualization : public QObject
{
    Q_OBJECT

public:
    TestNetworkVisualization();

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ── Network construction tests ─────────────────────────────────────
    void testNetworkConstruction_data();
    void testNetworkConstruction();

    void testNodePositions();
    void testEdgeWeights();
    void testFullConnectivityMatrix();

    // ── Threshold tests ────────────────────────────────────────────────
    void testThresholdZero();
    void testThresholdMax();
    void testThresholdProgressive();
    void testThresholdDegreeUpdate();

    // ── Visualization info ─────────────────────────────────────────────
    void testVisualizationInfo();

    // ── Linear network topology ────────────────────────────────────────
    void testLinearNetworkDegrees();
    void testLinearNetworkEdgeIds();

    // ── Edge cases ─────────────────────────────────────────────────────
    void testEmptyNetwork();
    void testSingleNode();
    void testTwoNodes();

    // ── Min/Max metrics ────────────────────────────────────────────────
    void testMinMaxWeights();
    void testMinMaxDegrees();

private:
    // Reused across tests
    Network m_fullNet4;     // 4-node fully connected
    Network m_fullNet10;    // 10-node fully connected
    Network m_linearNet;    // 3-node chain
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TestNetworkVisualization::TestNetworkVisualization()
{
}

void TestNetworkVisualization::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    m_fullNet4  = buildTestNetwork(4);
    m_fullNet10 = buildTestNetwork(10);
    m_linearNet = buildLinearNetwork(0.8, 0.3);

    qDebug() << "TestNetworkVisualization::initTestCase - Networks built";
    qDebug() << "  4-node:  " << m_fullNet4.getNodes().size()  << "nodes";
    qDebug() << "  10-node: " << m_fullNet10.getNodes().size() << "nodes";
    qDebug() << "  linear:  " << m_linearNet.getNodes().size() << "nodes";
}

void TestNetworkVisualization::cleanupTestCase()
{
    qDebug() << "TestNetworkVisualization::cleanupTestCase";
}

//=============================================================================================================
// Network construction
//=============================================================================================================

void TestNetworkVisualization::testNetworkConstruction_data()
{
    QTest::addColumn<int>("numNodes");
    QTest::addColumn<int>("expectedEdges");

    // n*(n-1)/2 edges for fully connected
    QTest::newRow("3 nodes")  << 3  << 3;
    QTest::newRow("4 nodes")  << 4  << 6;
    QTest::newRow("5 nodes")  << 5  << 10;
    QTest::newRow("10 nodes") << 10 << 45;
    QTest::newRow("20 nodes") << 20 << 190;
}

void TestNetworkVisualization::testNetworkConstruction()
{
    QFETCH(int, numNodes);
    QFETCH(int, expectedEdges);

    Network net = buildTestNetwork(numNodes);

    QVERIFY(!net.isEmpty());
    QCOMPARE(net.getNodes().size(), numNodes);
    QCOMPARE(net.getFullConnectivityMatrix(false).rows(), numNodes);
    QCOMPARE(net.getFullConnectivityMatrix(false).cols(), numNodes);

    // Count total edges from the connectivity matrix upper triangle
    MatrixXd connMat = net.getFullConnectivityMatrix(false);
    int edgeCount = 0;
    for (int i = 0; i < connMat.rows(); ++i) {
        for (int j = i + 1; j < connMat.cols(); ++j) {
            if (connMat(i, j) > 0) edgeCount++;
        }
    }
    QCOMPARE(edgeCount, expectedEdges);
}

//=============================================================================================================

void TestNetworkVisualization::testNodePositions()
{
    const auto &nodes = m_fullNet4.getNodes();
    QCOMPARE(nodes.size(), 4);

    // Nodes are on a circle of radius 0.05m
    for (int i = 0; i < nodes.size(); ++i) {
        const RowVectorXf &vert = nodes[i]->getVert();
        QCOMPARE(vert.size(), 3);

        float r = std::sqrt(vert(0) * vert(0) + vert(1) * vert(1) + vert(2) * vert(2));
        QVERIFY2(std::abs(r - 0.05f) < 1e-5f,
                 qPrintable(QString("Node %1 radius %2 != 0.05").arg(i).arg(r)));
    }

    // z should be 0 for all nodes
    for (int i = 0; i < nodes.size(); ++i) {
        QCOMPARE(nodes[i]->getVert()(2), 0.0f);
    }
}

//=============================================================================================================

void TestNetworkVisualization::testEdgeWeights()
{
    // In our test network, weights = 1/distance, all edges should have positive weights
    MatrixXd connMat = m_fullNet4.getFullConnectivityMatrix(false);

    for (int i = 0; i < connMat.rows(); ++i) {
        for (int j = i + 1; j < connMat.cols(); ++j) {
            QVERIFY2(connMat(i, j) > 0.0,
                     qPrintable(QString("Edge (%1,%2) weight %3 should be > 0")
                                .arg(i).arg(j).arg(connMat(i, j))));
        }
    }
}

//=============================================================================================================

void TestNetworkVisualization::testFullConnectivityMatrix()
{
    // Mirrored version should be symmetric
    MatrixXd mirrored = m_fullNet10.getFullConnectivityMatrix(true);
    QCOMPARE(mirrored.rows(), 10);
    QCOMPARE(mirrored.cols(), 10);

    for (int i = 0; i < mirrored.rows(); ++i) {
        for (int j = 0; j < mirrored.cols(); ++j) {
            QVERIFY2(std::abs(mirrored(i, j) - mirrored(j, i)) < 1e-10,
                     qPrintable(QString("Asymmetric at (%1,%2): %3 vs %4")
                                .arg(i).arg(j).arg(mirrored(i, j)).arg(mirrored(j, i))));
        }
    }

    // Diagonal should be 0
    for (int i = 0; i < mirrored.rows(); ++i) {
        QCOMPARE(mirrored(i, i), 0.0);
    }
}

//=============================================================================================================
// Threshold tests
//=============================================================================================================

void TestNetworkVisualization::testThresholdZero()
{
    Network net = buildTestNetwork(5);
    net.setThreshold(0.0);

    // At threshold 0, all edges should be active
    const auto &edges = net.getThresholdedEdges();
    int activeCount = 0;
    for (const auto &edge : edges) {
        if (edge->isActive()) activeCount++;
    }

    // 5 nodes -> 10 edges
    QCOMPARE(activeCount, 10);
}

//=============================================================================================================

void TestNetworkVisualization::testThresholdMax()
{
    // Threshold operates in absolute weight units (|weight| >= threshold).
    // Our test weights are ~10–14 (1/distance with radius 0.05m), so we need
    // a threshold above the maximum weight to filter everything out.
    Network net = buildTestNetwork(5);

    // First, find the max weight
    QPair<double, double> minMax = net.getMinMaxFullWeights();
    qDebug() << "  Weight range:" << minMax.first << "–" << minMax.second;

    // Set threshold above max weight -> no edges should survive
    net.setThreshold(minMax.second + 1.0);
    const auto &edges = net.getThresholdedEdges();
    QCOMPARE(edges.size(), 0);

    // Set threshold between min and max -> some edges filtered
    double midThresh = (minMax.first + minMax.second) / 2.0;
    net.setThreshold(midThresh);
    const auto &edgesMid = net.getThresholdedEdges();
    int activeMid = edgesMid.size();
    qDebug() << "  Mid threshold" << midThresh << "-> active:" << activeMid;
    QVERIFY2(activeMid > 0 && activeMid < 10,
             qPrintable(QString("Expected partial filtering, got %1 of 10").arg(activeMid)));
}

//=============================================================================================================

void TestNetworkVisualization::testThresholdProgressive()
{
    Network net = buildTestNetwork(6);  // 15 edges

    // Threshold operates in absolute weight units; sweep from 0 to max+epsilon
    QPair<double, double> minMax = net.getMinMaxFullWeights();
    double step = (minMax.second - minMax.first) / 10.0;

    int prevActive = 15;  // Start: all must be active at threshold 0
    for (int s = 0; s <= 10; ++s) {
        double t = minMax.first + step * s;
        net.setThreshold(t);
        int activeCount = net.getThresholdedEdges().size();

        qDebug() << "  Threshold" << t << "-> active:" << activeCount;

        // Active count should be monotonically non-increasing as threshold rises
        QVERIFY2(activeCount <= prevActive,
                 qPrintable(QString("Active edges increased from %1 to %2 at threshold %3")
                            .arg(prevActive).arg(activeCount).arg(t)));
        prevActive = activeCount;
    }

    // At threshold = max weight, at most a few edges with exactly max weight remain
    net.setThreshold(minMax.second);
    QVERIFY(net.getThresholdedEdges().size() <= 15);

    // Above max weight, no edges remain
    net.setThreshold(minMax.second + 1.0);
    QCOMPARE(net.getThresholdedEdges().size(), 0);
}

//=============================================================================================================

void TestNetworkVisualization::testThresholdDegreeUpdate()
{
    Network net = buildTestNetwork(4);

    // At threshold 0, all nodes should have degree 3 (connected to all others)
    net.setThreshold(0.0);
    const auto &nodes = net.getNodes();
    for (int i = 0; i < nodes.size(); ++i) {
        QCOMPARE(nodes[i]->getThresholdedDegree(), static_cast<qint16>(3));
    }

    // Use a threshold between min and max weight to filter some edges
    QPair<double, double> minMax = net.getMinMaxFullWeights();
    double midThresh = (minMax.first + minMax.second) / 2.0;
    net.setThreshold(midThresh);

    bool anyDecreased = false;
    for (int i = 0; i < nodes.size(); ++i) {
        qDebug() << "  Node" << i << "degree:" << nodes[i]->getThresholdedDegree();
        if (nodes[i]->getThresholdedDegree() < 3) {
            anyDecreased = true;
        }
    }
    QVERIFY2(anyDecreased,
             qPrintable(QString("At threshold %1 (mid of %2–%3), some degrees should decrease")
                        .arg(midThresh).arg(minMax.first).arg(minMax.second)));
}

//=============================================================================================================
// Visualization info
//=============================================================================================================

void TestNetworkVisualization::testVisualizationInfo()
{
    Network net = buildTestNetwork(4);

    // Default visualization info
    VisualizationInfo info = net.getVisualizationInfo();
    QVERIFY(!info.sMethod.isEmpty());
    QVERIFY(!info.sColormap.isEmpty());

    // Set custom visualization info
    VisualizationInfo customInfo;
    customInfo.sMethod = "Color";
    customInfo.sColormap = "Hot";
    customInfo.colNodes = Vector4i(0, 255, 0, 255);
    customInfo.colEdges = Vector4i(0, 0, 255, 128);

    net.setVisualizationInfo(customInfo);

    VisualizationInfo retrieved = net.getVisualizationInfo();
    QCOMPARE(retrieved.sMethod, QString("Color"));
    QCOMPARE(retrieved.sColormap, QString("Hot"));
    QCOMPARE(retrieved.colNodes, Vector4i(0, 255, 0, 255));
    QCOMPARE(retrieved.colEdges, Vector4i(0, 0, 255, 128));
}

//=============================================================================================================
// Linear network topology
//=============================================================================================================

void TestNetworkVisualization::testLinearNetworkDegrees()
{
    // Linear chain: 0--1--2
    // Node 0: degree 1 (connected to 1)
    // Node 1: degree 2 (connected to 0 and 2)
    // Node 2: degree 1 (connected to 1)
    m_linearNet.setThreshold(0.0);
    const auto &nodes = m_linearNet.getNodes();

    QCOMPARE(nodes[0]->getThresholdedDegree(), static_cast<qint16>(1));
    QCOMPARE(nodes[1]->getThresholdedDegree(), static_cast<qint16>(2));
    QCOMPARE(nodes[2]->getThresholdedDegree(), static_cast<qint16>(1));
}

//=============================================================================================================

void TestNetworkVisualization::testLinearNetworkEdgeIds()
{
    m_linearNet.setThreshold(0.0);
    const auto &edges = m_linearNet.getThresholdedEdges();

    QCOMPARE(edges.size(), 2);

    // Verify edge endpoint IDs
    bool foundEdge01 = false, foundEdge12 = false;
    for (const auto &edge : edges) {
        int s = edge->getStartNodeID();
        int e = edge->getEndNodeID();
        if ((s == 0 && e == 1) || (s == 1 && e == 0)) foundEdge01 = true;
        if ((s == 1 && e == 2) || (s == 2 && e == 1)) foundEdge12 = true;
    }

    QVERIFY2(foundEdge01, "Edge 0-1 should exist");
    QVERIFY2(foundEdge12, "Edge 1-2 should exist");
}

//=============================================================================================================
// Edge cases
//=============================================================================================================

void TestNetworkVisualization::testEmptyNetwork()
{
    Network net;
    QVERIFY(net.isEmpty());
    QCOMPARE(net.getNodes().size(), 0);
}

//=============================================================================================================

void TestNetworkVisualization::testSingleNode()
{
    Network net;
    RowVectorXf v(3);
    v << 0.0f, 0.0f, 0.0f;
    net.append(QSharedPointer<NetworkNode>::create(0, v));

    // Network::isEmpty() requires BOTH nodes and edges to be non-empty;
    // a single node with no edges is considered "empty" by the library.
    QVERIFY(net.isEmpty());
    QCOMPARE(net.getNodes().size(), 1);

    // Single node has no edges, so degree = 0
    QCOMPARE(net.getNodes()[0]->getThresholdedDegree(), static_cast<qint16>(0));
}

//=============================================================================================================

void TestNetworkVisualization::testTwoNodes()
{
    Network net;
    RowVectorXf v0(3); v0 << 0.0f, 0.0f, 0.0f;
    RowVectorXf v1(3); v1 << 0.1f, 0.0f, 0.0f;

    auto n0 = QSharedPointer<NetworkNode>::create(0, v0);
    auto n1 = QSharedPointer<NetworkNode>::create(1, v1);
    net.append(n0);
    net.append(n1);

    MatrixXd mw(1, 1);
    mw(0, 0) = 0.75;
    auto edge = QSharedPointer<NetworkEdge>::create(0, 1, mw);
    net.append(edge);
    n0->append(edge);
    n1->append(edge);

    QCOMPARE(net.getNodes().size(), 2);

    net.setThreshold(0.0);
    QCOMPARE(net.getNodes()[0]->getThresholdedDegree(), static_cast<qint16>(1));
    QCOMPARE(net.getNodes()[1]->getThresholdedDegree(), static_cast<qint16>(1));

    // Edge weight should match
    QVERIFY(std::abs(edge->getWeight() - 0.75) < 1e-10);
}

//=============================================================================================================
// Min/Max metrics
//=============================================================================================================

void TestNetworkVisualization::testMinMaxWeights()
{
    Network net = buildTestNetwork(4);
    net.setThreshold(0.0);

    QPair<double, double> minMaxFull = net.getMinMaxFullWeights();
    QPair<double, double> minMaxThresh = net.getMinMaxThresholdedWeights();

    qDebug() << "  Full weights: min =" << minMaxFull.first << " max =" << minMaxFull.second;
    qDebug() << "  Thresholded weights: min =" << minMaxThresh.first << " max =" << minMaxThresh.second;

    // Min should be less than max
    QVERIFY(minMaxFull.first <= minMaxFull.second);
    QVERIFY(minMaxThresh.first <= minMaxThresh.second);

    // Both min and max should be positive (our weights are 1/distance)
    QVERIFY(minMaxFull.first > 0.0);
    QVERIFY(minMaxFull.second > 0.0);

    // Network::getMinMaxThresholdedWeights() sets .first = threshold value,
    // .second = full max weight. At threshold 0, .first = 0.0.
    QVERIFY(std::abs(minMaxThresh.first - 0.0) < 1e-10);
    QVERIFY(std::abs(minMaxFull.second - minMaxThresh.second) < 1e-10);
}

//=============================================================================================================

void TestNetworkVisualization::testMinMaxDegrees()
{
    // 4-node fully connected: all degrees = 3
    Network net = buildTestNetwork(4);
    net.setThreshold(0.0);

    QPair<int, int> minMaxDeg = net.getMinMaxFullDegrees();
    QCOMPARE(minMaxDeg.first, 3);
    QCOMPARE(minMaxDeg.second, 3);

    QPair<int, int> minMaxThreshDeg = net.getMinMaxThresholdedDegrees();
    QCOMPARE(minMaxThreshDeg.first, 3);
    QCOMPARE(minMaxThreshDeg.second, 3);

    // Linear network: degrees are 1, 2, 1
    m_linearNet.setThreshold(0.0);
    QPair<int, int> linearDeg = m_linearNet.getMinMaxFullDegrees();
    QCOMPARE(linearDeg.first, 1);
    QCOMPARE(linearDeg.second, 2);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestNetworkVisualization)
#include "test_network_visualization.moc"
