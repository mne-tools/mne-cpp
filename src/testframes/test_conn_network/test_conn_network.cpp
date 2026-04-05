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
        net.append(QSharedPointer<NetworkEdge>::create(1, 3, w03));
        net.append(QSharedPointer<NetworkEdge>::create(0, 2, w05));
        net.append(QSharedPointer<NetworkEdge>::create(2, 3, w06));
        net.append(QSharedPointer<NetworkEdge>::create(0, 1, w08));

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
};

QTEST_GUILESS_MAIN(TestConnNetwork)
#include "test_conn_network.moc"
