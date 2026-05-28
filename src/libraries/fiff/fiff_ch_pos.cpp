//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_ch_pos.cpp
 * @since October 2012
 * @brief Implementation of @ref FiffChPos: streaming of the coil-frame origin and (ex, ey, ez) orientation triad sub-record.
 *
 * Twelve floats on disk (r0, ex, ey, ez), matching the legacy
 * @c fiffChPosRec struct so the record can be streamed without per-field
 * marshalling.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_ch_pos.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffChPos::FiffChPos()
: coil_type(0)
{
    for(qint32 i = 0; i < 3; ++i)
    {
        r0[i] = 0.0f;
        ex[i] = 0.0f;
        ey[i] = 0.0f;
        ey[i] = 0.0f;
    }
}

//=============================================================================================================

FiffChPos::FiffChPos(const FiffChPos &p_FiffChPos)
: coil_type(p_FiffChPos.coil_type)
{
    for(qint32 i = 0; i < 3; ++i)
    {
        r0[i] = p_FiffChPos.r0[i];
        ex[i] = p_FiffChPos.ex[i];
        ey[i] = p_FiffChPos.ey[i];
        ez[i] = p_FiffChPos.ez[i];
    }
}

//=============================================================================================================

FiffChPos::~FiffChPos()
{
}

