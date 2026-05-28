//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Daniel Strohmeier <daniel.strohmeier@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file phaselagindex.h
 * @since January 2018
 * @brief Phase Lag Index (Stam, Nolte & Daffertshofer 2007) between every channel pair.
 *
 * The Phase Lag Index measures the asymmetry of the cross-spectral phase
 * distribution about zero,
 *
 *   PLI_{xy}(f) = | E[ sign( Im( S_{xy}(f) ) ) ] |
 *
 * with output in @c [0, 1]: 0 means phase differences are symmetric about
 * 0 or pi (i.e. dominated by zero-lag mixing) and 1 means the phase lag
 * always has the same sign (perfect, consistently lagged or leading
 * interaction). Stam, Nolte and Daffertshofer (Human Brain Mapping, 2007)
 * introduced PLI specifically to suppress the spurious connectivity that
 * volume conduction and common-reference effects produce in EEG/MEG
 * sensor-space coherence: any pair of channels driven by the same source
 * with constant gains has a phase difference of exactly 0 or pi and
 * therefore PLI = 0 by construction.
 *
 * PLI shares the anti-leakage property of imaginary coherence but, being
 * amplitude-independent, is more robust to amplitude artefacts at the cost
 * of being biased upward for small samples and discontinuous around the
 * zero-phase line. The weighted variants @ref WeightedPhaseLagIndex and
 * @ref DebiasedSquaredWeightedPhaseLagIndex (Vinck et al., 2011) and the
 * sample-bias correction @ref UnbiasedSquaredPhaseLagIndex address those
 * shortcomings.
 */

#ifndef PHASELAGINDEX_H
#define PHASELAGINDEX_H

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
 * Computes the Phase Lag Index of Stam, Nolte & Daffertshofer (2007).
 *
 * For every channel pair the estimator averages @c sign(Im(S_{xy}(f)))
 * across DPSS-tapered trial spectra and returns the magnitude of that
 * average. Zero-lag mixing (volume conduction, common reference) yields
 * sign = 0 and is rejected; consistently lagged or leading interactions
 * yield magnitudes close to 1. The per-trial accumulation is parallelised
 * over trials via @c QtConcurrent and reduced under a single @c QMutex.
 *
 * @brief Phase Lag Index estimator (Stam et al. 2007); rejects zero-lag volume-conduction mixing.
 */
class CONNSHARED_EXPORT PhaseLagIndex : public AbstractMetric
{

public:
    typedef QSharedPointer<PhaseLagIndex> SPtr;            /**< Shared pointer type for PhaseLagIndex. */
    typedef QSharedPointer<const PhaseLagIndex> ConstSPtr; /**< Const shared pointer type for PhaseLagIndex. */

    //=========================================================================================================
    /**
     * Constructs a PhaseLagIndex object.
     */
    explicit PhaseLagIndex();

    //=========================================================================================================
    /**
     * Calculates the PLI between the rows of the data matrix.
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
     * @param[out]vecPairCsdSum          The sum of all CSD matrices for each trial.
     * @param[out]vecPairCsdImagSignSum  The sum of all imag sign CSD matrices for each trial.
     * @param[in] mutex                  The mutex used to safely access vecPairCsdSum.
     * @param[in] iNRows                 The number of rows.
     * @param[in] iNFreqs                The number of frequenciy bins.
     * @param[in] iNfft                  The FFT length.
     * @param[in] tapers                 The taper information.
     */
    static void compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        QVector<QPair<int,Eigen::MatrixXcd> >& vecPairCsdSum,
                        QVector<QPair<int,Eigen::MatrixXd> >& vecPairCsdImagSignSum,
                        QMutex& mutex,
                        int iNRows,
                        int iNFreqs,
                        int iNfft,
                        const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);

    //=========================================================================================================
    /**
     * Reduces the PLI computation to a final result.
     *
     * @param[out] connectivitySettings   The input data.
     * @param[in] finalNetwork           The final network.
     */
    static void computePLI(ConnectivitySettings &connectivitySettings,
                          Network& finalNetwork);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // PHASELAGINDEX_H
