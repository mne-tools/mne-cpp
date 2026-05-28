//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_dig_point.cpp
 * @since 2022
 * @date  October 2022
 * @brief Implementation of @ref FiffDigPoint: streaming of one FIFF_DIG_POINT record (kind, ident, xyz).
 *
 * Field layout matches @c fiffDigPointRec on disk so the records can be
 * read straight into the @ref FiffDigPoint array of a
 * @ref FiffDigPointSet.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dig_point.h"
#include "fiff_coord_trans.h"
#include "fiff_stream.h"
#include "fiff_dir_node.h"
#include "fiff_tag.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDigPoint::FiffDigPoint()
: kind(-1)
, ident(-1)
, coord_frame(-1)
{
    for(qint32 i = 0; i < 3; ++i)
        r[i] = -1;
}
