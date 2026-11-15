//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     math_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Thin forwarders that surface mne_math build-info into mne_math.
 *
 * The three accessors declared in @ref math_global.h simply delegate to
 * the equivalent helpers in @ref mne::buildinfo so that every
 * mne-cpp library exposes the same build-stamp API surface without
 * duplicating the @c __DATE__ / @c __TIME__ macros or having to know
 * how the CMake git-hash injection works. Keeping the implementation in
 * a separate translation unit also pins the build stamp to the moment
 * mne_math itself was compiled, not to whichever client first includes
 * @ref math_global.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "math_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* mne::buildDateTime()
{
    return mne::dateTimeNow();
}

//=============================================================================================================

const char* mne::buildHash()
{
    return mne::gitHash();
}

//=============================================================================================================

const char* mne::buildHashLong()
{
    return mne::gitHashLong();
}
