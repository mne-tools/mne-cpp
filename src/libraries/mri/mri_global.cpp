// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2012-2026
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
//   Gabriel Motta <gabrielbenmotta@gmail.com>

/**
 * @file mri_global.cpp
 * @since 2026
 * @date  May 2026
 *
 * @brief Out-of-line definitions of the MRILIB build-info accessors declared in @ref mri_global.h.
 *
 * Pulls the build timestamp and short git hash from
 * @c utils/buildinfo.h (populated at configure time by
 * @c cmake/BuildInfo.cmake) and exposes them through the
 * @c buildDateTime() / @c buildHash() entry points so any binary
 * linking against MRILIB can report which snapshot of the
 * library it is actually running.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_global.h"

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

const char* MRILIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}
const char* MRILIB::buildHash(){ return UTILSLIB::gitHash();}
const char* MRILIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
