//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_global.cpp
 * @since March 2026
 * @brief Implementation of the INVLIB build-info accessors.
 *
 * Defines @ref INVLIB::buildDateTime and @ref INVLIB::buildHash using
 * the compile-time strings produced by @c utils/buildinfo.h. The two
 * strings let an application link against INVLIB and still report the
 * exact upstream commit and build timestamp of the inverse library it
 * is using — essential for reproducibility of source estimates.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* INVLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* INVLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* INVLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
