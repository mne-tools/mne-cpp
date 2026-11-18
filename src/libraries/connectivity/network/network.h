//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     network.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August 2016
 * @brief    Graph container that stores the result of one functional-connectivity metric as nodes (sources/sensors) and weighted edges (channel-pair couplings).
 *
 * @ref mne::Network is the common output type returned by every estimator in
 * @c mne_connectivity. Each @ref mne::NetworkNode carries a 3D position (sensor coordinate
 * for sensor-space metrics, source vertex for source-space metrics) and
 * the indices of its incident edges, and each @ref mne::NetworkEdge stores the
 * full per-frequency weight matrix together with the scalar band-averaged
 * weight used for thresholding and display. The same container is used
 * for symmetric metrics (coherence, PLV, PLI/wPLI/dwPLI, imaginary
 * coherence, correlation, cross-correlation) and for the directed metrics
 * (Granger Causality, DTF, PDC); directionality is encoded by treating
 * @c (i, j) and @c (j, i) as distinct edges.
 *
 * Nodes and edges are stored contiguously by value, so a Network is a
 * regular value type: copies are deep and independent.
 */

#ifndef NETWORK_H
#define NETWORK_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"
#include "networkedge.h"
#include "networknode.h"

#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/**
 * @brief Per-network rendering hints: colour map or fixed RGBA for nodes and edges.
 *
 * @snippet ex_connectivity_metrics/main.cpp network_threshold
 */
struct VisualizationInfo
{
    /** How nodes and edges are coloured. */
    enum class Coloring
    {
        colormap, /**< Map node degree / edge weight through @ref colormap. */
        fixed     /**< Use @ref node_color and @ref edge_color. */
    };

    Coloring coloring = Coloring::colormap;                       /**< The colouring mode. */
    std::string colormap = "Viridis";                             /**< Colour map name used by Coloring::colormap. */
    Eigen::Vector4i node_color = Eigen::Vector4i(255, 0, 0, 255); /**< RGBA node colour used by Coloring::fixed. */
    Eigen::Vector4i edge_color = Eigen::Vector4i(255, 0, 0, 255); /**< RGBA edge colour used by Coloring::fixed. */
};

/** Selects all edges or only those passing the threshold. */
enum class EdgeSet
{
    full,       /**< All edges. */
    thresholded /**< Only active edges, i.e. |weight| >= threshold. */
};

/** Selects which incident edges of a node count. */
enum class Direction
{
    both, /**< Edges starting or ending at the node. */
    in,   /**< Edges ending at the node. */
    out   /**< Edges starting at the node. */
};

//=============================================================================================================
/**
 * Graph container for one functional-connectivity result.
 *
 * Holds the nodes (channel / source positions) and the edges (per-frequency
 * weight matrices + scalar band-averaged weights), the metric name ("COH",
 * "PLI", "GC", ...), the threshold that marks edges active, and the
 * @ref VisualizationInfo read by the disp3D rendering layer. Edges are
 * read-only once added so that the cached weight ranges stay consistent.
 *
 * @brief Graph container for one connectivity metric; nodes + weighted edges + threshold/visualisation state.
 *
 * @snippet ex_connectivity_metrics/main.cpp network_build
 *
 * @snippet ex_connectivity_metrics/main.cpp network_threshold
 */
class CONNECTIVITYSHARED_EXPORT Network
{
public:
    //=========================================================================================================
    /**
     * Constructs an empty network.
     *
     * @param[in] method       Name of the connectivity metric that produced the network.
     * @param[in] threshold    Edges with |weight| >= threshold are active.
     */
    explicit Network(std::string method = "Unknown", double threshold = 0.0);

    //=========================================================================================================
    /**
     * Reserves storage for the expected number of nodes and edges.
     *
     * @param[in] nodes    Expected node count.
     * @param[in] edges    Expected edge count.
     */
    void reserve(std::size_t nodes, std::size_t edges);

    //=========================================================================================================
    /**
     * Adds a node.
     *
     * @param[in] position    The 3D position of the node.
     *
     * @return The index of the new node.
     */
    int add_node(const Eigen::Vector3f& position);

    //=========================================================================================================
    /**
     * Adds an edge between two existing nodes and registers it with both. Self-loops are
     * ignored. The edge becomes active if |weight| >= @ref threshold.
     *
     * @param[in] edge    The edge; throws std::out_of_range unless source and target are valid node indices.
     */
    void add_edge(NetworkEdge edge);

    //=========================================================================================================
    /**
     * @return All nodes; a node's index is its position in this vector.
     */
    const std::vector<NetworkNode>& nodes() const noexcept
    {
        return m_nodes;
    }

    //=========================================================================================================
    /**
     * @return All edges, active or not.
     */
    const std::vector<NetworkEdge>& edges() const noexcept
    {
        return m_edges;
    }

    //=========================================================================================================
    /**
     * @param[in] set    Which edges count.
     *
     * @return The number of edges in @p set.
     */
    std::size_t edge_count(EdgeSet set = EdgeSet::full) const noexcept;

    //=========================================================================================================
    /**
     * @return Whether the network has no nodes or no edges.
     */
    bool empty() const noexcept
    {
        return m_nodes.empty() || m_edges.empty();
    }

    //=========================================================================================================
    /**
     * Returns the node-by-node adjacency matrix of edge weights.
     *
     * @param[in] set         Which edges to include.
     * @param[in] mirrored    Whether to also fill (target, source); otherwise only (source, target) is set.
     *
     * @return The adjacency matrix.
     */
    Eigen::MatrixXd connectivity_matrix(EdgeSet set = EdgeSet::full, bool mirrored = true) const;

