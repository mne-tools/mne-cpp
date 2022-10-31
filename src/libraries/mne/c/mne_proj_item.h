//=============================================================================================================
/**
 * @file     mne_proj_item.h
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
 * @brief    MNEProjItem class declaration.
 *
 */

#ifndef MNEPROJITEM_H
#define MNEPROJITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneNamedMatrix;

//=============================================================================================================
/**
 * Implements an MNE Projection Item (Replaces *mneProjItem,mneProjItemRec; struct of MNE-C mne_types.h).
 *
 * @brief One linear projection item
 */
class MNESHARED_EXPORT MneProjItem
{
public:
    typedef QSharedPointer<MneProjItem> SPtr;              /**< Shared pointer type for MneDeriv. */
    typedef QSharedPointer<const MneProjItem> ConstSPtr;   /**< Const shared pointer type for MneDeriv. */

    //=========================================================================================================
    /**
     * Constructs the MNE Projection Item
     * Refactored: mne_new_proj_op_item (mne_lin_proj.c)
     */
    MneProjItem();

    //=========================================================================================================
    /**
     * Destroys the MNE Projection Item
     * Refactored: mne_free_proj_op_item (mne_lin_proj.c)
     */
    ~MneProjItem();

    static int mne_proj_item_affect(MneProjItem* it, const QStringList& list, int nlist);

public:
    MneNamedMatrix* vecs;           /**< The original projection vectors. */
    int             nvec;           /**< Number of vectors = vecs->nrow. */
    QString         desc;           /**< Projection item description. */
    int             kind;           /**< Projection item kind. */
    int             active;         /**< Is this item active now?. */
    int             active_file;    /**< Was this item active when loaded from file?. */
    int             has_meg;        /**< Does it have MEG channels?. */
    int             has_eeg;        /**< Does it have EEG channels?. */

// ### OLD STRUCT ###
//typedef struct {    /* One linear projection item */
//    MNELIB::MneNamedMatrix* vecs;   /* The original projection vectors */
//    int            nvec;                /* Number of vectors = vecs->nrow */
//    char           *desc;               /* Projection item description */
//    int            kind;                /* Projection item kind */
//    int            active;              /* Is this item active now? */
//    int            active_file;         /* Was this item active when loaded from file? */
//    int            has_meg;             /* Does it have MEG channels? */
//    int            has_eeg;             /* Does it have EEG channels? */
//} *mneProjItem,mneProjItemRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEPROJITEM_H
