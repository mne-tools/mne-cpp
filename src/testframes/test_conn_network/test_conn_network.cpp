//=============================================================================================================
/**
 * @file     test_conn_network.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     April, 2026
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
 * @brief    Tests for the Network, NetworkNode, and NetworkEdge classes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <conn/network/network.h>
#include <conn/network/networknode.h>
#include <conn/network/networkedge.h>

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

using namespace CONNLIB;
using namespace Eigen;

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestConnNetwork : public QObject
{
    Q_OBJECT

private:
    //=========================================================================================================
    /**
     * Build a small synthetic network: 4 nodes forming a diamond graph.
     *
     *   0 --0.8-- 1
     *   |         |
     *  0.5       0.3
     *   |         |
     *   2 --0.6-- 3
     */
    Network buildDiamond()
    {
        Network net("TestCorrelation", 0.0);

        auto n0 = QSharedPointer<NetworkNode>::create(0, RowVectorXf::Zero(3));
        auto n1 = QSharedPointer<NetworkNode>::create(1, (RowVectorXf(3) << 1,0,0).finished());
        auto n2 = QSharedPointer<NetworkNode>::create(2, (RowVectorXf(3) << 0,1,0).finished());
        auto n3 = QSharedPointer<NetworkNode>::create(3, (RowVectorXf(3) << 1,1,0).finished());

        net.append(n0); net.append(n1); net.append(n2); net.append(n3);

        MatrixXd w08(1,1); w08(0,0) = 0.8;
        MatrixXd w05(1,1); w05(0,0) = 0.5;
        MatrixXd w03(1,1); w03(0,0) = 0.3;
        MatrixXd w06(1,1); w06(0,0) = 0.6;

        // Insert smallest weight first so Network::append min/max tracking
        // (which uses if/else-if) correctly captures both extremes.
        auto e13 = QSharedPointer<NetworkEdge>::create(1, 3, w03);
        auto e02 = QSharedPointer<NetworkEdge>::create(0, 2, w05);
        auto e23 = QSharedPointer<NetworkEdge>::create(2, 3, w06);
        auto e01 = QSharedPointer<NetworkEdge>::create(0, 1, w08);

        net.append(e13); net.append(e02); net.append(e23); net.append(e01);

        // Register edges on nodes so node-level degree/strength methods work
        n0->append(e02); n0->append(e01);
        n1->append(e13); n1->append(e01);
        n2->append(e02); n2->append(e23);
        n3->append(e13); n3->append(e23);

        return net;
    }

