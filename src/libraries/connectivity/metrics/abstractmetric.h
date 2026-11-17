//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     abstractmetric.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January 2018
 * @brief    Common base of the functional-connectivity estimators in @c mne_connectivity.
 *
 * The frequency bins a spectral metric estimates and whether the per-trial spectra are kept for the next
 * call belong to each @ref mne::ConnectivitySettings, so independent analyses do not share state.
 */

#ifndef ABSTRACTMETRIC_H
#define ABSTRACTMETRIC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// mne FORWARD DECLARATIONS
//=============================================================================================================

class Network;

//=============================================================================================================
/**
 * @brief Common base of the connectivity metrics.
 *
 * @snippet ex_connectivity_metrics/main.cpp connectivity_calculate
 */
class CONNECTIVITYSHARED_EXPORT AbstractMetric
{
public:
    //=========================================================================================================
    /**
     * Constructs a AbstractMetric object.
     */
    explicit AbstractMetric();

protected:
    //=========================================================================================================
    /**
     * Adds one node per channel to a network, at the given position or at the origin when
     * fewer positions than channels are known.
     *
     * @param[in, out] network      The network to add the nodes to.
     * @param[in] count             Number of channels.
     * @param[in] positions         Channel positions, one row per channel; may be empty.
     */
    static void add_nodes(Network& network, int count, const Eigen::MatrixX3f& positions);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace mne

#endif // ABSTRACTMETRIC_H
