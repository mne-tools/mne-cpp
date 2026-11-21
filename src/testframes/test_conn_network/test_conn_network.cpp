//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_conn_network.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     April, 2026
 * @brief    Tests for the Network, NetworkNode, and NetworkEdge classes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <connectivity/network/network.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

using IntPair = std::pair<int, int>;
using DoublePair = std::pair<double, double>;
using FloatPair = std::pair<float, float>;

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
    static Network buildDiamond()
    {
        Network net("TestCorrelation", 0.0);

        net.add_node(Vector3f(0, 0, 0));
        net.add_node(Vector3f(1, 0, 0));
        net.add_node(Vector3f(0, 1, 0));
        net.add_node(Vector3f(1, 1, 0));

        net.add_edge(NetworkEdge(1, 3, MatrixXd::Constant(1, 1, 0.3)));
        net.add_edge(NetworkEdge(0, 2, MatrixXd::Constant(1, 1, 0.5)));
        net.add_edge(NetworkEdge(2, 3, MatrixXd::Constant(1, 1, 0.6)));
        net.add_edge(NetworkEdge(0, 1, MatrixXd::Constant(1, 1, 0.8)));

        return net;
    }

    static Network twoNodes()
    {
        Network net("Test", 0.0);
        net.add_node(Vector3f::Zero());
        net.add_node(Vector3f::Zero());
        return net;
    }

