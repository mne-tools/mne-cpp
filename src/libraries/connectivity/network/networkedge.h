//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     networkedge.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August 2016
 * @brief    Weighted edge between two nodes of a @ref mne::Network; stores the full per-frequency weight matrix and the scalar band-averaged weight.
 *
 * Every functional-connectivity estimator in @c mne_connectivity produces one
 * @ref mne::NetworkEdge per ordered channel pair (or per unordered pair for
 * symmetric metrics) and adds it to the @ref mne::Network. The edge owns
 * the full weight matrix returned by the metric - one row per frequency bin -
 * and an inclusive @c [first, last] frequency-bin window that reduces the
 * matrix to the single scalar @c weight used by thresholding and display.
 */

#ifndef NETWORKEDGE_H
#define NETWORKEDGE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

#include <utility>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Directed weighted edge between the nodes with indices @ref source and
 * @ref target. Holds the full per-frequency weight matrix delivered by the
 * metric, the frequency-bin window that reduces it to the scalar
 * @ref weight, and the active flag set by @ref Network::set_threshold.
 * For symmetric metrics the direction is a storage convention only.
 *
 * NetworkEdge is a plain value type: copies are independent.
 *
 * @brief Weighted, directional edge in a @ref Network; carries per-frequency weights plus a band-averaged scalar.
 *
 * @snippet ex_connectivity_metrics/main.cpp network_build
 */
class CONNECTIVITYSHARED_EXPORT NetworkEdge
{
public:
    //=========================================================================================================
    /**
     * Constructs an edge and computes its scalar weight.
     *
     * @param[in] source     Index of the start node.
     * @param[in] target     Index of the end node.
     * @param[in] weights    Per-frequency weights, one row per bin. An empty matrix becomes a 1x1 zero matrix.
     * @param[in] bins       Inclusive frequency-bin window to average; {-1, -1} averages all weights.
     */
    NetworkEdge(int source, int target, Eigen::MatrixXd weights, std::pair<int, int> bins = {-1, -1});

    //=========================================================================================================
    /**
     * @return Index of the start node.
     */
    int source() const noexcept
    {
        return m_source;
    }

    //=========================================================================================================
    /**
     * @return Index of the end node.
     */
    int target() const noexcept
    {
        return m_target;
    }

    //=========================================================================================================
    /**
     * @return Whether the edge passes the threshold of its network.
     */
    bool active() const noexcept
    {
        return m_active;
    }

    //=========================================================================================================
    /**
     * Sets the active flag.
     *
     * @param[in] active    The new flag.
     */
    void set_active(bool active) noexcept
    {
        m_active = active;
    }

    //=========================================================================================================
    /**
     * @return The scalar weight averaged over the frequency-bin window.
     */
    double weight() const noexcept
    {
        return m_weight;
    }

    //=========================================================================================================
    /**
     * Overrides the scalar weight.
     *
     * @param[in] weight    The new scalar weight.
     */
    void set_weight(double weight) noexcept
    {
        m_weight = weight;
    }

    //=========================================================================================================
    /**
     * @return The per-frequency weights, one row per bin.
     */
    const Eigen::MatrixXd& weights() const noexcept
    {
        return m_weights;
    }

    //=========================================================================================================
    /**
     * @return The inclusive frequency-bin window the scalar weight is averaged over; {-1, -1} means all bins.
     */
    std::pair<int, int> frequency_bins() const noexcept
    {
        return m_bins;
    }

    //=========================================================================================================
    /**
     * Sets the frequency-bin window and recomputes the scalar weight. Invalid windows
     * (last before first, or indices below -1) are stored but leave the weight unchanged.
     * A window reaching past the last bin is clipped.
     *
     * @param[in] bins    The inclusive window; {-1, -1} averages all bins.
     */
    void set_frequency_bins(std::pair<int, int> bins);

private:
    void average();

    Eigen::MatrixXd m_weights;  /**< Per-frequency weights, one row per bin. */
    std::pair<int, int> m_bins; /**< Inclusive frequency-bin window of the scalar weight. */
    double m_weight = 0.0;      /**< Scalar weight averaged over m_bins. */
    int m_source;               /**< Index of the start node. */
    int m_target;               /**< Index of the end node. */
    bool m_active = true;       /**< Whether the edge passes the network threshold. */
};

} // namespace mne

#endif // NETWORKEDGE_H
