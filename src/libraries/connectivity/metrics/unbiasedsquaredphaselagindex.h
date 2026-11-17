//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     unbiasedsquaredphaselagindex.h
 * @author   Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2018
 * @brief    Unbiased estimator of squared Phase Lag Index between every channel pair.
 *
 * The plain Phase Lag Index (Stam et al. 2007) and its square are biased
 * upward when the number of trials @c N is small: even uncorrelated signals
 * produce a non-zero expected magnitude of the empirical sign average
 * because each @c sign() contribution has variance @c 1. The unbiased
 * squared PLI removes the diagonal contribution analytically,
 *
 *   USPLI_{xy}(f) = ( ( sum_n sign(Im(S_{xy}^{(n)}(f))) )^2 - N ) / ( N * (N - 1) )
 *
 * which is an unbiased estimator of @c PLI^2 in the sense that
 * @c E[USPLI] = PLI^2 for any @c N >= 2. Values outside the nominal
 * @c [0, 1] range can occur for very small @c N or noise-dominated pairs;
 * these indicate a true PLI indistinguishable from zero at the available
 * sample size and are usually clipped to 0 for display purposes.
 *
 * This estimator is the direct phase-only analogue of the squared
 * debiased weighted PLI of Vinck et al. (2011) and is the right choice
 * when amplitude weighting is undesirable but small-sample bias must be
 * removed.
 */

#ifndef UNBIASEDSQUAREDPHASELAGINDEX_H
#define UNBIASEDSQUAREDPHASELAGINDEX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

#include <vector>
#include <utility>

#include "abstractmetric.h"
#include "../connectivitysettings.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutex>

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
 * Computes the unbiased estimator of squared PLI for every channel pair.
 *
 * The estimator subtracts the analytically known diagonal contribution
 * from the squared sum of cross-spectral sign samples, yielding an
 * estimate whose expectation equals @c PLI^2 for any trial count
 * @c N >= 2 and is therefore free of the upward small-sample bias that
 * affects the plain @ref PhaseLagIndex. The construction preserves the
 * zero-lag rejection property of the PLI family.
 *
 * @brief Unbiased squared Phase Lag Index estimator; removes the small-sample bias of @ref PhaseLagIndex.
 *
 * @snippet ex_connectivity_metrics/main.cpp unbiased_squared_phase_lag_index_calculate
 */
class CONNECTIVITYSHARED_EXPORT UnbiasedSquaredPhaseLagIndex : public AbstractMetric
{
public:
    //=========================================================================================================
    /**
     * Constructs a UnbiasedSquaredPhaseLagIndex object.
     */
    explicit UnbiasedSquaredPhaseLagIndex();

    //=========================================================================================================
    /**
     * Calculates the USPLI between the rows of the data matrix.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     */
    static Network calculate(ConnectivitySettings& connectivitySettings);

protected:
    //=========================================================================================================
    /**
     * Computes the PLI values. This function gets called in parallel.
     *
     * @param[in] inputData              The input data.
     * @param[out] vecPairCsdSum          The sum of all CSD matrices for each trial.
     * @param[out] vecPairCsdImagSignSum  The sum of all imag sign CSD matrices for each trial.
     * @param[in] mutex                  The mutex used to safely access vecPairCsdSum.
     * @param[in] iNRows                 The number of rows.
     * @param[in] iNFreqs                The number of frequenciy bins.
     * @param[in] iNfft                  The FFT length.
     * @param[in] tapers                 The taper information.
     * @param[in] binStart               First frequency bin of the half spectrum to use.
     * @param[in] binAmount              Number of frequency bins to use.
     * @param[in] keepTrialData          Whether to keep the tapered spectra and cross-spectra of the trial.
     */
    static void compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        std::vector<std::pair<int, Eigen::MatrixXcd>>& vecPairCsdSum,
                        std::vector<std::pair<int, Eigen::MatrixXd>>& vecPairCsdImagSignSum,
                        QMutex& mutex,
                        int iNRows,
                        int iNFreqs,
                        int iNfft,
                        const std::pair<Eigen::MatrixXd, Eigen::VectorXd>& tapers,
                        int binStart,
                        int binAmount,
                        bool keepTrialData);

    //=========================================================================================================
    /**
     * Reduces the USPLI computation to a final result.
     *
     * @param[out] connectivitySettings   The input data.
     * @param[in] finalNetwork           The final network.
     */
    static void computeUSPLI(ConnectivitySettings& connectivitySettings,
                             Network& finalNetwork);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace mne

#endif // UNBIASEDSQUAREDPHASELAGINDEX_H
