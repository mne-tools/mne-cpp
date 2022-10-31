//=============================================================================================================
/**
 * @file     mne_deriv.h
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
 * @brief    MNE Derivation (MneDeriv) class declaration.
 *
 */

#ifndef MNEDERIV_H
#define MNEDERIV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"
#include <fiff/fiff_types.h>
#include "mne_types.h"

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
/**
 * Implements an MNE Derivation (Replaces *mneDeriv,mneDerivRec; struct of MNE-C mne_types.h).
 *
 * @brief One item in a derivation data set
 */
class MNESHARED_EXPORT MneDeriv
{
public:
    typedef QSharedPointer<MneDeriv> SPtr;              /**< Shared pointer type for MneDeriv. */
    typedef QSharedPointer<const MneDeriv> ConstSPtr;   /**< Const shared pointer type for MneDeriv. */

    //=========================================================================================================
    /**
     * Constructs the MNE Derivation
     */
    MneDeriv();

    //=========================================================================================================
    /**
     * Destroys the MNE Derivation
     * Refactored: mne_free_deriv (mne_derivations.c)
     */
    ~MneDeriv();

public:
    char                    *filename;  /* Source file name */
    char                    *shortname; /* Short nickname for this derivation */
    mneSparseNamedMatrix    deriv_data; /* The derivation data itself */
    int                     *in_use;    /* How many non-zero elements on each column of the derivation data (This field is not always used) */
    int                     *valid;     /* Which of the derivations are valid considering the units of the input channels (This field is not always used) */
    QList<FIFFLIB::FiffChInfo>     chs;        /* First matching channel info in each derivation */

// ### OLD STRUCT ###
//typedef struct {                        /* One item in a derivation data set */
//    char                    *filename;  /* Source file name */
//    char                    *shortname; /* Short nickname for this derivation */
//    mneSparseNamedMatrix    deriv_data; /* The derivation data itself */
//    int                     *in_use;    /* How many non-zero elements on each column of the derivation data (This field is not always used) */
//    int                     *valid;     /* Which of the derivations are valid considering the units of the input channels (This field is not always used) */
//    FIFFLIB::fiffChInfo     chs;        /* First matching channel info in each derivation */
//} *mneDeriv,mneDerivRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEDERIV_H
