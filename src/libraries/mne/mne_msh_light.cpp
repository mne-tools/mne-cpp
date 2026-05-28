//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_msh_light.cpp
 * @since March 2026
 * @brief Implementation of @ref MNELIB::MNEMshLight.
 *
 * Implements constructors and the small helpers used by
 * @ref MNEMshLightSet to apply the light to a renderer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_msh_light.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEMshLight::MNEMshLight()
{
}

//=============================================================================================================

MNEMshLight::MNEMshLight(const MNEMshLight &p_mneMshLight)
: state(p_mneMshLight.state)
{
    this->pos[0] = p_mneMshLight.pos[0];
    this->pos[1] = p_mneMshLight.pos[1];
    this->pos[2] = p_mneMshLight.pos[2];
    this->diff[0] = p_mneMshLight.diff[0];
    this->diff[1] = p_mneMshLight.diff[1];
    this->diff[2] = p_mneMshLight.diff[2];
}

//=============================================================================================================

MNEMshLight::MNEMshLight(int state, float posX, float posY,float posZ, float diffX,float diffY,float diffZ)
{
    this->state = state;
    this->pos[0] = posX;
    this->pos[1] = posY;
    this->pos[2] = posZ;
    this->diff[0] = diffX;
    this->diff[1] = diffY;
    this->diff[2] = diffZ;
}

//=============================================================================================================

MNEMshLight::~MNEMshLight()
{
}
