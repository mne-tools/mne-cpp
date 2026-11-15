//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mri_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Out-of-line definitions of the mne_mri build-info accessors declared in @ref mri_global.h.
 *
 * Pulls the build timestamp and short git hash from
 * @c utils/buildinfo.h (populated at configure time by
 * @c cmake/BuildInfo.cmake) and exposes them through the
 * @c buildDateTime() / @c buildHash() entry points so any binary
 * linking against mne_mri can report which snapshot of the
 * library it is actually running.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_global.h"

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

const char* mne::buildDateTime()
{
    return mne::dateTimeNow();
}
const char* mne::buildHash()
{
    return mne::gitHash();
}
const char* mne::buildHashLong()
{
    return mne::gitHashLong();
}
