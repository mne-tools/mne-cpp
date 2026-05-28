//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_msh_light_set.cpp
 * @since March 2026
 * @brief Implementation of @ref MNELIB::MNEMshLightSet.
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
