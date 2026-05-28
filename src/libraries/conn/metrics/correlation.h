//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file correlation.h
 * @since January 2018
 * @brief Pearson correlation coefficient between every pair of zero-lag time-domain channels.
 *
 * For two zero-mean, unit-variance signals @c x[t] and @c y[t] this is
 * simply
 *
 *   r_{xy} = E[ x[t] * y[t] ]
 *
 * with output in @c [-1, 1] - the time-domain, broadband, zero-lag
 * counterpart of magnitude-squared coherence. It is the simplest possible
 * functional-connectivity measure and is included here as a baseline:
 * it carries no spectral information, has no rejection of
 * volume-conduction / common-reference mixing, and is dominated by
 * whichever frequency band has the largest amplitude in the broadband
 * signal. For lag-resolved or band-limited interactions use
 * @ref CrossCorrelation, @ref Coherence or one of the phase-based
 * estimators.
 *
 * Trials are processed in parallel through @c QtConcurrent::mappedReduced;
 * the per-trial correlation matrices are summed in @ref reduce and divided
 * by the number of trials at the end to obtain the trial-averaged result.
 */

#ifndef CORRELATION_H
#define CORRELATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../conn_global.h"

#include "abstractmetric.h"
#include "../connectivitysettings.h"

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

//=============================================================================================================
/**
 * Computes the trial-averaged Pearson correlation coefficient between every
 * channel pair from the raw time-domain trial data.
 *
 * Per-trial correlation matrices are computed in parallel via
 * @c QtConcurrent::mappedReduced and summed in @ref reduce. The result is
 * a symmetric matrix in @c [-1, 1]; it is broadband, zero-lag and does
 * not reject volume conduction, so it is mainly useful as a baseline
 * against the spectral and phase-based estimators.
 *
 * @brief Pearson correlation estimator; broadband, zero-lag time-domain baseline.
 */
class CONNSHARED_EXPORT Correlation : public AbstractMetric
{

public:
    typedef QSharedPointer<Correlation> SPtr;            /**< Shared pointer type for Correlation. */
    typedef QSharedPointer<const Correlation> ConstSPtr; /**< Const shared pointer type for Correlation. */

    //=========================================================================================================
    /**
     * Constructs a Correlation object.
     */
    explicit Correlation();

    //=========================================================================================================
    /**
     * Calculates the correlation coefficient between the rows of the data matrix.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     */
    static Network calculate(ConnectivitySettings &connectivitySettings);

protected:
    //=========================================================================================================
    /**
     * Calculates the connectivity matrix for a given input data matrix based on the correlation coefficient.
     *
     * @param[in] inputData      The input data.
     *
     * @return                   The connectivity matrix.
     */
    static Eigen::MatrixXd compute(const ConnectivitySettings::IntermediateTrialData& inputData);

    //=========================================================================================================
    /**
     * Sums up (reduces) the in parallel processed connectivity matrix.
     *
     * @param[out] resultData    The result data.
     * @param[in] data          The incoming, temporary result data.
     */
    static void reduce(Eigen::MatrixXd &resultData,
                       const Eigen::MatrixXd &data);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // CORRELATION_H
