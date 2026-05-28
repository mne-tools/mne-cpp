//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_mgh_tag.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNEMghTag.
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
