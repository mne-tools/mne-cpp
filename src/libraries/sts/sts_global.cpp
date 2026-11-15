//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sts_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Definitions of the mne_sts build-info accessors declared in sts_global.h.
 *
 * The three thin wrappers forward to mne_base so the build timestamp and
 * git hash are baked into the mne_sts shared object at link time; this
 * lets analysis pipelines record which exact mne_sts binary produced a
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