private slots:

    //=========================================================================================================
    // NetworkNode
    //=========================================================================================================

    void testNode()
    {
        NetworkNode node(Vector3f(1.0f, 2.0f, 3.0f));
        QCOMPARE(node.position()(0), 1.0f);
        QCOMPARE(node.position()(2), 3.0f);
        QVERIFY(node.edges().empty());

        QVERIFY(!node.hub());
        node.set_hub(true);
        QVERIFY(node.hub());
    }

    void testNodeEdgeIndices()
    {
        const Network net = buildDiamond();
        // Node 3 is the end of edge 0 (1->3) and edge 2 (2->3)
        QCOMPARE(net.nodes()[3].edges(), std::vector<int>({0, 2}));
        QCOMPARE(net.nodes()[0].edges(), std::vector<int>({1, 3}));
    }

    //=========================================================================================================
    // NetworkEdge
    //=========================================================================================================

    void testEdgeConstruction()
    {
        NetworkEdge edge(0, 1, MatrixXd::Constant(1, 1, 0.75));
        QCOMPARE(edge.source(), 0);
        QCOMPARE(edge.target(), 1);
        QVERIFY(edge.active());
        QCOMPARE(edge.weight(), 0.75);

        edge.set_active(false);
        QVERIFY(!edge.active());
        edge.set_weight(0.99);
        QCOMPARE(edge.weight(), 0.99);
    }

    void testEdgeAveragesAllWeights()
    {
        const NetworkEdge edge(2, 5, (MatrixXd(1, 3) << 0.1, 0.5, 0.9).finished());
        QVERIFY(qAbs(edge.weight() - 0.5) < 1e-12);
    }

    void testEdgeFrequencyBins()
    {
        NetworkEdge edge(0, 1, (MatrixXd(4, 1) << 0.1, 0.2, 0.3, 0.4).finished());

        QCOMPARE(edge.frequency_bins(), IntPair(-1, -1));
        QVERIFY(qAbs(edge.weight() - 0.25) < 1e-12);

        edge.set_frequency_bins({0, 1});
        QCOMPARE(edge.frequency_bins(), IntPair(0, 1));
        QVERIFY(qAbs(edge.weight() - 0.15) < 1e-12);

        // A window reaching past the last bin is clipped
        edge.set_frequency_bins({2, 10});
        QVERIFY(qAbs(edge.weight() - 0.35) < 1e-12);

        // An invalid window keeps the previous weight
        edge.set_frequency_bins({3, 1});
        QVERIFY(qAbs(edge.weight() - 0.35) < 1e-12);
    }

    void testEdgeWeights()
    {
        const NetworkEdge edge(0, 1, (MatrixXd(2, 2) << 0.1, 0.2, 0.3, 0.4).finished());
        QCOMPARE(edge.weights().rows(), Index(2));
        QCOMPARE(edge.weights().cols(), Index(2));
        QCOMPARE(edge.weights()(0, 0), 0.1);
    }

    void testEdgeZeroMatrix()
    {
        const NetworkEdge edge(0, 1, MatrixXd(0, 0));
        QCOMPARE(edge.weights().size(), Index(1));
        QCOMPARE(edge.weight(), 0.0);
    }

    //=========================================================================================================
    // Network construction
    //=========================================================================================================

    void testEmptyNetwork()
    {
        Network net;
        QVERIFY(net.empty());
        QCOMPARE(net.method(), std::string("Unknown"));
        QCOMPARE(net.weight_range(), DoublePair(0.0, 0.0));
        QCOMPARE(net.degree_range(), IntPair(0, 0));

        net.set_method("WPLI");
        QCOMPARE(net.method(), std::string("WPLI"));
    }

    void testNetworkMethod()
    {
        const Network net("PLI", 0.5);
        QCOMPARE(net.method(), std::string("PLI"));
        QCOMPARE(net.threshold(), 0.5);
    }

    void testAddNodesAndEdges()
    {
        const Network net = buildDiamond();
        QVERIFY(!net.empty());
        QCOMPARE(net.nodes().size(), std::size_t(4));
        QCOMPARE(net.edges().size(), std::size_t(4));
        QCOMPARE(net.edge_count(), std::size_t(4));
        QCOMPARE(net.edges()[0].source(), 1);
        QCOMPARE(net.edges()[0].target(), 3);
    }

    void testAddEdgeRejectsSelfLoopsAndInvalidNodes()
    {
        Network net = twoNodes();
        net.add_edge(NetworkEdge(1, 1, MatrixXd::Constant(1, 1, 0.9)));
        QVERIFY(net.empty());

        QVERIFY_THROWS_EXCEPTION(std::out_of_range, net.add_edge(NetworkEdge(0, 5, MatrixXd::Constant(1, 1, 0.7))));
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, net.add_edge(NetworkEdge(-1, 0, MatrixXd::Constant(1, 1, 0.7))));
        QVERIFY(net.empty());
        QVERIFY(net.nodes()[0].edges().empty());
    }

    void testAddEdgeAppliesThreshold()
    {
        Network net("Test", 0.5);
        net.add_node(Vector3f::Zero());
        net.add_node(Vector3f::Zero());
        net.add_node(Vector3f::Zero());
        net.add_edge(NetworkEdge(0, 1, MatrixXd::Constant(1, 1, 0.2)));
        net.add_edge(NetworkEdge(1, 2, MatrixXd::Constant(1, 1, -0.7)));

        QVERIFY(!net.edges()[0].active());
        QVERIFY(net.edges()[1].active());
        QCOMPARE(net.edge_count(EdgeSet::thresholded), std::size_t(1));
    }

    void testCopiesAreIndependent()
    {
        const Network original = buildDiamond();
        Network copy = original;
        copy.set_threshold(0.7);
        copy.normalize();

        QVERIFY(original.edges()[1].active());
        QCOMPARE(original.edges()[3].weight(), 0.8);
        QCOMPARE(copy.edges()[3].weight(), 1.0);
        QVERIFY(!copy.edges()[1].active());
    }

    //=========================================================================================================
    // Connectivity matrix
    //=========================================================================================================

    void testConnectivityMatrix()
    {
        const Network net = buildDiamond();
        const MatrixXd C = net.connectivity_matrix();

        QCOMPARE(C.rows(), Index(4));
        QCOMPARE(C.diagonal().cwiseAbs().sum(), 0.0);
        QCOMPARE(C(0, 1), 0.8);
        QCOMPARE(C(1, 0), 0.8);

        const MatrixXd upper = net.connectivity_matrix(EdgeSet::full, false);
        QCOMPARE(upper(1, 0), 0.0);
        QCOMPARE(upper(0, 1), 0.8);
    }

    void testThresholdedConnectivityMatrix()
    {
        Network net = buildDiamond();
        net.set_threshold(0.4);

        const MatrixXd C = net.connectivity_matrix(EdgeSet::thresholded);
        QCOMPARE(C(1, 3), 0.0);
        QCOMPARE(C(3, 1), 0.0);
        QCOMPARE(C(0, 1), 0.8);

        const MatrixXd upper = net.connectivity_matrix(EdgeSet::thresholded, false);
        QCOMPARE(upper(1, 0), 0.0);
        QCOMPARE(upper(0, 1), 0.8);
    }

    //=========================================================================================================
    // Threshold
    //=========================================================================================================

    void testThresholding()
    {
        Network net = buildDiamond();
        net.set_threshold(0.4);
        QCOMPARE(net.threshold(), 0.4);
        QCOMPARE(net.edge_count(EdgeSet::thresholded), std::size_t(3));
        QVERIFY(!net.edges()[0].active());

        net.set_threshold(0.9);
        QCOMPARE(net.edge_count(EdgeSet::thresholded), std::size_t(0));
    }

    //=========================================================================================================
    // Degree and strength
    //=========================================================================================================

    void testDegree()
    {
        Network net = buildDiamond();
        QCOMPARE(net.degree(0), 2);
        QCOMPARE(net.degree(1, EdgeSet::full, Direction::in), 1);
        QCOMPARE(net.degree(0, EdgeSet::full, Direction::out), 2);
        QCOMPARE(net.degree(3, EdgeSet::full, Direction::in), 2);

        net.set_threshold(0.4);
        QCOMPARE(net.degree(0, EdgeSet::thresholded), 2);
        QCOMPARE(net.degree(3, EdgeSet::thresholded), 1);
        QCOMPARE(net.degree(3, EdgeSet::thresholded, Direction::in), 1);
        QCOMPARE(net.degree(1, EdgeSet::thresholded, Direction::out), 0);
    }

    void testStrength()
    {
        Network net = buildDiamond();
        QVERIFY(qAbs(net.strength(0) - 1.3) < 1e-12);
        QVERIFY(qAbs(net.strength(1, EdgeSet::full, Direction::in) - 0.8) < 1e-12);
        QVERIFY(qAbs(net.strength(0, EdgeSet::full, Direction::out) - 1.3) < 1e-12);

        net.set_threshold(0.4);
        QVERIFY(qAbs(net.strength(0, EdgeSet::thresholded) - 1.3) < 1e-12);
        QVERIFY(qAbs(net.strength(3, EdgeSet::thresholded, Direction::in) - 0.6) < 1e-12);
        QCOMPARE(net.strength(1, EdgeSet::thresholded, Direction::out), 0.0);
    }

    void testMixedDirections()
    {
        Network net("Test", 1.0);
        for (int i = 0; i < 5; ++i) {
            net.add_node(Vector3f::Zero());
        }
        net.add_edge(NetworkEdge(0, 1, MatrixXd::Constant(1, 1, 0.5)));
        net.add_edge(NetworkEdge(0, 2, MatrixXd::Constant(1, 1, 0.25)));
        net.add_edge(NetworkEdge(3, 0, MatrixXd::Constant(1, 1, 2.0)));
        net.add_edge(NetworkEdge(4, 0, MatrixXd::Constant(1, 1, 1.5)));
        net.add_edge(NetworkEdge(3, 4, MatrixXd::Constant(1, 1, 8.0)));

        QCOMPARE(net.degree(0), 4);
        QCOMPARE(net.degree(0, EdgeSet::thresholded), 2);
        QCOMPARE(net.degree(0, EdgeSet::full, Direction::in), 2);
        QCOMPARE(net.degree(0, EdgeSet::thresholded, Direction::in), 2);
        QCOMPARE(net.degree(0, EdgeSet::full, Direction::out), 2);
        QCOMPARE(net.degree(0, EdgeSet::thresholded, Direction::out), 0);

        QCOMPARE(net.strength(0), 4.25);
        QCOMPARE(net.strength(0, EdgeSet::thresholded), 3.5);
        QCOMPARE(net.strength(0, EdgeSet::full, Direction::in), 3.5);
        QCOMPARE(net.strength(0, EdgeSet::full, Direction::out), 0.75);
    }

    void testTotalDegree()
    {
        Network net = buildDiamond();
        QCOMPARE(net.total_degree(), 8);
        net.set_threshold(0.4);
        QCOMPARE(net.total_degree(EdgeSet::thresholded), 6);
    }

    void testDegreeRanges()
    {
        Network net = buildDiamond();
        QCOMPARE(net.degree_range(), IntPair(2, 2));
        QCOMPARE(net.degree_range(EdgeSet::full, Direction::in), IntPair(0, 2));
        QCOMPARE(net.degree_range(EdgeSet::full, Direction::out), IntPair(0, 2));

        net.set_threshold(0.4);
        QCOMPARE(net.degree_range(EdgeSet::thresholded), IntPair(1, 2));
        QCOMPARE(net.degree_range(EdgeSet::thresholded, Direction::in), IntPair(0, 1));
        QCOMPARE(net.degree_range(EdgeSet::thresholded, Direction::out), IntPair(0, 2));
    }

    //=========================================================================================================
    // Weight ranges, frequency range, normalization
    //=========================================================================================================

    void testWeightRanges()
    {
        Network net = buildDiamond();
        QCOMPARE(net.weight_range(), DoublePair(0.3, 0.8));

        net.set_threshold(0.4);
        QCOMPARE(net.weight_range(EdgeSet::thresholded), DoublePair(0.4, 0.8));
    }

    void testWeightRangeUsesMagnitudes()
    {
        Network net = twoNodes();
        net.add_edge(NetworkEdge(0, 1, MatrixXd::Constant(1, 1, -0.9)));
        net.add_edge(NetworkEdge(1, 0, MatrixXd::Constant(1, 1, 0.2)));
        QCOMPARE(net.weight_range(), DoublePair(0.2, 0.9));
    }

    void testFrequencyRange()
    {
        Network net = twoNodes();
        net.add_edge(NetworkEdge(0, 1, (MatrixXd(4, 1) << 0.125, 0.125, 0.875, 0.875).finished()));
        net.add_edge(NetworkEdge(1, 0, (MatrixXd(4, 1) << 1.0, 1.0, -0.25, -0.25).finished()));

        const FloatPair unset(0.0f, 0.0f);
        net.set_frequency_range(3.0f, 1.0f);
        QCOMPARE(net.frequency_range(), unset);
        net.set_frequency_range(1.0f, 3.0f);
        QCOMPARE(net.frequency_range(), unset);

        net.set_sampling_frequency(8.0f);
        QCOMPARE(net.sampling_frequency(), 8.0f);
        net.set_fft_size(4);
        QCOMPARE(net.fft_size(), 4);
        net.set_frequency_range(1.0f, 5.0f);
        QCOMPARE(net.frequency_range(), unset);
        net.set_frequency_range(1.0f, 3.0f);
        QCOMPARE(net.frequency_range(), unset);

        // Scale factor fft/(sfreq/2) = 1, so 2..3 Hz maps to bins 2..3.
        net.set_used_freq_bins(4);
        QCOMPARE(net.used_freq_bins(), 4);
        net.set_frequency_range(2.0f, 3.0f);
        QCOMPARE(net.frequency_range(), FloatPair(2.0f, 3.0f));
        QCOMPARE(net.edges()[0].weight(), 0.875);
        QCOMPARE(net.edges()[1].weight(), -0.25);
        QCOMPARE(net.weight_range(), DoublePair(0.25, 0.875));
    }

    void testNormalize()
    {
        Network net = buildDiamond();
        net.set_threshold(0.4);
        net.normalize();

        QCOMPARE(net.weight_range().second, 1.0);
        QVERIFY(qAbs(net.weight_range().first - 0.3 / 0.8) < 1e-12);
        QVERIFY(qAbs(net.threshold() - 0.5) < 1e-12);
        QCOMPARE(net.edge_count(EdgeSet::thresholded), std::size_t(3));
    }

    void testNormalizeNegativeWeights()
    {
        Network net = twoNodes();
        net.add_edge(NetworkEdge(0, 1, MatrixXd::Constant(1, 1, -0.5)));
        net.normalize();
        QCOMPARE(net.edges()[0].weight(), -1.0);
    }

    void testNormalizeAllZeroIsNoOp()
    {
        Network net = twoNodes();
        net.add_edge(NetworkEdge(0, 1, MatrixXd::Constant(1, 1, 0.0)));
        net.normalize();
        QCOMPARE(net.edges()[0].weight(), 0.0);
    }

    //=========================================================================================================
    // VisualizationInfo
    //=========================================================================================================

    void testVisualizationInfo()
    {
        Network net;
        QCOMPARE(net.visualization_info().coloring, VisualizationInfo::Coloring::colormap);

        net.set_visualization_info({.coloring = VisualizationInfo::Coloring::fixed, .colormap = "Hot"});
        QCOMPARE(net.visualization_info().coloring, VisualizationInfo::Coloring::fixed);
        QCOMPARE(net.visualization_info().colormap, std::string("Hot"));
    }
};

QTEST_GUILESS_MAIN(TestConnNetwork)
#include "test_conn_network.moc"
