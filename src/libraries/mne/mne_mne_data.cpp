//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_mne_data.cpp
 * @since March 2026
 * @brief Implementation of @ref MNELIB::MNEMneData.
 *
 * Provides the constructors, resize helpers and explicit deallocation
 * needed because the original C code expected manual lifecycle
 * management of the scratch buffers.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_mne_data.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
