//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_mgh_tag.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of @ref MNELIB::MNEMghTag.
 *
 * Implements binary read/write of the tag triple, including the
 * endianness flip needed because MGH stores everything in big-endian.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_mgh_tag.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
