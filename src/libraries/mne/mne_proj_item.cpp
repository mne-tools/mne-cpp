//=============================================================================================================
/**
 * @file     mne_proj_item.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MNEProjItem class definition.
 *
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
