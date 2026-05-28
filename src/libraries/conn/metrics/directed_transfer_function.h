//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file directed_transfer_function.h
 * @since April 2026
 * @brief Directed Transfer Function (Kaminski & Blinowska 1991) between every channel pair, derived from a fitted MVAR transfer function.
 *
 * The DTF normalises each row of the squared MVAR transfer matrix to a
 * unit sum,
 *
 *   DTF_{ij}(f) = |H_{ij}(f)|^2 / sum_k |H_{ik}(f)|^2
 *
 * which can be read as the fraction of the spectral power that channel
 * @c i would have at frequency @c f that originates from channel @c j
 * relative to all other channels in the model. Output is non-negative,
 * bounded by 1, and asymmetric (@c DTF_{ij} != DTF_{ji} in general).
 * Kaminski and Blinowska (Biological Cybernetics, 1991) introduced DTF
 * for multi-channel EEG to track stimulus-driven propagation patterns
 * across cortex, and it remains a popular reference for directed flow in
 * multivariate spectral analysis.
 *
 * Unlike @ref PartialDirectedCoherence, DTF includes both direct and
 * indirect causal pathways: an indirect interaction @c j -> k -> i shows
 * up as a non-zero @c DTF_{ij}. The two metrics are therefore
 * complementary and are usually reported alongside @ref GrangerCausality,
 * with all three computed from the same @ref MvarModel fit.
 */

#ifndef DIRECTEDTRANSFERFUNCTION_H
#define DIRECTEDTRANSFERFUNCTION_H

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
 * Directed Transfer Function estimator of Kaminski & Blinowska (1991).
 *
 * Computes @c DTF_{ij}(f) = |H_{ij}(f)|^2 / sum_k |H_{ik}(f)|^2 from the
 * transfer matrix @c H delivered by @ref MvarModel and reduces it to a
 * scalar edge weight by averaging over the frequency window defined on
 * @ref AbstractMetric. The resulting @ref Network is directional and
 * includes both direct and indirect causal pathways - use
 * @ref PartialDirectedCoherence for direct-only flow.
 *
 * @brief Directed Transfer Function estimator (Kaminski & Blinowska 1991); directional, includes indirect paths.
 * @since 2.2.0
 */
class CONNSHARED_EXPORT DirectedTransferFunction : public AbstractMetric
{

public:
    typedef QSharedPointer<DirectedTransferFunction> SPtr;            /**< Shared pointer type for DirectedTransferFunction. */
    typedef QSharedPointer<const DirectedTransferFunction> ConstSPtr; /**< Const shared pointer type for DirectedTransferFunction. */

    //=========================================================================================================
    /**
     * Constructs a DirectedTransferFunction object.
     */
    explicit DirectedTransferFunction();

    //=========================================================================================================
    /**
     * Calculates the Directed Transfer Function between all channel pairs.
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

#endif // DIRECTEDTRANSFERFUNCTION_H
