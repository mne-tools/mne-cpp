//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Build-info free-function definitions for the MNE-CPP BIDS shared library.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* BIDSLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* BIDSLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* BIDSLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
