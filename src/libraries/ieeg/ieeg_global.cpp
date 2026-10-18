//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ieeg_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Definitions of the IEEGLIB build-info accessors declared in ieeg_global.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ieeg_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* IEEGLIB::buildDateTime()
{
    return UTILSLIB::dateTimeNow();
}

//=============================================================================================================

const char* IEEGLIB::buildHash()
{
    return UTILSLIB::gitHash();
}

//=============================================================================================================

const char* IEEGLIB::buildHashLong()
{
    return UTILSLIB::gitHashLong();
}
