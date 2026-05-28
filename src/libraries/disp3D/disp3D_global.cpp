//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file disp3D_global.cpp
 * @since 2022
 * @date  March 2026
 * @brief Library-version stamp for the disp3D 3-D visualisation library.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3D_global.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* DISP3DLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}
const char* DISP3DLIB::buildHash(){ return UTILSLIB::gitHash();}
const char* DISP3DLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
