//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     abstractmetric.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January 2018
 * @brief    Implementation of @ref mne::AbstractMetric - holds the static control knobs (storage-mode flag and band-averaging window) shared by every mne_connectivity estimator.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstractmetric.h"

#include "../network/network.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AbstractMetric::AbstractMetric()
{
}

//=============================================================================================================

void AbstractMetric::add_nodes(Network& network, int count, const Eigen::MatrixX3f& positions)
{
    network.reserve(static_cast<std::size_t>(count), static_cast<std::size_t>(count) * static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        network.add_node(i < positions.rows() ? Eigen::Vector3f(positions.row(i).transpose()) : Eigen::Vector3f::Zero());
    }
}
