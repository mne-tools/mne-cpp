//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_raw_buf_def.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Implementation of @ref mne::MNERawBufDef.
 *
 * Provides constructors and the per-buffer disk-to-Eigen materialiser
 * (handling int16 / int32 / float / double encodings with the
 * calibration vector applied).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_raw_buf_def.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

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