private slots:

    //=========================================================================================================
    // NetworkNode tests
    //=========================================================================================================

    void testNodeConstruction()
    {
        RowVectorXf pos(3);
        pos << 1.0f, 2.0f, 3.0f;
        auto node = QSharedPointer<NetworkNode>::create(42, pos);

        QCOMPARE(node->getId(), static_cast<qint16>(42));
        QCOMPARE(node->getVert().size(), 3);
        QCOMPARE(node->getVert()(0), 1.0f);
    }

    //=========================================================================================================
    // NetworkEdge tests
    //=========================================================================================================

    void testEdgeConstruction()
    {
        MatrixXd w(1,1); w(0,0) = 0.75;
        auto edge = QSharedPointer<NetworkEdge>::create(0, 1, w, true);

        QCOMPARE(edge->getStartNodeID(), 0);
        QCOMPARE(edge->getEndNodeID(), 1);
        QVERIFY(edge->isActive());
    }

    void testEdgeActiveToggle()
    {
        MatrixXd w(1,1); w(0,0) = 0.5;
        auto edge = QSharedPointer<NetworkEdge>::create(0, 1, w, true);

        QVERIFY(edge->isActive());
        edge->setActive(false);
        QVERIFY(!edge->isActive());
    }

    void testEdgeWeight()
    {
        MatrixXd w(1,3); w << 0.1, 0.5, 0.9;
        auto edge = QSharedPointer<NetworkEdge>::create(2, 5, w);

        // Average across all bins
        double avg = edge->getWeight();
        double expected = (0.1 + 0.5 + 0.9) / 3.0;
        QVERIFY(qAbs(avg - expected) < 1e-10);
    }

    //=========================================================================================================
    // Network construction tests
    //=========================================================================================================

    void testEmptyNetwork()
    {
        Network net;
        QVERIFY(net.isEmpty());
        QCOMPARE(net.getConnectivityMethod(), QString("Unknown"));
    }

    void testNetworkMethod()
    {
        Network net("PLI", 0.5);
        QCOMPARE(net.getConnectivityMethod(), QString("PLI"));
        QCOMPARE(net.getThreshold(), 0.5);
    }

    void testAppendNodesAndEdges()
    {
        Network net = buildDiamond();
        QVERIFY(!net.isEmpty());
        QCOMPARE(net.getNodes().size(), 4);
        QCOMPARE(net.getFullEdges().size(), 4);
    }

    //=========================================================================================================
    // Connectivity matrix tests
    //=========================================================================================================

    void testFullConnectivityMatrix()
    {
        Network net = buildDiamond();
        MatrixXd C = net.getFullConnectivityMatrix(true);

        QCOMPARE(C.rows(), 4);
        QCOMPARE(C.cols(), 4);

        // Diagonal should be zero
        for (int i = 0; i < 4; ++i)
            QCOMPARE(C(i,i), 0.0);

        // Edge 0→1 has weight 0.8
        QVERIFY(qAbs(C(0,1) - 0.8) < 1e-10);

        // Mirrored: C(1,0) should also be 0.8
        QVERIFY(qAbs(C(1,0) - 0.8) < 1e-10);
    }

    //=========================================================================================================
    // Threshold tests
    //=========================================================================================================

    void testThresholding()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);

        // Edges with weight >= 0.4: 0.8, 0.5, 0.6 → 3 edges active
        const auto& tEdges = net.getThresholdedEdges();
        int activeCount = 0;
        for (const auto& e : tEdges)
            if (e->isActive()) ++activeCount;
        QCOMPARE(activeCount, 3);
    }

    void testThresholdHighCutsAll()
    {
        Network net = buildDiamond();
        net.setThreshold(0.9);

        const auto& tEdges = net.getThresholdedEdges();
        int activeCount = 0;
        for (const auto& e : tEdges)
            if (e->isActive()) ++activeCount;
        QCOMPARE(activeCount, 0);
    }

    //=========================================================================================================
    // Min/max tests
    //=========================================================================================================

    void testMinMaxWeights()
    {
        Network net = buildDiamond();
        auto minmax = net.getMinMaxFullWeights();

        QVERIFY(qAbs(minmax.first - 0.3) < 1e-10);
        QVERIFY(qAbs(minmax.second - 0.8) < 1e-10);
    }

    //=========================================================================================================
    // Frequency range tests
    //=========================================================================================================

    void testFrequencyRange()
    {
        Network net;
        // All prerequisites must be set before setFrequencyRange will accept values
        net.setSamplingFrequency(1000.0f);
        net.setFFTSize(128);
        net.setUsedFreqBins(64);
        net.setFrequencyRange(8.0f, 13.0f);
        auto range = net.getFrequencyRange();
        QCOMPARE(range.first, 8.0f);
        QCOMPARE(range.second, 13.0f);
    }

    //=========================================================================================================
    // Sampling frequency tests
    //=========================================================================================================

    void testSamplingFrequency()
    {
        Network net;
        net.setSamplingFrequency(1000.0f);
        QCOMPARE(net.getSamplingFrequency(), 1000.0f);
    }

    //=========================================================================================================
    // Normalization tests
    //=========================================================================================================

    void testNormalize()
    {
        Network net = buildDiamond();
        net.normalize();

        auto minmax = net.getMinMaxFullWeights();
        // After normalization, max should be 1.0
        QVERIFY(qAbs(minmax.second - 1.0) < 1e-10);
        // Min should be 0.3/0.8
        QVERIFY(qAbs(minmax.first - 0.3/0.8) < 1e-6);
    }

    //=========================================================================================================
    // VisualizationInfo tests
    //=========================================================================================================

    void testVisualizationInfo()
    {
        Network net;
        VisualizationInfo vi;
        vi.sMethod = "Color";
        vi.sColormap = "Hot";
        net.setVisualizationInfo(vi);

        auto result = net.getVisualizationInfo();
        QCOMPARE(result.sMethod, QString("Color"));
        QCOMPARE(result.sColormap, QString("Hot"));
    }

    //=========================================================================================================
    // Node degree/strength tests (exercises networknode.cpp uncovered methods)
    //=========================================================================================================

    void testNodeFullDegree()
    {
        Network net = buildDiamond();
        // Node 0 is connected to edges 0→2 and 0→1 = 2 edges
        auto n0 = net.getNodeAt(0);
        QCOMPARE(n0->getFullDegree(), static_cast<qint16>(2));
    }

    void testNodeThresholdedDegree()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        // Node 0: edges 0→2 (w=0.5, active) and 0→1 (w=0.8, active) → 2 thresholded
        auto n0 = net.getNodeAt(0);
        QCOMPARE(n0->getThresholdedDegree(), static_cast<qint16>(2));
        // Node 3: edges 1→3 (w=0.3, inactive) and 2→3 (w=0.6, active) → 1 thresholded
        auto n3 = net.getNodeAt(3);
        QCOMPARE(n3->getThresholdedDegree(), static_cast<qint16>(1));
    }

    void testNodeFullIndegree()
    {
        Network net = buildDiamond();
        // Node 1: edge 0→1 has endNodeID=1, so indegree=1
        auto n1 = net.getNodeAt(1);
        QCOMPARE(n1->getFullIndegree(), static_cast<qint16>(1));
    }

    void testNodeThresholdedIndegree()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        // Node 3: edge 1→3 (w=0.3, inactive), edge 2→3 (w=0.6, active)
        auto n3 = net.getNodeAt(3);
        QCOMPARE(n3->getThresholdedIndegree(), static_cast<qint16>(1));
    }

    void testNodeFullOutdegree()
    {
        Network net = buildDiamond();
        // Node 0: edges 0→2 and 0→1 have startNodeID=0 → outdegree=2
        auto n0 = net.getNodeAt(0);
        QCOMPARE(n0->getFullOutdegree(), static_cast<qint16>(2));
    }

    void testNodeThresholdedOutdegree()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        // Node 1: edge 1→3 (w=0.3, inactive) → thresholded outdegree=0
        auto n1 = net.getNodeAt(1);
        QCOMPARE(n1->getThresholdedOutdegree(), static_cast<qint16>(0));
    }

    void testNodeFullStrength()
    {
        Network net = buildDiamond();
        // Node 0: edges 0→2 (w=0.5) and 0→1 (w=0.8) → strength=1.3
        auto n0 = net.getNodeAt(0);
        QVERIFY(qAbs(n0->getFullStrength() - 1.3) < 1e-10);
    }

    void testNodeThresholdedStrength()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        // Node 0: both edges active (0.5 + 0.8 = 1.3)
        auto n0 = net.getNodeAt(0);
        QVERIFY(qAbs(n0->getThresholdedStrength() - 1.3) < 1e-10);
    }

    void testNodeFullInstrength()
    {
        Network net = buildDiamond();
        // Node 1: only edge 0→1 (w=0.8) has endNodeID=1
        auto n1 = net.getNodeAt(1);
        QVERIFY(qAbs(n1->getFullInstrength() - 0.8) < 1e-10);
    }

    void testNodeThresholdedInstrength()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        // Node 3: edge 1→3 (w=0.3, inactive), edge 2→3 (w=0.6, active)
        auto n3 = net.getNodeAt(3);
        QVERIFY(qAbs(n3->getThresholdedInstrength() - 0.6) < 1e-10);
    }

    void testNodeFullOutstrength()
    {
        Network net = buildDiamond();
        // Node 0: edges 0→2 (w=0.5) and 0→1 (w=0.8) have startNodeID=0
        auto n0 = net.getNodeAt(0);
        QVERIFY(qAbs(n0->getFullOutstrength() - 1.3) < 1e-10);
    }

    void testNodeThresholdedOutstrength()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        // Node 1: edge 1→3 (w=0.3, inactive) → thresholded outstrength=0.0
        auto n1 = net.getNodeAt(1);
        QVERIFY(qAbs(n1->getThresholdedOutstrength()) < 1e-10);
    }

    void testNodeHubStatus()
    {
        RowVectorXf pos = RowVectorXf::Zero(3);
        auto node = QSharedPointer<NetworkNode>::create(0, pos);
        QVERIFY(!node->getHubStatus());
        node->setHubStatus(true);
        QVERIFY(node->getHubStatus());
    }

    void testNodeAppendSelfLoop()
    {
        RowVectorXf pos = RowVectorXf::Zero(3);
        auto node = QSharedPointer<NetworkNode>::create(5, pos);

        // Self-loop edge (startID == endID) should be rejected by append
        MatrixXd w(1,1); w(0,0) = 1.0;
        auto selfEdge = QSharedPointer<NetworkEdge>::create(5, 5, w);
        node->append(selfEdge);
        QCOMPARE(node->getFullDegree(), static_cast<qint16>(0));
    }

    void testNodeFullEdgesIn()
    {
        Network net = buildDiamond();
        // Node 3 has incoming edges: 1→3, 2→3
        auto n3 = net.getNodeAt(3);
        auto inEdges = n3->getFullEdgesIn();
        QCOMPARE(inEdges.size(), 2);
    }

    void testNodeFullEdgesOut()
    {
        Network net = buildDiamond();
        // Node 0 has outgoing edges: 0→2, 0→1
        auto n0 = net.getNodeAt(0);
        auto outEdges = n0->getFullEdgesOut();
        QCOMPARE(outEdges.size(), 2);
    }

    void testNodeThresholdedEdgesIn()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        // Node 3: edge 1→3 inactive, edge 2→3 active → 1 thresholded incoming
        auto n3 = net.getNodeAt(3);
        auto tInEdges = n3->getThresholdedEdgesIn();
        QCOMPARE(tInEdges.size(), 1);
    }

    void testNodeThresholdedEdgesOut()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        // Node 0: both edges active (0.5 and 0.8 >= 0.4) → 2 thresholded outgoing
        auto n0 = net.getNodeAt(0);
        auto tOutEdges = n0->getThresholdedEdgesOut();
        QCOMPARE(tOutEdges.size(), 2);
    }

    //=========================================================================================================
    // Edge frequency bin tests (exercises networkedge.cpp uncovered methods)
    //=========================================================================================================

    void testEdgeFrequencyBins()
    {
        MatrixXd w(4,1);
        w << 0.1, 0.2, 0.3, 0.4;
        auto edge = QSharedPointer<NetworkEdge>::create(0, 1, w);

        // Default bins: (-1,-1) → average all
        auto bins = edge->getFrequencyBins();
        QCOMPARE(bins.first, -1);
        QCOMPARE(bins.second, -1);
        double expected = (0.1 + 0.2 + 0.3 + 0.4) / 4.0;
        QVERIFY(qAbs(edge->getWeight() - expected) < 1e-10);

        // Set frequency bins to average rows 1-2 (0.2, 0.3)
        edge->setFrequencyBins(QPair<int,int>(1, 2));
        auto bins2 = edge->getFrequencyBins();
        QCOMPARE(bins2.first, 1);
        QCOMPARE(bins2.second, 2);
        double expected2 = (0.2 + 0.3) / 2.0;
        QVERIFY(qAbs(edge->getWeight() - expected2) < 1e-10);
    }

    void testEdgeMatrixWeight()
    {
        MatrixXd w(2,2);
        w << 0.1, 0.2, 0.3, 0.4;
        auto edge = QSharedPointer<NetworkEdge>::create(0, 1, w);

        MatrixXd mw = edge->getMatrixWeight();
        QCOMPARE(mw.rows(), static_cast<Eigen::Index>(2));
        QCOMPARE(mw.cols(), static_cast<Eigen::Index>(2));
        QVERIFY(qAbs(mw(0,0) - 0.1) < 1e-10);
    }

    void testEdgeSetWeight()
    {
        MatrixXd w(1,1); w(0,0) = 0.5;
        auto edge = QSharedPointer<NetworkEdge>::create(0, 1, w);
        edge->setWeight(0.99);
        QVERIFY(qAbs(edge->getWeight() - 0.99) < 1e-10);
    }

    void testEdgeZeroMatrix()
    {
        // Empty matrix should be converted to 1x1 zero matrix
        MatrixXd w(0,0);
        auto edge = QSharedPointer<NetworkEdge>::create(0, 1, w);
        QCOMPARE(edge->getWeight(), 0.0);
    }

    //=========================================================================================================
    // Network distribution/degree tests (exercises network.cpp uncovered methods)
    //=========================================================================================================

    void testFullDistribution()
    {
        Network net = buildDiamond();
        qint16 dist = net.getFullDistribution();
        // Each node's degree: n0=2, n1=2, n2=2, n3=2 → total=8
        QCOMPARE(dist, static_cast<qint16>(8));
    }

    void testThresholdedDistribution()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        qint16 dist = net.getThresholdedDistribution();
        // Active edges: 0→2 (0.5), 2→3 (0.6), 0→1 (0.8). Nodes involved:
        // n0: 2 active, n1: 1 active, n2: 2 active, n3: 1 active → 6
        QCOMPARE(dist, static_cast<qint16>(6));
    }

    void testThresholdedConnectivityMatrix()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        MatrixXd C = net.getThresholdedConnectivityMatrix(true);

        QCOMPARE(C.rows(), 4);
        QCOMPARE(C.cols(), 4);
        // Edge 1→3 (w=0.3) should be inactive → 0
        QCOMPARE(C(1,3), 0.0);
        // Edge 0→1 (w=0.8) should be active
        QVERIFY(qAbs(C(0,1) - 0.8) < 1e-10);
    }

    void testThresholdedConnectivityMatrixNoMirror()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        MatrixXd C = net.getThresholdedConnectivityMatrix(false);

        // Without mirroring, lower triangle should be 0
        QCOMPARE(C(1,0), 0.0);
        // Upper triangle edge 0→1 should have weight
        QVERIFY(qAbs(C(0,1) - 0.8) < 1e-10);
    }

    void testGetEdgeAt()
    {
        Network net = buildDiamond();
        auto edge = net.getEdgeAt(0);
        QVERIFY(edge);
        // First edge added was 1→3 with w=0.3
        QCOMPARE(edge->getStartNodeID(), 1);
        QCOMPARE(edge->getEndNodeID(), 3);
    }

    void testGetNodeAt()
    {
        Network net = buildDiamond();
        auto node = net.getNodeAt(2);
        QCOMPARE(node->getId(), static_cast<qint16>(2));
    }

    void testSetConnectivityMethod()
    {
        Network net;
        net.setConnectivityMethod("WPLI");
        QCOMPARE(net.getConnectivityMethod(), QString("WPLI"));
    }

    void testMinMaxThresholdedWeights()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        auto mm = net.getMinMaxThresholdedWeights();
        // setThreshold sets min = threshold, max = fullMax
        QVERIFY(qAbs(mm.first - 0.4) < 1e-10);
        QVERIFY(qAbs(mm.second - 0.8) < 1e-10);
    }

    void testMinMaxFullDegrees()
    {
        Network net = buildDiamond();
        auto mm = net.getMinMaxFullDegrees();
        // All nodes have degree 2
        QCOMPARE(mm.first, 2);
        QCOMPARE(mm.second, 2);
    }

    void testMinMaxThresholdedDegrees()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        auto mm = net.getMinMaxThresholdedDegrees();
        // n0=2, n1=1, n2=2, n3=1 → min=1, max=2
        QCOMPARE(mm.first, 1);
        QCOMPARE(mm.second, 2);
    }

    void testMinMaxFullIndegrees()
    {
        Network net = buildDiamond();
        auto mm = net.getMinMaxFullIndegrees();
        // n0 indegree=0, n1=1, n2=1, n3=2 → min=0, max=2
        QCOMPARE(mm.first, 0);
        QCOMPARE(mm.second, 2);
    }

    void testMinMaxThresholdedIndegrees()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        auto mm = net.getMinMaxThresholdedIndegrees();
        // n0=0, n1=1 (0→1 active), n2=1 (0→2 active), n3=1 (2→3 active) → min=0, max=1
        QCOMPARE(mm.first, 0);
        QCOMPARE(mm.second, 1);
    }

    void testMinMaxFullOutdegrees()
    {
        Network net = buildDiamond();
        auto mm = net.getMinMaxFullOutdegrees();
        // n0=2, n1=1, n2=1, n3=0 → min=0, max=2
        QCOMPARE(mm.first, 0);
        QCOMPARE(mm.second, 2);
    }

    void testMinMaxThresholdedOutdegrees()
    {
        Network net = buildDiamond();
        net.setThreshold(0.4);
        auto mm = net.getMinMaxThresholdedOutdegrees();
        // n0=2, n1=0 (edge 1→3 inactive), n2=1, n3=0 → min=0, max=2
        QCOMPARE(mm.first, 0);
        QCOMPARE(mm.second, 2);
    }

    void testFFTSize()
    {
        Network net;
        net.setFFTSize(256);
        QCOMPARE(net.getFFTSize(), 256);
    }

    void testUsedFreqBins()
    {
        Network net;
        net.setUsedFreqBins(64);
        QCOMPARE(net.getUsedFreqBins(), 64);
    }

    void testFullConnectivityMatrixNoMirror()
    {
        Network net = buildDiamond();
        MatrixXd C = net.getFullConnectivityMatrix(false);
        // Without mirroring, lower triangle should be 0
        QCOMPARE(C(1,0), 0.0);
        QVERIFY(qAbs(C(0,1) - 0.8) < 1e-10);
    }
};

QTEST_GUILESS_MAIN(TestConnNetwork)
#include "test_conn_network.moc"
