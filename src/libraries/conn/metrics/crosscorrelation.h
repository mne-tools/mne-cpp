//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file crosscorrelation.h
 * @since 2026
 * @date  March 2026
 * @brief Time-lagged cross-correlation between every channel pair, computed via the inverse FFT of the cross-spectrum.
 *
 * For two zero-mean signals @c x and @c y the cross-correlation is
 *
 *   c_{xy}(tau) = E[ x[t] * y[t + tau] ]
 *
 * which by the Wiener-Khinchin theorem equals the inverse Fourier
 * transform of the cross-spectrum @c S_{xy}(f). This implementation
 * therefore reuses the DPSS-tapered FFTs computed by the spectral metrics
 * and runs an @c IFFT on @c S_{xy} to obtain @c c_{xy}(tau) for all lags
 * in @c [-Nfft/2, Nfft/2); the edge weight stored on the @ref Network is
 * the maximum absolute correlation over the lag axis, which gives a
 * lag-robust scalar similarity measure suitable for detecting delayed
 * coupling that zero-lag @ref Correlation would miss.
 *
 * Like @ref Correlation this is a broadband time-domain estimator with no
 * rejection of volume conduction or common-reference mixing; the lag at
 * which the maximum occurs can however be inspected separately and used
 * as a coarse conduction-delay estimate.
 */

#ifndef CROSSCORRELATION_H
#define CROSSCORRELATION_H

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
 * Computes the time-lagged cross-correlation between every channel pair via
 * the inverse FFT of the DPSS-tapered cross-spectrum, and reduces it to
 * the maximum absolute correlation over the lag axis.
 *
 * Per-trial computations are dispatched through @c QtConcurrent::mapped
 * and accumulated under a single @c QMutex. The estimator is broadband
 * and does not reject zero-lag mixing, but - unlike @ref Correlation -
 * is sensitive to delayed coupling because the lag axis is searched.
 *
 * @brief Time-lagged cross-correlation estimator; broadband, sensitive to delayed coupling.
 */
class CONNSHARED_EXPORT CrossCorrelation : public AbstractMetric
{

public:
    typedef QSharedPointer<CrossCorrelation> SPtr;            /**< Shared pointer type for CrossCorrelation. */
    typedef QSharedPointer<const CrossCorrelation> ConstSPtr; /**< Const shared pointer type for CrossCorrelation. */

    //=========================================================================================================
    /**
     * Constructs a CrossCorrelation object.
     */
    explicit CrossCorrelation();

    //=========================================================================================================
    /**
     * Calculates the cross correlation between the rows of the data matrix.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     */
    static Network calculate(ConnectivitySettings &connectivitySettings);

protected:
    //=========================================================================================================
    /**
     * Calculates the connectivity matrix for a given input data matrix based on the cross correlation coefficient.
     *
     * @param[in]   inputData           The input data.
     * @param[out]   matDist             The sum of all edge weights.
     * @param[in]   mutex               The mutex used to safely access matDist.
     * @param[in]   iNfft               The FFT length.
     * @param[in]   tapers              The taper information.
     */
    static void compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        Eigen::MatrixXd& matDist,
                        QMutex& mutex,
                        int iNfft,
                        const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // CROSSCORRELATION_H
