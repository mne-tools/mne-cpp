//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ml_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of mne_ml build-stamp accessors that forward to mne_base, which owns the build stamp.
 *
 * Each of @ref mne::buildDateTime, @ref mne::buildHash and
 * @ref mne::buildHashLong is a one-line delegate to the matching
 *  mne_base helper; centralising the call here lets every library in
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
