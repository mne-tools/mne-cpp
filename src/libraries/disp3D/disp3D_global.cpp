//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     disp3D_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     August 2021
 * @brief    Library-version stamp for the disp3D 3-D visualisation library.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3D_global.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* mne::disp::buildDateTime()
{
    return mne::dateTimeNow();
}
const char* mne::disp::buildHash()
{
    return mne::gitHash();
}
const char* mne::disp::buildHashLong()
{
    return mne::gitHashLong();
}
