//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_msh_color_scale_def.h
 * @since 2026
 * @date  April 2026
 * @brief Mesh-display colour scale definition (FreeSurfer @c mri_glm style).
 *
 * @ref MNELIB::MNEMshColorScaleDef holds the threshold / fmid / fmax
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
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Replaces *mshPicked, mshPickedRec struct (analyze_types.c).
 *
 * @brief Color scale definition with value range and color mapping for surface visualization.
 */
class MNESHARED_EXPORT MNEMshColorScaleDef
{
public:
    typedef QSharedPointer<MNEMshColorScaleDef> SPtr;              /**< Shared pointer type for MNEMshColorScaleDef. */
    typedef QSharedPointer<const MNEMshColorScaleDef> ConstSPtr;   /**< Const shared pointer type for MNEMshColorScaleDef. */

    //=========================================================================================================
    /**
     * Constructs the MNEMshColorScaleDef.
     */
    MNEMshColorScaleDef();

    //=========================================================================================================
    /**
     * Destroys the MNEMshColorScaleDef.
     */
    ~MNEMshColorScaleDef();

public:
    int   type;                     /* What is this scale setting good for? */
    float mult;                     /* Convenience multiplier from internal units to displayed numbers */
    float fthresh;                  /* Threshold */
    float fmid;                     /* This is in the middle */
    float fslope;                   /* We still use the slope internally (sigh) */
    float tc_mult;                  /* Multiply the scales by this value for timecourses */
    bool  relative;                 /* Are fthresh and fmid relative to the maximum value over the surface? */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHCOLORSCALEDEF_H
