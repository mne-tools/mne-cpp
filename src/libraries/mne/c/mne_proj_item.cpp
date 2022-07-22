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
 * @brief    Definition of the MNEProjItem Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_proj_item.h"
#include "mne_types.h"

#define FREE_21(x) if ((char *)(x) != NULL) free((char *)(x))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneProjItem::MneProjItem()
: vecs (NULL)
, nvec (0)
, kind (FIFFV_PROJ_ITEM_NONE)
, active (TRUE)
, active_file (FALSE)
, has_meg (FALSE)
, has_eeg (FALSE)
{
}

//=============================================================================================================

MneProjItem::~MneProjItem()
{
    if(vecs)
        delete vecs;
    desc.clear();
    return;
}

//=============================================================================================================

int MneProjItem::mne_proj_item_affect(MneProjItem *it, const QStringList& list, int nlist)
/*
     * Does this projection item affect this list of channels?
     */
{
    int k,p,q;

    if (it == NULL || it->vecs == NULL || it->nvec == 0)
        return FALSE;

    for (k = 0; k < nlist; k++)
        for (p = 0; p < it->vecs->ncol; p++)
            if (QString::compare(it->vecs->collist[p],list[k]) == 0) {
                for (q = 0; q < it->vecs->nrow; q++) {
                    if (it->vecs->data[q][p] != 0.0)
                        return TRUE;
                }
            }
    return FALSE;
}
