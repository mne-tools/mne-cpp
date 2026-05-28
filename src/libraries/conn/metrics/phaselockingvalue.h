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
 * @file phaselockingvalue.h
 * @since April 2018
 * @brief Phase Locking Value (Lachaux, Rodriguez, Martinerie & Varela 1999) between every channel pair.
 *
 * The Phase Locking Value is the resultant length of the unit-modulus
 * cross-spectrum sample distribution,
 *
 *   PLV_{xy}(f) = | E[ S_{xy}(f) / |S_{xy}(f)| ] |
 *
 * with output in @c [0, 1]: 1 means the instantaneous phase difference
 * @c phi_x(f) - phi_y(f) is constant across trials (perfect inter-trial
 * phase locking at frequency @c f) and 0 means the phase differences are
 * uniformly distributed on the unit circle. Lachaux et al. (Human Brain
 * Mapping, 1999) introduced PLV to detect transient phase coupling
 * between visual-cortex sites in single-trial intracranial recordings
 * without the amplitude bias that affects coherence-based measures.
 *
 * Unlike imaginary coherence and the PLI family, PLV does not project out
 * the zero-lag component of the phase distribution: it is therefore
 * sensitive to genuine instantaneous coupling but cannot distinguish it
 * from common-reference / volume-conduction mixing. PLV is the right
 * choice for source-space analyses (where volume conduction is largely
 * absorbed into the inverse operator) and for paradigms in which zero-lag
 * locking is a hypothesis of interest; in sensor-space EEG/MEG it should
 * be reported alongside @ref ImagCoherence or @ref WeightedPhaseLagIndex.
 */

#ifndef PHASELOCKINGVALUE_H
#define PHASELOCKINGVALUE_H

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
 * Computes the Phase Locking Value of Lachaux et al. (HBM 1999).
 *
 * Each cross-spectral sample is divided by its magnitude (yielding a unit
 * complex number that encodes only the instantaneous phase difference)
 * and averaged across trials; PLV is the magnitude of that average. Per-
 * trial normalised cross-spectra are accumulated in
 * @ref ConnectivitySettings::IntermediateSumData and the resulting edge
 * weights are averaged over the frequency window defined on
 * @ref AbstractMetric.
 *
 * @brief Phase Locking Value estimator (Lachaux et al. 1999); amplitude-independent phase coupling, sensitive to zero-lag.
 */
class CONNSHARED_EXPORT PhaseLockingValue : public AbstractMetric
{

public:
    typedef QSharedPointer<PhaseLockingValue> SPtr;            /**< Shared pointer type for PhaseLockingValue. */
    typedef QSharedPointer<const PhaseLockingValue> ConstSPtr; /**< Const shared pointer type for PhaseLockingValue. */

    //=========================================================================================================
    /**
     * Constructs a PhaseLockingValue object.
     */
    explicit PhaseLockingValue();

    //=========================================================================================================
    /**
     * Calculates the phase locking value between the rows of the data matrix.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     */
    static Network calculate(ConnectivitySettings &connectivitySettings);

protected:
    //=========================================================================================================
    /**
     * Computes the PLV values. This function gets called in parallel.
     *
     * @param[in] inputData                  The input data.
     * @param[out]vecPairCsdSum              The sum of all CSD matrices for each trial.
     * @param[out]vecPairCsdNormalizedSum    The sum of all normalized CSD matrices for each trial.
     * @param[in] mutex                      The mutex used to safely access vecPairCsdSum.
     * @param[in] iNRows                     The number of rows.
     * @param[in] iNFreqs                    The number of frequenciy bins.
     * @param[in] iNfft                      The FFT length.
     * @param[in] tapers                     The taper information.
     */
    static void compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        QVector<QPair<int,Eigen::MatrixXcd> >& vecPairCsdSum,
                        QVector<QPair<int,Eigen::MatrixXcd> >& vecPairCsdNormalizedSum,
                        QMutex& mutex,
                        int iNRows,
                        int iNFreqs,
                        int iNfft,
                        const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);

    //=========================================================================================================
    /**
     * Reduces the PLV computation to a final result.
     *
     * @param[out] connectivitySettings   The input data.
     * @param[in] finalNetwork           The final network.
     */
    static void computePLV(ConnectivitySettings &connectivitySettings,
                           Network& finalNetwork);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // PHASELOCKINGVALUE_H
