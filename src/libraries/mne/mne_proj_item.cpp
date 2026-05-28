//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_proj_item.cpp
 * @since March 2026
 * @brief Implementation of @ref MNELIB::MNEProjItem.
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
#include "mne_types.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEProjItem::MNEProjItem()
: nvec(0)
, kind(FIFFV_PROJ_ITEM_NONE)
, active(true)
, active_file(false)
, has_meg(false)
, has_eeg(false)
{
}

//=============================================================================================================

MNEProjItem::MNEProjItem(const MNEProjItem& other)
: vecs(other.vecs ? std::make_unique<MNENamedMatrix>(*other.vecs) : nullptr)
, nvec(other.nvec)
, desc(other.desc)
, kind(other.kind)
, active(other.active)
, active_file(other.active_file)
, has_meg(other.has_meg)
, has_eeg(other.has_eeg)
{
}

//=============================================================================================================

MNEProjItem& MNEProjItem::operator=(const MNEProjItem& other)
{
    if (this != &other) {
        vecs        = other.vecs ? std::make_unique<MNENamedMatrix>(*other.vecs) : nullptr;
        nvec        = other.nvec;
        desc        = other.desc;
        kind        = other.kind;
        active      = other.active;
        active_file = other.active_file;
        has_meg     = other.has_meg;
        has_eeg     = other.has_eeg;
    }
    return *this;
}

//=============================================================================================================

MNEProjItem::~MNEProjItem()
{
}

//=============================================================================================================

int MNEProjItem::affect(const QStringList& list, int nlist) const
{
    if (nvec == 0 || !vecs)
        return false;

    for (int k = 0; k < nlist; ++k) {
        for (int p = 0; p < vecs->ncol; ++p) {
            if (QString::compare(vecs->collist[p], list[k]) == 0) {
                for (int q = 0; q < vecs->nrow; ++q) {
                    if (vecs->data(q, p) != 0.0f)
                        return true;
                }
            }
        }
    }
    return false;
}
