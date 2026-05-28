//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_raw_buf_def.cpp
 * @since March 2026
 * @brief Implementation of @ref MNELIB::MNERawBufDef.
 *
 * Provides constructors and the per-buffer disk-to-Eigen materialiser
 * (handling int16 / int32 / float / double encodings with the
 * calibration vector applied).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_raw_buf_def.h"

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNERawBufDef::MNERawBufDef()
{
}

//=============================================================================================================

MNERawBufDef::~MNERawBufDef()
{
}
