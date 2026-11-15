//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     networknode.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August 2016
 * @brief    Node of a connectivity @ref mne::Network; carries a 3D position and the indices of its incident edges.
 *
 * One @ref mne::NetworkNode exists per sensor (in sensor-space connectivity)
 * or per source vertex / ROI centroid (in source-space connectivity). The
 * position comes from @c mne::ConnectivitySettings::setNodePositions "ConnectivitySettings::setNodePositions"
 * and is used by disp3D to draw the node at its anatomical location.
 */

#ifndef NETWORKNODE_H
#define NETWORKNODE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

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

//=============================================================================================================
/**
 * Node of a @ref mne::Network "Network": a 3D position (sensor coordinate or source vertex),
 * a hub flag, and the indices into @ref mne::Network::edges "Network::edges" of every edge that
 * starts or ends at this node. Degree and strength are computed by
 * @ref Network::degree and @ref Network::strength.
 *
 * @brief Graph node carrying a 3D position and the indices of its incident edges.
 *
 * @snippet ex_connectivity_metrics/main.cpp network_build
 */
class CONNECTIVITYSHARED_EXPORT NetworkNode
{
public:
    //=========================================================================================================
    /**
     * Constructs a node without edges.
     *
     * @param[in] position    The 3D position of the node.
     */
    explicit NetworkNode(const Eigen::Vector3f& position)
    : m_position(position)
    {
    }

    //=========================================================================================================
    /**
     * @return The 3D position of the node.
     */
    const Eigen::Vector3f& position() const noexcept
    {
        return m_position;
    }

    //=========================================================================================================
    /**
     * @return Indices into @ref Network::edges of the edges starting or ending at this node.
     */
    const std::vector<int>& edges() const noexcept
    {
        return m_edges;
    }

    //=========================================================================================================
    /**
     * @return Whether the node is marked as a hub.
     */
    bool hub() const noexcept
    {
        return m_hub;
    }

    //=========================================================================================================
    /**
     * Marks the node as hub or not.
     *
     * @param[in] hub    The new flag.
     */
    void set_hub(bool hub) noexcept
    {
        m_hub = hub;
    }

private:
    friend class Network;

    Eigen::Vector3f m_position; /**< The 3D position. */
    std::vector<int> m_edges;   /**< Indices of the incident edges. */
    bool m_hub = false;         /**< Hub flag. */
};

} // namespace mne

#endif // NETWORKNODE_H
