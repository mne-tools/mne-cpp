//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     coherency.h
 * @author   Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2018
 * @brief    Complex coherency between every channel pair and its two reductions: magnitude (coherence) and imaginary part (imaginary coherence).
 *
 * Coherency is the normalised cross-spectrum,
 *
 *   Coh_{xy}(f) = S_{xy}(f) / sqrt(S_{xx}(f) * S_{yy}(f))
 *
 * a complex-valued quantity whose magnitude is the classical coherence in
 * [0, 1] and whose argument is the average phase lag between the two
 * signals at frequency @c f. Splitting it into real and imaginary parts is
 * the basis of the volume-conduction-robust imaginary-coherence estimator
 * of Nolte et al. (NeuroImage, 2004): only non-zero phase lag (i.e. true
 * interaction with finite conduction delay) survives projection onto the
 * imaginary axis, while instantaneous common-reference / volume-conduction
 * mixing contributes only to the real axis and is rejected.
 *
 * The per-trial workhorse @ref Coherency::compute computes DPSS tapered
 * spectra and accumulates the cross-spectral and auto-spectral sums into
 * the shared @ref ConnectivitySettings::IntermediateSumData. The two
 * public reductions, @ref calculateAbs and @ref calculateImag, then divide
 * by the running auto-spectral norms and average over the frequency window
 * defined on @ref AbstractMetric to produce the scalar edge weights of the
 * returned @ref Network.
 */

#ifndef COHERENCY_H
#define COHERENCY_H

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
 * Shared core of the coherence / imaginary-coherence family.
 *
 * @ref compute fills the per-trial DPSS spectra and accumulates the cross-
 * and auto-spectral sums in @ref ConnectivitySettings::IntermediateSumData.
 * The two public reductions then collapse the complex coherency to a real
 * scalar per channel pair: @ref calculateAbs returns |Coh_{xy}(f)|^2
 * (classical magnitude-squared coherence, symmetric, sensitive to
 * zero-lag mixing) and @ref calculateImag returns |Im(Coh_{xy}(f))|, the
 * volume-conduction-robust imaginary coherence of Nolte et al. (2004).
 *
 * @brief Complex coherency core; produces magnitude-squared and imaginary-part reductions for downstream metrics.
 */
class CONNSHARED_EXPORT Coherency : public AbstractMetric
{

public:
    typedef QSharedPointer<Coherency> SPtr;            /**< Shared pointer type for Coherency. */
    typedef QSharedPointer<const Coherency> ConstSPtr; /**< Const shared pointer type for Coherency. */

    //=========================================================================================================
    /**
     * Constructs a Coherency object.
     */
    explicit Coherency();

    //=========================================================================================================
    /**
     * Calculates the absolute value of coherency of the rows of the data matrix.
     *
     * @param[out]   finalNetwork          The resulting network.
     * @param[in]   connectivitySettings  The input data and parameters.
     */
    static void calculateAbs(Network& finalNetwork,
                             ConnectivitySettings &connectivitySettings);

    //=========================================================================================================
    /**
     * Calculates the imaginary part of coherency of the rows of the data matrix.
     *
     * @param[out]   finalNetwork          The resulting network.
     * @param[in]   connectivitySettings  The input data and parameters.
     */
    static void calculateImag(Network& finalNetwork,
                              ConnectivitySettings &connectivitySettings);

private:
    //=========================================================================================================
    /**
     * Computes the coherency values. This function gets called in parallel.
     *
     * @param[in]   inputData           The input data.
     * @param[out]   matPsdSum           The sum of all PSD matrices for each trial.
     * @param[out]   vecPairCsdSum       The sum of all CSD matrices for each trial.
     * @param[in]   mutex               The mutex used to safely access matPsdSum and vecPairCsdSum.
     * @param[in]   iNRows              The number of rows.
     * @param[in]   iNFreqs             The number of frequenciy bins.
     * @param[in]   iNfft               The FFT length.
     * @param[in]   tapers              The taper information.
     */
    static void compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        Eigen::MatrixXd& matPsdSum,
                        QVector<QPair<int,Eigen::MatrixXcd> >& vecPairCsdSum,
                        QMutex& mutex,
                        int iNRows,
                        int iNFreqs,
                        int iNfft,
                        const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);

    //=========================================================================================================
    /**
     * Computes the PSD and CSD. This function gets called in parallel.
     */
    static void computePSDCSDAbs(QMutex& mutex,
                                 Network& finalNetwork,
                                 const QPair<int,Eigen::MatrixXcd>& pairInput,
                                 const Eigen::MatrixXd& matPsdSum);
    static void computePSDCSDImag(QMutex& mutex,
                                  Network& finalNetwork,
                                  const QPair<int,Eigen::MatrixXcd>& pairInput,
                                  const Eigen::MatrixXd& matPsdSum);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // COHERENCY_H
