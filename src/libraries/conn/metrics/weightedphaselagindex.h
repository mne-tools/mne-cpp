//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file weightedphaselagindex.h
 * @since 2026
 * @date  March 2026
 * @brief Weighted Phase Lag Index (Vinck, Oostenveld, van Wingerden, Battaglia & Pennartz 2011) between every channel pair.
 *
 * The weighted PLI replaces the unweighted sign average used by @ref PhaseLagIndex
 * with an imaginary-part-magnitude-weighted version,
 *
 *   wPLI_{xy}(f) = | E[ Im(S_{xy}(f)) ] | / E[ |Im(S_{xy}(f))| ]
 *
 * Vinck et al. (NeuroImage 2011) showed that this estimator dominates the
 * plain PLI on three counts: it is less sensitive to phase fluctuations
 * around the zero-lag line (because near-zero @c Im(S_{xy}) values
 * contribute proportionally little to both numerator and denominator), it
 * has lower sample-size bias, and it has higher statistical power to
 * detect genuine non-zero lag interactions in noisy MEG/EEG. Like PLI and
 * imaginary coherence, the construction guarantees rejection of zero-lag
 * volume-conduction / common-reference mixing.
 *
 * The output is bounded in @c [0, 1] and is still positively biased for
 * small numbers of trials. The squared, debiased variant
 * @ref DebiasedSquaredWeightedPhaseLagIndex - also from Vinck et al.
 * (2011) - removes that small-sample bias analytically and should be
 * preferred when the trial count is low.
 */

#ifndef WEIGHTEDPHASELAGINDEX_H
#define WEIGHTEDPHASELAGINDEX_H

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
 * Computes the Weighted Phase Lag Index of Vinck et al. (NeuroImage 2011).
 *
 * Each channel pair is reduced to
 * @c |sum Im(S_{xy})| / sum |Im(S_{xy})|, weighting every cross-spectrum
 * sample by the magnitude of its imaginary part. This down-weights
 * near-zero-lag samples that dominate the bias of the plain @ref PhaseLagIndex
 * and yields higher detection power for true delayed interactions while
 * preserving the volume-conduction rejection property. Per-trial sums are
 * accumulated in @ref ConnectivitySettings::IntermediateSumData and shared
 * with @ref DebiasedSquaredWeightedPhaseLagIndex when both metrics are
 * requested in one batch.
 *
 * @brief Weighted Phase Lag Index estimator (Vinck et al. 2011); volume-conduction-robust with lower bias than PLI.
 */
class CONNSHARED_EXPORT WeightedPhaseLagIndex : public AbstractMetric
{

public:
    typedef QSharedPointer<WeightedPhaseLagIndex> SPtr;            /**< Shared pointer type for WeightedPhaseLagIndex. */
    typedef QSharedPointer<const WeightedPhaseLagIndex> ConstSPtr; /**< Const shared pointer type for WeightedPhaseLagIndex. */

    //=========================================================================================================
    /**
     * Constructs a WeightedPhaseLagIndex object.
     */
    explicit WeightedPhaseLagIndex();

    //=========================================================================================================
    /**
     * Calculates the WPLI between the rows of the data matrix.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     */
    static Network calculate(ConnectivitySettings& connectivitySettings);

protected:
    //=========================================================================================================
    /**
     * Computes the WPLI values. This function gets called in parallel.
     *
     * @param[in] inputData              The input data.
     * @param[out]vecPairCsdSum          The sum of all CSD matrices for each trial.
     * @param[out]vecPairCsdImagAbsSum   The sum of all imag abs CSD matrices for each trial.
     * @param[in] mutex                  The mutex used to safely access vecPairCsdSum.
     * @param[in] iNRows                 The number of rows.
     * @param[in] iNFreqs                The number of frequenciy bins.
     * @param[in] iNfft                  The FFT length.
     * @param[in] tapers                 The taper information.
     */
    static void compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        QVector<QPair<int,Eigen::MatrixXcd> >& vecPairCsdSum,
                        QVector<QPair<int,Eigen::MatrixXd> >& vecPairCsdImagAbsSum,
                        QMutex& mutex,
                        int iNRows,
                        int iNFreqs,
                        int iNfft,
                        const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);

    //=========================================================================================================
    /**
     * Reduces the WPLI computation to a final result.
     *
     * @param[out] connectivitySettings   The input data.
     * @param[in] finalNetwork           The final network.
     */
    static void computeWPLI(ConnectivitySettings &connectivitySettings,
                            Network& finalNetwork);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // WEIGHTEDPHASELAGINDEX_H
