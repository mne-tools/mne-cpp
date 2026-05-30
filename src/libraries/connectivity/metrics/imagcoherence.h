//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     imagcoherence.h
 * @author   Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2018
 * @brief    Volume-conduction-robust imaginary coherence (Nolte et al., 2004) between every channel pair.
 *
 * Imaginary coherence is the imaginary part of complex coherency,
 *
 *   ImCoh_{xy}(f) = Im( S_{xy}(f) / sqrt(S_{xx}(f) * S_{yy}(f)) )
 *
 * with output range @c [-1, 1] - or @c [0, 1] when reported as
 * @c |Im(Coh)|, as done here. Its key property, established by Nolte,
 * Bai, Wheaton, Mari, Vorbach and Hallett (NeuroImage 2004), is that any
 * instantaneous mixing of independent sources (volume conduction,
 * common-reference, common-pickup) projects purely onto the real axis
 * and therefore contributes 0 to @c Im(Coh). Only interactions with a
 * non-zero phase lag - the signature of finite conduction delay between
 * two true neural generators - survive, which makes this estimator a
 * standard choice for sensor-space MEG/EEG connectivity where volume
 * conduction would otherwise dominate ordinary coherence.
 *
 * The trade-off is reduced sensitivity to genuine but short-lag
 * interactions and a non-trivial dependence on the relative source
 * orientations, so ImCoh is usually reported alongside the phase-based
 * estimators (PLI, wPLI) which share its anti-leakage property but have
 * different bias/variance profiles.
 */

#ifndef IMAGCOHERENCE_H
#define IMAGCOHERENCE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

#include "abstractmetric.h"
#include "../connectivitysettings.h"

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

//=============================================================================================================
/**
 * Computes the imaginary part of complex coherency for every channel pair.
 *
 * The estimator inherits the volume-conduction rejection property derived
 * by Nolte et al. (NeuroImage, 2004): zero-lag mixing of independent
 * sources contributes only to the real axis and is removed, so the
 * returned edge weights reflect interactions with non-zero conduction
 * delay only. The cross-spectral accumulation is delegated to
 * @ref Coherency::calculateImag; this class is the public entry point used
 * by the dispatcher in @ref Connectivity.
 *
 * @brief Imaginary-coherence estimator (Nolte et al. 2004); rejects zero-lag volume-conduction mixing.
 */
class CONNECTIVITYSHARED_EXPORT ImagCoherence : public AbstractMetric
{

public:
    typedef QSharedPointer<ImagCoherence> SPtr;            /**< Shared pointer type for ImagCoherence. */
    typedef QSharedPointer<const ImagCoherence> ConstSPtr; /**< Const shared pointer type for ImagCoherence. */

    //=========================================================================================================
    /**
     * Constructs a ImagCoherence object.
     */
    explicit ImagCoherence();

    //=========================================================================================================
    /**
     * Calculates the imaginary coherence between the rows of the data matrix.
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

#endif // IMAGCOHERENCE_H
