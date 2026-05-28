//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Definitions for the static MATLAB-style FIFF facade declared in @ref fiff.h.
 *
 * Each method forwards to the matching method on the underlying class
 * (@ref FiffStream, @ref FiffRawData, @ref FiffEvoked, @ref FiffEvokedSet,
 * @ref FiffCov, @ref FiffInfo). No state lives here.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
