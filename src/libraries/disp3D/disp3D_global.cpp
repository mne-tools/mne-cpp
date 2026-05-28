//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file disp3D_global.cpp
 * @since August 2021
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
