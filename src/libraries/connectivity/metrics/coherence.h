//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     coherence.h
 * @author   Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2018
 * @brief    Magnitude-squared coherence (MSC) between every channel pair, band-averaged over the selected DPSS spectral window.
 *
 * Coherence is the canonical undirected linear measure of frequency-domain
 * coupling. It is defined as the squared magnitude of complex coherency
 * normalised by the auto-spectra,
 *
 *   C_{xy}(f) = |S_{xy}(f)|^2 / (S_{xx}(f) * S_{yy}(f))
 *
 * where @c S_{xy} is the cross-spectral density and @c S_{xx}, @c S_{yy}
 * are the auto-spectra. Values lie in @c [0, 1]: 1 means a perfectly
 * linear, constant phase/amplitude relationship at frequency @c f and 0
 * means no linear relationship. Because the squared magnitude discards the
 * phase, coherence is symmetric and cannot distinguish true coupling from
 * volume-conduction / common-reference artefacts that produce instantaneous
 * (zero-lag) mixing - use @ref ImagCoherence (Nolte et al., 2004) when
 * volume conduction is a concern.
 *
 * This implementation delegates the per-trial FFT, tapering and CSD
 * accumulation to @ref Coherency::calculateAbs, then collapses the result
 * to a scalar per channel pair by averaging |coherency|^2 over the
 * frequency window @c [AbstractMetric::m_iNumberBinStart,
 * m_iNumberBinStart + m_iNumberBinAmount).
 */

#ifndef COHERENCE_H
#define COHERENCE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

#include "abstractmetric.h"

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
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {

//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

class Network;
class ConnectivitySettings;

//=============================================================================================================
/**
 * Computes magnitude-squared coherence between every pair of channels.
 *
 * The actual cross-spectral and auto-spectral accumulation is shared with
 * @ref Coherency; this class only wraps the absolute-value branch and
 * exposes a single @ref calculate entry point that returns a @ref Network
 * whose edge weights are the band-averaged |coherency|^2 values in [0, 1].
 *
 * @brief Magnitude-squared coherence estimator (symmetric, sensitive to zero-lag coupling).
 */
class CONNECTIVITYSHARED_EXPORT Coherence : public AbstractMetric
{

public:
    typedef QSharedPointer<Coherence> SPtr;            /**< Shared pointer type for Coherence. */
    typedef QSharedPointer<const Coherence> ConstSPtr; /**< Const shared pointer type for Coherence. */

    //=========================================================================================================
    /**
     * Constructs a Coherence object.
     */
    explicit Coherence();

    //=========================================================================================================
    /**
     * Calculates the coherence between the rows of the data matrix.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     */
    static Network calculate(ConnectivitySettings &connectivitySettings);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNECTIVITYLIB

#endif // COHERENCE_H
