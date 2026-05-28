//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_morph_map.cpp
 * @since March 2026
 * @brief Implementation of @ref MNELIB::MNEMorphMap.
 *
 * Implements construction of the bi-hemispheric sparse map from per-side
 * cortical maps and FIFF read/write of the @c FIFFB_MNE_MORPH_MAP block.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_morph_map.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
