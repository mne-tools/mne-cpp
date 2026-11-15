//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     networkedge.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August 2016
 * @brief    Implementation of @ref mne::NetworkEdge - directional weighted edge with per-frequency weight matrix and configurable band-averaging window.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "networkedge.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkEdge::NetworkEdge(int source, int target, MatrixXd weights, std::pair<int, int> bins)
: m_weights(weights.size() == 0 ? MatrixXd::Zero(1, 1) : std::move(weights))
, m_bins(bins)
, m_source(source)
, m_target(target)
{
    average();
}

//=============================================================================================================

void NetworkEdge::set_frequency_bins(std::pair<int, int> bins)
{
    m_bins = bins;
    average();
}

//=============================================================================================================

void NetworkEdge::average()
{
    const auto [first, last] = m_bins;

    if (last < first || first < -1) {
        return;
    }

    const auto rows = m_weights.rows();

    if (first == -1) {
        m_weight = m_weights.mean();
    } else if (first < rows) {
        const auto end = std::min<Index>(last + 1, rows);
        m_weight = m_weights.middleRows(first, end - first).col(0).mean();
    }
}
