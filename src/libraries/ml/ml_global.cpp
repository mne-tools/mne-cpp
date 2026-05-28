//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ml_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of MLLIB build-stamp accessors that forward to UTILSLIB::buildinfo.
 *
 * Each of @ref MLLIB::buildDateTime, @ref MLLIB::buildHash and
 * @ref MLLIB::buildHashLong is a one-line delegate to the matching
 * @c UTILSLIB helper; centralising the call here lets every library in
 * mne-cpp report identical build metadata (filled in once by
 * @c buildinfo.h.in at CMake configure time) without each library
 * having to depend on the generation rules directly.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* MLLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* MLLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* MLLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
