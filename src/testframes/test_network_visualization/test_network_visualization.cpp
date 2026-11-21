//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_network_visualization.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Tests for the ex_brain_view connectivity network visualization.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include <base/log_qt.h>

#include <connectivity/network/network.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <cmath>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
// HELPERS
//=============================================================================================================

/**
 * Build a synthetic fully-connected network with nNodes nodes
 * arranged on a circle in the XY plane at z=0, with edge weights
 * proportional to the reciprocal of Euclidean distance.
 */
static Network buildTestNetwork(int nNodes)
{
    Network net("TestMethod");

    // Create nodes on a circle (radius 50 mm, simulating brain scale)
    const float radius = 0.05f; // 50 mm in metres
    for (int i = 0; i < nNodes; ++i) {
        const float angle = 2.0f * static_cast<float>(std::numbers::pi) * static_cast<float>(i) / static_cast<float>(nNodes);
        net.add_node(Vector3f(radius * std::cos(angle), radius * std::sin(angle), 0.0f));
    }

    // Create edges between all pairs (fully connected, upper triangle), weight = 1/distance
    for (int i = 0; i < nNodes; ++i) {
        for (int j = i + 1; j < nNodes; ++j) {
            const float dist = (net.nodes()[i].position() - net.nodes()[j].position()).norm();
            net.add_edge(NetworkEdge(i, j, MatrixXd::Constant(1, 1, 1.0 / dist)));
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
    Network net("Linear");
    net.add_node(Vector3f(-0.05f, 0.0f, 0.0f));
    net.add_node(Vector3f(0.0f, 0.0f, 0.0f));
    net.add_node(Vector3f(0.05f, 0.0f, 0.0f));
    net.add_edge(NetworkEdge(0, 1, MatrixXd::Constant(1, 1, w01)));
    net.add_edge(NetworkEdge(1, 2, MatrixXd::Constant(1, 1, w12)));
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
    Network m_fullNet4;  // 4-node fully connected
    Network m_fullNet10; // 10-node fully connected
    Network m_linearNet; // 3-node chain
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TestNetworkVisualization::TestNetworkVisualization()
{
}

void TestNetworkVisualization::initTestCase()
{
    mne::install_qt_message_handler();

    m_fullNet4 = buildTestNetwork(4);
    m_fullNet10 = buildTestNetwork(10);
    m_linearNet = buildLinearNetwork(0.8, 0.3);

    qDebug() << "TestNetworkVisualization::initTestCase - Networks built";
    qDebug() << "  4-node:  " << m_fullNet4.nodes().size() << "nodes";
    qDebug() << "  10-node: " << m_fullNet10.nodes().size() << "nodes";
    qDebug() << "  linear:  " << m_linearNet.nodes().size() << "nodes";
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
    QTest::newRow("3 nodes") << 3 << 3;
    QTest::newRow("4 nodes") << 4 << 6;
    QTest::newRow("5 nodes") << 5 << 10;
    QTest::newRow("10 nodes") << 10 << 45;
    QTest::newRow("20 nodes") << 20 << 190;
}

void TestNetworkVisualization::testNetworkConstruction()
{
    QFETCH(int, numNodes);
    QFETCH(int, expectedEdges);

    Network net = buildTestNetwork(numNodes);

    QVERIFY(!net.empty());
    QCOMPARE(std::ssize(net.nodes()), numNodes);
    QCOMPARE(net.connectivity_matrix(EdgeSet::full, false).rows(), Index(numNodes));
    QCOMPARE(net.connectivity_matrix(EdgeSet::full, false).cols(), Index(numNodes));

    // Count total edges from the connectivity matrix upper triangle
    MatrixXd connMat = net.connectivity_matrix(EdgeSet::full, false);
    int edgeCount = 0;
    for (int i = 0; i < connMat.rows(); ++i) {
        for (int j = i + 1; j < connMat.cols(); ++j) {
            if (connMat(i, j) > 0)
                edgeCount++;
        }
    }
    QCOMPARE(edgeCount, expectedEdges);
}

//=============================================================================================================

void TestNetworkVisualization::testNodePositions()
{
    const auto& nodes = m_fullNet4.nodes();
    QCOMPARE(nodes.size(), std::size_t(4));

    // Nodes are on a circle of radius 0.05m in the z = 0 plane
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const float r = nodes[i].position().norm();
        QVERIFY2(std::abs(r - 0.05f) < 1e-5f, qPrintable(QString("Node %1 radius %2 != 0.05").arg(i).arg(r)));
        QCOMPARE(nodes[i].position()(2), 0.0f);
    }
}

//=============================================================================================================

void TestNetworkVisualization::testEdgeWeights()
{
    // In our test network, weights = 1/distance, all edges should have positive weights
    MatrixXd connMat = m_fullNet4.connectivity_matrix(EdgeSet::full, false);

    for (int i = 0; i < connMat.rows(); ++i) {
        for (int j = i + 1; j < connMat.cols(); ++j) {
            QVERIFY2(connMat(i, j) > 0.0,
                     qPrintable(QString("Edge (%1,%2) weight %3 should be > 0")
                                    .arg(i)
                                    .arg(j)
                                    .arg(connMat(i, j))));
        }
    }
}

//=============================================================================================================

void TestNetworkVisualization::testFullConnectivityMatrix()
{
    // Mirrored version should be symmetric
    MatrixXd mirrored = m_fullNet10.connectivity_matrix();
    QCOMPARE(mirrored.rows(), Index(10));
    QCOMPARE(mirrored.cols(), Index(10));

    for (int i = 0; i < mirrored.rows(); ++i) {
        for (int j = 0; j < mirrored.cols(); ++j) {
            QVERIFY2(std::abs(mirrored(i, j) - mirrored(j, i)) < 1e-10,
                     qPrintable(QString("Asymmetric at (%1,%2): %3 vs %4")
                                    .arg(i)
                                    .arg(j)
                                    .arg(mirrored(i, j))
                                    .arg(mirrored(j, i))));
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
    net.set_threshold(0.0);

    // At threshold 0, all 10 edges of 5 nodes are active
    QCOMPARE(net.edge_count(EdgeSet::thresholded), std::size_t(10));
}

//=============================================================================================================

void TestNetworkVisualization::testThresholdMax()
{
    // Threshold operates in absolute weight units (|weight| >= threshold).
    // Our test weights are ~10–14 (1/distance with radius 0.05m), so we need
    // a threshold above the maximum weight to filter everything out.
    Network net = buildTestNetwork(5);

    // First, find the max weight
    std::pair<double, double> minMax = net.weight_range();
    qDebug() << "  Weight range:" << minMax.first << "–" << minMax.second;

    // Set threshold above max weight -> no edges should survive
    net.set_threshold(minMax.second + 1.0);
    QCOMPARE(net.edge_count(EdgeSet::thresholded), std::size_t(0));

    // Set threshold between min and max -> some edges filtered
    double midThresh = (minMax.first + minMax.second) / 2.0;
    net.set_threshold(midThresh);
    const auto activeMid = net.edge_count(EdgeSet::thresholded);
    qDebug() << "  Mid threshold" << midThresh << "-> active:" << activeMid;
    QVERIFY2(activeMid > 0 && activeMid < 10,
             qPrintable(QString("Expected partial filtering, got %1 of 10").arg(activeMid)));
}

//=============================================================================================================

void TestNetworkVisualization::testThresholdProgressive()
{
    Network net = buildTestNetwork(6); // 15 edges

    // Threshold operates in absolute weight units; sweep from 0 to max+epsilon
    std::pair<double, double> minMax = net.weight_range();
    double step = (minMax.second - minMax.first) / 10.0;

    std::size_t prevActive = 15; // Start: all must be active at threshold 0
    for (int s = 0; s <= 10; ++s) {
        double t = minMax.first + step * s;
        net.set_threshold(t);
        const std::size_t activeCount = net.edge_count(EdgeSet::thresholded);

        qDebug() << "  Threshold" << t << "-> active:" << activeCount;

        // Active count should be monotonically non-increasing as threshold rises
        QVERIFY2(activeCount <= prevActive,
                 qPrintable(QString("Active edges increased from %1 to %2 at threshold %3")
                                .arg(prevActive)
                                .arg(activeCount)
                                .arg(t)));
        prevActive = activeCount;
    }

    // At threshold = max weight, at most a few edges with exactly max weight remain
    net.set_threshold(minMax.second);
    QVERIFY(net.edge_count(EdgeSet::thresholded) <= std::size_t(15));

    // Above max weight, no edges remain
    net.set_threshold(minMax.second + 1.0);
    QCOMPARE(net.edge_count(EdgeSet::thresholded), std::size_t(0));
}

//=============================================================================================================

void TestNetworkVisualization::testThresholdDegreeUpdate()
{
    Network net = buildTestNetwork(4);

    // At threshold 0, all nodes should have degree 3 (connected to all others)
    net.set_threshold(0.0);
    for (int i = 0; i < 4; ++i) {
        QCOMPARE(net.degree(i, EdgeSet::thresholded), 3);
    }

    // Use a threshold between min and max weight to filter some edges
    std::pair<double, double> minMax = net.weight_range();
    double midThresh = (minMax.first + minMax.second) / 2.0;
    net.set_threshold(midThresh);

    bool anyDecreased = false;
    for (int i = 0; i < 4; ++i) {
        qDebug() << "  Node" << i << "degree:" << net.degree(i, EdgeSet::thresholded);
        if (net.degree(i, EdgeSet::thresholded) < 3) {
            anyDecreased = true;
        }
    }
    QVERIFY2(anyDecreased,
             qPrintable(QString("At threshold %1 (mid of %2–%3), some degrees should decrease")
                            .arg(midThresh)
                            .arg(minMax.first)
                            .arg(minMax.second)));
}

//=============================================================================================================
// Visualization info
//=============================================================================================================

void TestNetworkVisualization::testVisualizationInfo()
{
    Network net = buildTestNetwork(4);

    // Default visualization info
    QCOMPARE(net.visualization_info().coloring, VisualizationInfo::Coloring::colormap);
    QVERIFY(!net.visualization_info().colormap.empty());

    net.set_visualization_info({.coloring = VisualizationInfo::Coloring::fixed,
                                .colormap = "Hot",
                                .node_color = Vector4i(0, 255, 0, 255),
                                .edge_color = Vector4i(0, 0, 255, 128)});

    const VisualizationInfo& retrieved = net.visualization_info();
    QCOMPARE(retrieved.coloring, VisualizationInfo::Coloring::fixed);
    QCOMPARE(retrieved.colormap, std::string("Hot"));
    QCOMPARE(retrieved.node_color, Vector4i(0, 255, 0, 255));
    QCOMPARE(retrieved.edge_color, Vector4i(0, 0, 255, 128));
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
    m_linearNet.set_threshold(0.0);
    QCOMPARE(m_linearNet.degree(0, EdgeSet::thresholded), 1);
    QCOMPARE(m_linearNet.degree(1, EdgeSet::thresholded), 2);
    QCOMPARE(m_linearNet.degree(2, EdgeSet::thresholded), 1);
}

//=============================================================================================================

void TestNetworkVisualization::testLinearNetworkEdgeIds()
{
    m_linearNet.set_threshold(0.0);
    QCOMPARE(m_linearNet.edge_count(EdgeSet::thresholded), std::size_t(2));

    // Verify edge endpoint IDs
    bool foundEdge01 = false, foundEdge12 = false;
    for (const NetworkEdge& edge : m_linearNet.edges()) {
        const int s = edge.source();
        const int e = edge.target();
        if ((s == 0 && e == 1) || (s == 1 && e == 0))
            foundEdge01 = true;
        if ((s == 1 && e == 2) || (s == 2 && e == 1))
            foundEdge12 = true;
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
    QVERIFY(net.empty());
    QVERIFY(net.nodes().empty());
}

//=============================================================================================================

void TestNetworkVisualization::testSingleNode()
{
    Network net;
    net.add_node(Vector3f::Zero());

    // A network needs both nodes and edges to be non-empty
    QVERIFY(net.empty());
    QCOMPARE(net.nodes().size(), std::size_t(1));
    QCOMPARE(net.degree(0, EdgeSet::thresholded), 0);
}

//=============================================================================================================

void TestNetworkVisualization::testTwoNodes()
{
    Network net;
    net.add_node(Vector3f(0.0f, 0.0f, 0.0f));
    net.add_node(Vector3f(0.1f, 0.0f, 0.0f));
    net.add_edge(NetworkEdge(0, 1, MatrixXd::Constant(1, 1, 0.75)));

    net.set_threshold(0.0);
    QCOMPARE(net.degree(0, EdgeSet::thresholded), 1);
    QCOMPARE(net.degree(1, EdgeSet::thresholded), 1);
    QCOMPARE(net.edges()[0].weight(), 0.75);
}

//=============================================================================================================
// Min/Max metrics
//=============================================================================================================

void TestNetworkVisualization::testMinMaxWeights()
{
    Network net = buildTestNetwork(4);
    net.set_threshold(0.0);

    std::pair<double, double> minMaxFull = net.weight_range();
    std::pair<double, double> minMaxThresh = net.weight_range(EdgeSet::thresholded);

    qDebug() << "  Full weights: min =" << minMaxFull.first << " max =" << minMaxFull.second;
    qDebug() << "  Thresholded weights: min =" << minMaxThresh.first << " max =" << minMaxThresh.second;

    // Min should be less than max
    QVERIFY(minMaxFull.first <= minMaxFull.second);
    QVERIFY(minMaxThresh.first <= minMaxThresh.second);

    // Both min and max should be positive (our weights are 1/distance)
    QVERIFY(minMaxFull.first > 0.0);
    QVERIFY(minMaxFull.second > 0.0);

    // The thresholded range is {threshold, full max weight}; at threshold 0, .first = 0.0.
    QVERIFY(std::abs(minMaxThresh.first - 0.0) < 1e-10);
    QVERIFY(std::abs(minMaxFull.second - minMaxThresh.second) < 1e-10);
}

//=============================================================================================================

void TestNetworkVisualization::testMinMaxDegrees()
{
    // 4-node fully connected: all degrees = 3
    Network net = buildTestNetwork(4);
    net.set_threshold(0.0);

    std::pair<int, int> minMaxDeg = net.degree_range();
    QCOMPARE(minMaxDeg.first, 3);
    QCOMPARE(minMaxDeg.second, 3);

    std::pair<int, int> minMaxThreshDeg = net.degree_range(EdgeSet::thresholded);
    QCOMPARE(minMaxThreshDeg.first, 3);
    QCOMPARE(minMaxThreshDeg.second, 3);

    // Linear network: degrees are 1, 2, 1
    m_linearNet.set_threshold(0.0);
    std::pair<int, int> linearDeg = m_linearNet.degree_range();
    QCOMPARE(linearDeg.first, 1);
    QCOMPARE(linearDeg.second, 2);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestNetworkVisualization)
#include "test_network_visualization.moc"
