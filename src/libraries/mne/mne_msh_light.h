//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_msh_light.h
 * @since March 2026
 * @brief Single directional / positional light source for the mesh viewer.
 *
 * @ref MNELIB::MNEMshLight stores direction or position, RGB colour,
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
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
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
    typedef QSharedPointer<MNEMshLight> SPtr;              /**< Shared pointer type for MNEMshLight. */
    typedef QSharedPointer<const MNEMshLight> ConstSPtr;   /**< Const shared pointer type for MNEMshLight. */

    //=========================================================================================================
    /**
     * Constructs the MNEMshLight.
     */
    MNEMshLight();

    //=========================================================================================================
    /**
     * Copy Constructs of the MNEMshLight.
     */
    MNEMshLight(const MNEMshLight &p_mneMshLight);

    //=========================================================================================================
    /**
     * Constructs the MNEMshLight.
     */
    MNEMshLight(int state, float posX, float posY,float posZ,float diffX,float diffY,float diffZ);

    //=========================================================================================================
    /**
     * Destroys the MNEMshLight.
     */
    ~MNEMshLight();

public:
    bool  state;			/* On or off? */
    float pos[3];			/* Where is the light? */
    float diff[3];		/* Diffuse intensity */

// ### OLD STRUCT ###
//    typedef struct {		/* Definition of lighting */
//      int   state;			/* On or off? */
//      float pos[3];			/* Where is the light? */
//      float diff[3];		/* Diffuse intensity */
//    } *mshLight,mshLightRec;	/* We are only using diffuse lights here */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHLIGHT_H
