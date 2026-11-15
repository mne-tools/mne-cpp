//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Trivial out-of-line definitions for the mne_lsl build-stamp accessors declared in lsl_global.h.
 *
 * The three @c buildDateTime / @c buildHash / @c buildHashLong
 * functions are intentionally implemented in a dedicated translation
 * unit rather than inlined in the header. This forces them to be
 * recompiled (and therefore re-stamped with the current value of
 * @c mne::dateTimeNow and @c mne::gitHash) every time the
 * LSL library is rebuilt, which is what allows the version dialogs
 * of downstream applications to report the exact commit and build
 * timestamp of the LSL stack they were linked against.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_global.h"

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
