//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     base_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     August 2021
 * @brief    Out-of-line definitions for the mne_base build-info accessors declared in @ref base_global.h.
 *
 * The only file that expands @c __DATE__ and @c __TIME__ and sees the git hash, which CMake defines for it
 * alone. Every other unit's preprocessed text and compile command then stay the same from one build and commit
 * to the next, so the compiler cache can reuse them.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* mne::build_date_time()
{
    return __DATE__ " " __TIME__;
}

//=============================================================================================================

const char* mne::build_hash()
{
#ifdef MNE_GIT_HASH_SHORT
    return MNE_GIT_HASH_SHORT;
#else
    return "Git hash not defined.";
#endif
}

//=============================================================================================================

const char* mne::build_hash_long()
{
#ifdef MNE_GIT_HASH_LONG
    return MNE_GIT_HASH_LONG;
#else
    return "Git hash long not defined.";
#endif
}
