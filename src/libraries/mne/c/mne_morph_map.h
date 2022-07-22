//=============================================================================================================
/**
 * @file     mne_morph_map.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    MneMorphMap class declaration.
 *
 */

#ifndef MNEMORPHMAP_H
#define MNEMORPHMAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"
#include "fiff/c/fiff_sparse_matrix.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Replaces *morphMap,morphMapRec struct (analyze_types.c).
 *
 * @brief The MneMorphMap class.
 */
class MNESHARED_EXPORT MneMorphMap
{
public:
    typedef QSharedPointer<MneMorphMap> SPtr;              /**< Shared pointer type for MneMorphMap. */
    typedef QSharedPointer<const MneMorphMap> ConstSPtr;   /**< Const shared pointer type for MneMorphMap. */

    //=========================================================================================================
    /**
     * Constructs the MneMorphMap.
     */
    MneMorphMap();

    //=========================================================================================================
    /**
     * Destroys the MneMorphMap.
     */
    ~MneMorphMap();

public:
    FIFFLIB::FiffSparseMatrix* map;		/* Multiply the data in the from surface with this to get to
                   * 'this' surface from the 'from' surface */
    int *best;			/* For each point on 'this' surface, the closest point on 'from' surface */
    int from_kind;		/* The kind field of the other surface */
    char *from_subj;		/* Name of the subject of the other surface */

// ### OLD STRUCT ###
//    typedef struct {
//      FIFFLIB::FiffSparseMatrix* map;		/* Multiply the data in the from surface with this to get to
//                     * 'this' surface from the 'from' surface */
//      int *best;			/* For each point on 'this' surface, the closest point on 'from' surface */
//      int from_kind;		/* The kind field of the other surface */
//      char *from_subj;		/* Name of the subject of the other surface */
//    } *morphMap,morphMapRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMORPHMAP_H
