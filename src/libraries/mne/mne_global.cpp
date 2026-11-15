//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     mne_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     August 2021
 * @brief    mne_mne build-info helper implementation.
 *
 * Provides the out-of-line definition of @ref mne::buildDateTime which
 * returns the timestamp baked in by @c utils/buildinfo.h. Kept in a
 * separate translation unit so that touching the build date does not
 * force a recompile of every consumer of @c mne_global.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

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
