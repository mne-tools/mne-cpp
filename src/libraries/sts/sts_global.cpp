//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_global.cpp
 * @since 2026
 * @date  April 2026
 * @brief Definitions of the STSLIB build-info accessors declared in sts_global.h.
 *
 * The three thin wrappers forward to UTILSLIB so the build timestamp and
 * git hash are baked into the STSLIB shared object at link time; this
 * lets analysis pipelines record which exact STSLIB binary produced a
 * given cluster p-value, TFCE map or shrinkage covariance, which matters
 * when results are revisited months later.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* STSLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* STSLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* STSLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
