//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_mgh_tag_group.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of @ref MNELIB::MNEMghTagGroup.
 *
 * Implements iteration over the tag stream until EOF and the lookup
 * helpers used by @ref MRILIB::MriMghIO to recover the volume geometry
 * transform.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_mgh_tag_group.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