    //=========================================================================================================
    /**
     * Returns the number of edges incident to a node.
     *
     * @param[in] node         The node index.
     * @param[in] set          Which edges count.
     * @param[in] direction    Which incident edges count.
     *
     * @return The degree.
     */
    int degree(int node, EdgeSet set = EdgeSet::full, Direction direction = Direction::both) const;

    //=========================================================================================================
    /**
     * Returns the summed weight of the edges incident to a node.
     *
     * @param[in] node         The node index.
     * @param[in] set          Which edges count.
     * @param[in] direction    Which incident edges count.
     *
     * @return The strength.
     */
    double strength(int node, EdgeSet set = EdgeSet::full, Direction direction = Direction::both) const;

    //=========================================================================================================
    /**
     * Returns the smallest and largest node degree.
     *
     * @param[in] set          Which edges count.
     * @param[in] direction    Which incident edges count.
     *
     * @return {min, max}; {0, 0} for a network without nodes.
     */
    std::pair<int, int> degree_range(EdgeSet set = EdgeSet::full, Direction direction = Direction::both) const;

    //=========================================================================================================
    /**
     * Returns the sum of all node degrees, i.e. twice the number of edges in @p set.
     *
     * @param[in] set    Which edges count.
     *
     * @return The total degree.
     */
    int total_degree(EdgeSet set = EdgeSet::full) const;

    //=========================================================================================================
    /**
     * Returns the range of edge weight magnitudes |weight|. For the full set this is the
     * smallest and largest magnitude; for the thresholded set it is {threshold, largest}.
     *
     * @param[in] set    Which edges count.
     *
     * @return {min, max}; {0, 0} for a network without edges.
     */
    std::pair<double, double> weight_range(EdgeSet set = EdgeSet::full) const noexcept;

    //=========================================================================================================
    /**
     * @return The name of the connectivity metric that produced the network.
     */
    const std::string& method() const noexcept
    {
        return m_method;
    }

    //=========================================================================================================
    /**
     * @param[in] method    The name of the connectivity metric.
     */
    void set_method(std::string method)
    {
        m_method = std::move(method);
    }

    //=========================================================================================================
    /**
     * @return The threshold that marks edges active.
     */
    double threshold() const noexcept
    {
        return m_threshold;
    }

    //=========================================================================================================
    /**
     * Sets the threshold and marks every edge with |weight| >= threshold active.
     *
     * @param[in] threshold    The new threshold.
     */
    void set_threshold(double threshold = 0.0);

    //=========================================================================================================
    /**
     * @return The frequency range in Hz the edge weights are averaged over.
     */
    std::pair<float, float> frequency_range() const noexcept
    {
        return m_frequency_range;
    }

    //=========================================================================================================
    /**
     * Averages every edge weight over a new frequency range and updates the weight range.
     * Requires @ref sampling_frequency and @ref used_freq_bins to be set; invalid ranges are
     * ignored with a warning.
     *
     * @param[in] lower    Lower edge in Hz.
     * @param[in] upper    Upper edge in Hz, at most the Nyquist frequency.
     */
    void set_frequency_range(float lower, float upper);

    //=========================================================================================================
    /**
     * Divides every edge weight and the threshold by the largest magnitude, so that weights
     * lie in [-1, 1] and the same edges stay active.
     */
    void normalize();

    //=========================================================================================================
    /**
     * @return The rendering hints.
     */
    const VisualizationInfo& visualization_info() const noexcept
    {
        return m_visualization_info;
    }

    //=========================================================================================================
    /**
     * @param[in] info    The new rendering hints.
     */
    void set_visualization_info(VisualizationInfo info)
    {
        m_visualization_info = std::move(info);
    }

    //=========================================================================================================
    /**
     * @return The sampling frequency of the data the network was computed from.
     */
    float sampling_frequency() const noexcept
    {
        return m_sfreq;
    }

    //=========================================================================================================
    /**
     * @param[in] sfreq    The sampling frequency.
     */
    void set_sampling_frequency(float sfreq) noexcept
    {
        m_sfreq = sfreq;
    }

    //=========================================================================================================
    /**
     * @return The number of frequency bins stored per edge.
     */
    int used_freq_bins() const noexcept
    {
        return m_used_freq_bins;
    }

    //=========================================================================================================
    /**
     * @param[in] bins    The number of frequency bins stored per edge.
     */
    void set_used_freq_bins(int bins) noexcept
    {
        m_used_freq_bins = bins;
    }

    //=========================================================================================================
    /**
     * @return The FFT size (number of bins of the positive half spectrum).
     */
    int fft_size() const noexcept
    {
        return m_fft_size;
    }

    //=========================================================================================================
    /**
     * @param[in] fft_size    The FFT size (number of bins of the positive half spectrum).
     */
    void set_fft_size(int fft_size) noexcept
    {
        m_fft_size = fft_size;
    }

private:
    bool counts(const NetworkEdge& edge, int node, EdgeSet set, Direction direction) const noexcept;

    std::vector<NetworkNode> m_nodes; /**< All nodes. */
    std::vector<NetworkEdge> m_edges; /**< All edges. */

    std::string m_method;                      /**< Name of the producing connectivity metric. */
    std::pair<double, double> m_weight_range;  /**< Smallest and largest |weight|. */
    std::pair<float, float> m_frequency_range; /**< Frequency range in Hz the weights are averaged over. */

    double m_threshold;       /**< Edges with |weight| >= threshold are active. */
    float m_sfreq = 0.0f;     /**< Sampling frequency of the source data. */
    int m_used_freq_bins = 0; /**< Number of frequency bins stored per edge. */
    int m_fft_size = 128;     /**< FFT size (positive half spectrum). */
    std::size_t m_active = 0; /**< Number of active edges. */

    VisualizationInfo m_visualization_info; /**< Rendering hints. */
};

} // namespace mne

#endif // NETWORK_H
