//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     com_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     December, 2012
 * @brief    Translation unit for the @c mne_com build-info entry points declared in @ref com_global.h.
 *
 * The CMake build re-runs this file whenever any other source under @c src/libraries/com/ changes
 * (@c OBJECT_DEPENDS in the CMakeLists.txt) so the embedded build timestamp tracks the most recent edit.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "com_global.h"

#ifndef MNE_GIT_HASH_SHORT
#define MNE_GIT_HASH_SHORT "unknown"
#endif
#ifndef MNE_GIT_HASH_LONG
#define MNE_GIT_HASH_LONG "unknown"
#endif

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* mne::buildDateTime()
{
    return __DATE__ " " __TIME__;
}

//=============================================================================================================

const char* mne::buildHash()
{
    return MNE_GIT_HASH_SHORT;
}

//=============================================================================================================

const char* mne::buildHashLong()
{
    return MNE_GIT_HASH_LONG;
}
