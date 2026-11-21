//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_proj_item.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Implementation of @ref mne::MNEProjItem.
 *
 * Implements FIFF read/write of the @c FIFFB_PROJ_ITEM block including
 * the @c FIFF_PROJ_ITEM_VECTORS, @c FIFF_PROJ_ITEM_CH_NAME_LIST and the
 * @c FIFF_PROJ_ITEM_KIND / @c FIFF_PROJ_ITEM_ACTIVE tags.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_proj_item.h"
#include "mne_named_matrix.h"
#include <fiff/fiff_file.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MNEProjItem::affect(std::span<const std::string> list) const
{
    if (nvec() == 0)
        return false;

    for (const auto& name : list) {
        for (int p = 0; p < vecs.ncol(); ++p) {
            if (vecs.collist[p] == name && (vecs.data.col(p).array() != 0.0f).any())
                return true;
        }
    }
    return false;
}
