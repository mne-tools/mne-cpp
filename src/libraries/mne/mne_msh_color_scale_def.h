//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_msh_color_scale_def.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     April 2017
 * @brief    Mesh-display colour scale definition (FreeSurfer @c mri_glm style).
 *
 * @ref mne::MNEMshColorScaleDef holds the threshold / fmid / fmax
 * triplet, the colormap kind and the sign mode used by the legacy
 * @c tksurfer / @c mneAnalyze viewers when mapping a scalar overlay
 * (curvature, sulcal depth, t-statistic, source estimate amplitude)
 * onto a cortical surface. Preserved so the same files can be opened in
 * mne-cpp's 3D viewers.
 */

#ifndef MNEMSHCOLORSCALEDEF_H
#define MNEMSHCOLORSCALEDEF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Replaces *mshColorScaleDef, mshColorScaleDefRec struct (analyze_types.c).
 *
 * @brief Color scale definition with value range and color mapping for surface visualization.
 */
class MNESHARED_EXPORT MNEMshColorScaleDef
{
public:
    int type = 0;          /**< What this scale setting is used for. */
    float mult = 0.0f;     /**< Multiplier from internal units to displayed numbers. */
    float fthresh = 0.0f;  /**< Threshold. */
    float fmid = 0.0f;     /**< Midpoint. */
    float fslope = 0.0f;   /**< Slope. */
    float tc_mult = 0.0f;  /**< Multiplier applied to the scales for time courses. */
    bool relative = false; /**< Whether fthresh and fmid are relative to the maximum over the surface. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEMSHCOLORSCALEDEF_H
