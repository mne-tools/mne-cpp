//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file abstractmetric.h
 * @since 2026
 * @date  March 2026
 * @brief Common static knobs shared by every functional-connectivity estimator in @c CONNLIB.
 *
 * Every metric in @c CONNLIB (Coherence, Imaginary Coherence, PLI, wPLI,
 * dwPLI, USPLI, PLV, cross-correlation, Granger Causality, DTF, PDC) reads
 * the same global control flags from @ref AbstractMetric so that the
 * dispatcher in @ref Connectivity can decide once - before any FFT runs -
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

#include "../conn_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>

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

//=============================================================================================================
/**
 * Static base for every estimator in @c CONNLIB. Holds the two pieces of
 * global state that the dispatcher (@ref Connectivity::calculate) must set
 * before any metric runs:
 *  - @ref m_bStorageModeIsActive enables caching of per-trial tapered
 *    spectra and CSDs in @ref ConnectivitySettings::IntermediateTrialData,
 *    which lets a second metric reuse the FFT work done by the first.
 *  - @ref m_iNumberBinStart and @ref m_iNumberBinAmount define the
 *    frequency-bin window over which each metric averages its spectral
 *    output into the single scalar weight stored on each network edge.
 *
 * @brief Static control knobs (storage mode, frequency band) shared by all CONNLIB metrics.
 */
class CONNSHARED_EXPORT AbstractMetric
{

public:
    typedef QSharedPointer<AbstractMetric> SPtr;            /**< Shared pointer type for AbstractMetric. */
    typedef QSharedPointer<const AbstractMetric> ConstSPtr; /**< Const shared pointer type for AbstractMetric. */

    //=========================================================================================================
    /**
     * Constructs a AbstractMetric object.
     */
    explicit AbstractMetric();

    static bool     m_bStorageModeIsActive;
    static int      m_iNumberBinStart;
    static int      m_iNumberBinAmount;

protected:
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // ABSTRACTMETRIC_H
