//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fwd_eeg_sphere_layer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Single concentric spherical shell (radius + conductivity) of a multi-layer EEG head model.
 *
 * The classic four-shell de Munck / Berg-Scherg head model approximates
 * the head as concentric spheres with piecewise-constant conductivity
 * (brain, CSF, skull, scalp). Each layer contributes one boundary at
 * which the electric potential must be continuous and the normal current
 * density must scale by the conductivity ratio. FwdEegSphereLayer stores
 * the two quantities the analytic solution needs from each shell —
 * outer radius @c rad and conductivity σ @c sigma — along with the
 * derived ratios reused inside the Legendre-series expansion.
 *
 * Layers are sorted from inner to outer by @c rad before any series
 * evaluation; this is the order assumed by @c fwd_eeg_multi_spherepot()
 * in MNE-C and by every subsequent Berg-Scherg fit.
 */

#ifndef FWD_EEG_SPHERE_LAYER_H
#define FWD_EEG_SPHERE_LAYER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * Implements FwdEegSphereLayer (replaces @c fwdEegSphereLayer / @c fwdEegSphereLayerRec from MNE-C @c fwd_types.h).
 *
 * @brief One concentric shell (outer radius @c rad, conductivity @c sigma and the derived ratios) of a multi-shell de Munck / Berg-Scherg EEG head model.
 */
class FWDSHARED_EXPORT FwdEegSphereLayer
{
public:

    //=========================================================================================================
    /**
     * Constructs the Electric Current Dipole
     */
    FwdEegSphereLayer();

    //=========================================================================================================
    /**
     * Destroys the Forward EEG Sphere Layer description
     */
    ~FwdEegSphereLayer();

    //=========================================================================================================
    /**
     * Compare two sphere layers by radius for sorting.
     *
     * @param[in] v1   First layer.
     * @param[in] v2   Second layer.
     *
     * @return True if v1.rad < v2.rad.
     */
    static bool comp_layers(const FwdEegSphereLayer& v1, const FwdEegSphereLayer& v2)
    {
        return v1.rad < v2.rad;
    }

public:
    float rad;          /**< The actual rads. */
    float rel_rad;      /**< Relative rads. */
    float sigma;        /**< Conductivity. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_EEG_SPHERE_LAYER_H
