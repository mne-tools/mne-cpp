//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_msh_light.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Single directional / positional light source for the mesh viewer.
 *
 * @ref mne::MNEMshLight stores direction or position, RGB colour,
 * intensity and on/off state - one record per scene light - matching
 * the @c tksurfer light specification.
 */

#ifndef MNEMSHLIGHT_H
#define MNEMSHLIGHT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

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

//=============================================================================================================
/**
 * Replaces *mshLight,mshLightRec struct (analyze_types.c).
 *
 * @brief Single light source with position, color, and state for 3-D surface rendering.
 */
class MNESHARED_EXPORT MNEMshLight
{
public:
    //=========================================================================================================
    /**
     * Constructs a switched-off light at the origin.
     */
    MNEMshLight() = default;

    //=========================================================================================================
    /**
     * Constructs the MNEMshLight.
     *
     * @param[in] state  Whether the light is switched on.
     * @param[in] posX   Light position x coordinate.
     * @param[in] posY   Light position y coordinate.
     * @param[in] posZ   Light position z coordinate.
     * @param[in] diffX  Diffuse intensity, red component.
     * @param[in] diffY  Diffuse intensity, green component.
     * @param[in] diffZ  Diffuse intensity, blue component.
     */
    MNEMshLight(bool state, float posX, float posY, float posZ, float diffX, float diffY, float diffZ);

public:
    bool state = false; /**< Whether the light is switched on. */
    float pos[3] = {};  /**< Light position. */
    float diff[3] = {}; /**< Diffuse intensity (red, green, blue). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEMSHLIGHT_H
