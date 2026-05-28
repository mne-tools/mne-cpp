//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file partial_directed_coherence.h
 * @since April 2026
 * @brief Partial Directed Coherence (Baccala & Sameshima 2001) between every channel pair, derived from a fitted MVAR model.
 *
 * PDC works in the @c z-domain on the inverse of the MVAR transfer
 * function, @c A(f) = I - sum_{k=1}^{p} A_k * exp(-2*pi*i*f*k), and
 * column-normalises its magnitudes,
 *
 *   PDC_{ij}(f) = |A_{ij}(f)| / sqrt( sum_k |A_{kj}(f)|^2 )
 *
 * giving the proportion of the outflow from channel @c j at frequency
 * @c f that targets channel @c i directly. Output is in @c [0, 1] and
 * asymmetric. Baccala and Sameshima (Biological Cybernetics, 2001)
 * introduced PDC explicitly to separate direct from indirect causal
 * pathways: a relay @c j -> k -> i contributes 0 to @c PDC_{ij} because
 * the direct coefficient @c A_{ij}(f) is zero, in contrast to
 * @ref DirectedTransferFunction which would still flag the indirect
 * route.
 *
 * PDC therefore plays the role that partial correlation plays in static
 * Gaussian models: it removes the influence of all other channels in the
 * MVAR system before measuring the @c j -> i interaction. Like DTF and
 * @ref GrangerCausality, PDC reuses the @ref MvarModel fit, so requesting
 * all three metrics in one batch costs only one Levinson-Durbin solve.
 */

#ifndef PARTIALDIRECTEDCOHERENCE_H
#define PARTIALDIRECTEDCOHERENCE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../conn_global.h"

#include "abstractmetric.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE CONNLIB
//=============================================================================================================

namespace CONNLIB {

//=============================================================================================================
// CONNLIB FORWARD DECLARATIONS
//=============================================================================================================

class Network;
class ConnectivitySettings;

//=============================================================================================================
/**
 * Partial Directed Coherence estimator of Baccala & Sameshima (2001).
 *
 * Computes @c PDC_{ij}(f) = |A_{ij}(f)| / sqrt( sum_k |A_{kj}(f)|^2 ) from
 * the MVAR coefficients delivered by @ref MvarModel, where @c A(f) is the
 * frequency-domain inverse of the transfer matrix. Unlike
 * @ref DirectedTransferFunction this estimator counts only direct causal
 * paths - relayed interactions @c j -> k -> i contribute 0 - so PDC and
 * DTF together separate direct from indirect flow.
 *
 * @brief Partial Directed Coherence estimator (Baccala & Sameshima 2001); directional, direct paths only.
 * @since 2.2.0
 */
class CONNSHARED_EXPORT PartialDirectedCoherence : public AbstractMetric
{

public:
    typedef QSharedPointer<PartialDirectedCoherence> SPtr;            /**< Shared pointer type for PartialDirectedCoherence. */
    typedef QSharedPointer<const PartialDirectedCoherence> ConstSPtr; /**< Const shared pointer type for PartialDirectedCoherence. */

    //=========================================================================================================
    /**
     * Constructs a PartialDirectedCoherence object.
     */
    explicit PartialDirectedCoherence();

    //=========================================================================================================
    /**
     * Calculates Partial Directed Coherence between all channel pairs.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     *
     * @since 2.2.0
     */
    static Network calculate(ConnectivitySettings &connectivitySettings);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // PARTIALDIRECTEDCOHERENCE_H
