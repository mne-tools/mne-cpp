//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Daniel Strohmeier <daniel.strohmeier@gmail.com>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file debiasedsquaredweightedphaselagindex.h
 * @since April 2018
 * @brief Debiased squared Weighted Phase Lag Index (Vinck et al., 2011) between every channel pair.
 *
 * The plain @ref WeightedPhaseLagIndex is still positively biased for
 * small numbers of trials @c N because the squared expectation in the
 * numerator picks up a diagonal variance term. Vinck, Oostenveld, van
 * Wingerden, Battaglia and Pennartz (NeuroImage 2011) derived the
 * analytic correction
 *
 *   dwPLI_{xy}(f) = ( ( sum_n Im(S_{xy}^{(n)}) )^2 - sum_n Im(S_{xy}^{(n)})^2 )
 *                   / ( ( sum_n |Im(S_{xy}^{(n)})| )^2 - sum_n Im(S_{xy}^{(n)})^2 )
 *
 * whose expectation equals @c wPLI^2 even for very small trial counts.
 * This is the recommended PLI-family estimator whenever the trial count
 * is low (single-subject epoched MEG/EEG, sliding-window time-resolved
 * connectivity, sub-second tapered segments) and is the volume-conduction-
 * robust counterpart of magnitude-squared coherence.
 *
 * The dwPLI is bounded in @c [0, 1] for any @c N once the analytic bias
 * is subtracted, and inherits the zero-lag rejection property of the PLI
 * family - any pair of channels driven by the same source with constant
 * gains has @c Im(S_{xy}) = 0 sample-by-sample and therefore @c dwPLI = 0
 * by construction.
 */

#ifndef DEBIASEDSQUAREDWEIGHTEDPHASELAGINDEX_H
#define DEBIASEDSQUAREDWEIGHTEDPHASELAGINDEX_H

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
#include <QMutex>

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
 * Computes the debiased squared Weighted Phase Lag Index of Vinck et al.
 * (NeuroImage 2011) for every channel pair.
 *
 * The estimator subtracts the analytically known diagonal variance from
 * both numerator and denominator of @ref WeightedPhaseLagIndex, producing
 * an estimator whose expectation equals @c wPLI^2 even for very small
 * trial counts. It is the preferred PLI-family metric when the number of
 * trials is low and is the volume-conduction-robust counterpart of
 * magnitude-squared coherence.
 *
 * @brief Debiased squared Weighted Phase Lag Index (Vinck et al. 2011); low-bias volume-conduction-robust estimator.
 */
class CONNSHARED_EXPORT DebiasedSquaredWeightedPhaseLagIndex : public AbstractMetric
{

public:
    typedef QSharedPointer<DebiasedSquaredWeightedPhaseLagIndex> SPtr;            /**< Shared pointer type for DebiasedSquaredWeightedPhaseLagIndex. */
    typedef QSharedPointer<const DebiasedSquaredWeightedPhaseLagIndex> ConstSPtr; /**< Const shared pointer type for DebiasedSquaredWeightedPhaseLagIndex. */

    //=========================================================================================================
    /**
     * Constructs a DebiasedSquaredWeightedPhaseLagIndex object.
     */
    explicit DebiasedSquaredWeightedPhaseLagIndex();

    //=========================================================================================================
    /**
     * Calculates the debiased squared weighted phase lag index between the rows of the data matrix.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     */
    static Network calculate(ConnectivitySettings &connectivitySettings);

protected:
    //=========================================================================================================
    /**
     * Computes the DSWPLI values. This function gets called in parallel.
     *
     * @param[in] inputData              The input data.
     * @param[out]vecPairCsdSum          The sum of all CSD matrices for each trial.
     * @param[out]vecPairCsdImagAbsSum   The sum of all imag abs CSD matrices for each trial.
     * @param[out]vecPairCsdImagSqrdSum  The sum of all imag aqrd CSD matrices for each trial.
     * @param[in] mutex                  The mutex used to safely access vecPairCsdSum.
     * @param[in] iNRows                 The number of rows.
     * @param[in] iNFreqs                The number of frequenciy bins.
     * @param[in] iNfft                  The FFT length.
     * @param[in] tapers                 The taper information.
     */
    static void compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        QVector<QPair<int,Eigen::MatrixXcd> >& vecPairCsdSum,
                        QVector<QPair<int,Eigen::MatrixXd> >& vecPairCsdImagAbsSum,
                        QVector<QPair<int,Eigen::MatrixXd> >& vecPairCsdImagSqrdSum,
                        QMutex& mutex,
                        int iNRows,
                        int iNFreqs,
                        int iNfft,
                        const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);

    //=========================================================================================================
    /**
     * Reduces the DSWPLI computation to a final result.
     *
     * @param[out] connectivitySettings   The input data.
     * @param[in] finalNetwork           The final network.
     */
    static void computeDSWPLI(ConnectivitySettings &connectivitySettings,
                              Network& finalNetwork);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // DEBIASEDSQUAREDWEIGHTEDPHASELAGINDEX_H
