//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of the mne_inv build-info accessors.
 *
 * Defines @ref mne::buildDateTime and @ref mne::buildHash using
 * the compile-time strings produced by @c utils/buildinfo.h. The two
 * strings let an application link against mne_inv and still report the
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
