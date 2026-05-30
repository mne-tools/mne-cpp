//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     granger_causality.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Spectral Granger Causality (Geweke 1982, Bressler & Seth 2011) between every channel pair, computed from a fitted MVAR model.
 *
 * Granger's idea (Granger, Econometrica 1969) is that a process @c X_j
 * "causes" @c X_i if the past of @c X_j helps predict @c X_i beyond what
 * the past of @c X_i alone already does. Geweke (JASA 1982) gave the
 * frequency-resolved version used here:
 *
 *   GC_{j->i}(f) = ln( S_{ii}(f) /
 *                      ( S_{ii}(f) -
 *                        ( Sigma_{jj} - Sigma_{ij}^2 / Sigma_{ii} ) *
 *                        |H_{ij}(f)|^2 ) )
 *
 * with @c H the MVAR transfer function and @c Sigma the innovation
 * covariance, both supplied by @ref MvarModel. The output is non-negative
 * and asymmetric (@c GC_{j->i} != GC_{i->j} in general), so the resulting
 * @ref Network is directional. Spectral GC is the standard reference
 * directed measure for stationary linear systems and is the metric most
 * directly comparable to the @c spectral_connectivity_epochs(method='gc')
 * output produced by MNE-Python's mne-connectivity.
 *
 * Practical caveats are well known (Bressler & Seth, NeuroImage 2011):
 * the estimate is sensitive to MVAR model order, requires reasonably
 * stationary trial segments, and is biased by observation noise. The
 * complementary @ref DirectedTransferFunction and @ref PartialDirectedCoherence
 * metrics in this library are derived from the same MVAR fit and are
 * usually reported together.
 */

#ifndef GRANGERCAUSALITY_H
#define GRANGERCAUSALITY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

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
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {

//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

class Network;
class ConnectivitySettings;

//=============================================================================================================
/**
 * Spectral Granger Causality estimator (Geweke 1982 formulation).
 *
 * For every ordered channel pair @c (j -> i) the estimator fits an MVAR
 * model to the trial-concatenated time series via @ref MvarModel and
 * evaluates the log ratio of the unconditional and conditional spectra
 * of @c X_i,
 * @c GC_{j->i}(f) = ln( S_{ii}(f) / ( S_{ii}(f) - (Sigma_{jj} -
 * Sigma_{ij}^2 / Sigma_{ii}) * |H_{ij}(f)|^2 ) ).
 * The resulting @ref Network is directional and stores the band-averaged
 * GC value as edge weight.
 *
 * @brief Spectral Granger Causality estimator; directional, MVAR-based.
 * @since 2.2.0
 */
class CONNECTIVITYSHARED_EXPORT GrangerCausality : public AbstractMetric
{

public:
    typedef QSharedPointer<GrangerCausality> SPtr;            /**< Shared pointer type for GrangerCausality. */
    typedef QSharedPointer<const GrangerCausality> ConstSPtr; /**< Const shared pointer type for GrangerCausality. */

    //=========================================================================================================
    /**
     * Constructs a GrangerCausality object.
     */
    explicit GrangerCausality();

    //=========================================================================================================
    /**
     * Calculates spectral Granger causality between all channel pairs.
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
} // namespace CONNECTIVITYLIB

#endif // GRANGERCAUSALITY_H
