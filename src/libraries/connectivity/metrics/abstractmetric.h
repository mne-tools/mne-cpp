//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     abstractmetric.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January 2018
 * @brief    Common static knobs shared by every functional-connectivity estimator in @c mne_connectivity.
 *
 * Every metric in @c mne_connectivity (Coherence, Imaginary Coherence, PLI, wPLI,
 * dwPLI, USPLI, PLV, cross-correlation, Granger Causality, DTF, PDC) reads
 * the same global control flags from @ref mne::AbstractMetric so that the
 * dispatcher in @ref mne::compute_connectivity can decide once - before any FFT runs -
 * whether intermediate per-trial spectra should be cached for reuse
 * (storage mode) and which frequency-bin window the metrics should
 * average over.
 *
 * The frequency-bin window is expressed as @c [m_iNumberBinStart,
 * m_iNumberBinStart + m_iNumberBinAmount). All metrics collapse the
 * spectral output to a single scalar per channel pair by averaging the
 * magnitude (or squared magnitude, depending on the metric) over exactly
 * this window, so callers select an EEG band (e.g. alpha 8-12 Hz) once and
 * every estimator returns a comparable band-limited connectivity matrix.
 */

#ifndef ABSTRACTMETRIC_H
#define ABSTRACTMETRIC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

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
 * Static base for every estimator in @c mne_connectivity. Holds the two pieces of
 * global state that the dispatcher (@ref compute_connectivity) must set
 * before any metric runs:
 *  - @c mne::AbstractMetric::m_bStorageModeIsActive "m_bStorageModeIsActive" enables caching of per-trial tapered
 *    spectra and CSDs in @ref ConnectivitySettings::IntermediateTrialData,
 *    which lets a second metric reuse the FFT work done by the first.
 *  - @c mne::AbstractMetric::m_iNumberBinStart "m_iNumberBinStart" and @c mne::AbstractMetric::m_iNumberBinAmount "m_iNumberBinAmount" define the
 *    frequency-bin window over which each metric averages its spectral
 *    output into the single scalar weight stored on each network edge.
 *
 * @brief Static control knobs (storage mode, frequency band) shared by all mne_connectivity metrics.
 *
 * @snippet ex_connectivity_metrics/main.cpp abstract_metric_band
 */
class CONNECTIVITYSHARED_EXPORT AbstractMetric
{
public:
    //=========================================================================================================
    /**
     * Constructs a AbstractMetric object.
     */
    explicit AbstractMetric();

    static bool m_bStorageModeIsActive;
    static int m_iNumberBinStart;
    static int m_iNumberBinAmount;

protected:
    //=========================================================================================================
    /**
     * Adds one node per channel to a network, at the given position or at the origin when
     * fewer positions than channels are known.
     *
     * @param[in, out] network      The network to add the nodes to.
     * @param[in] count             Number of channels.
     * @param[in] positions         Channel positions, one row per channel; may be empty.
     */
    static void add_nodes(Network& network, int count, const Eigen::MatrixX3f& positions);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace mne

#endif // ABSTRACTMETRIC_H
