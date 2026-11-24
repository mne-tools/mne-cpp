//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_msh_light.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::MNEMshLight.
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

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEMshLight::MNEMshLight(bool state, float posX, float posY, float posZ, float diffX, float diffY, float diffZ)
: state(state)
, pos{posX, posY, posZ}
, diff{diffX, diffY, diffZ}
{
}
