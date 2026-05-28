//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_dig_point.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffDigPoint: streaming of one FIFF_DIG_POINT record (kind, ident, xyz).
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
