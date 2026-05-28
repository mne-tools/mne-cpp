//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_msh_light_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNEMshLightSet.
 *
 * Provides list management, the default three-light setup matching
 * @c tksurfer and the broadcast of intensity / on/off changes to every
 * member.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_msh_light_set.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
