//=============================================================================================================
/**
 * @file     mne_deriv_set.h
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
 * @brief    MNE Derivation Set (MneDerivSet) class declaration.
 *
 */

#ifndef MNEDERIVSET_H
#define MNEDERIVSET_H

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
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneDeriv;

//=============================================================================================================
/**
 * Implements an MNE Derivation (Replaces *mneDerivSet,mneDerivSetRec; struct of MNE-C mne_types.h).
 *
 * @brief One item in a derivation data set
 */
class MNESHARED_EXPORT MneDerivSet
{
public:
    typedef QSharedPointer<MneDerivSet> SPtr;              /**< Shared pointer type for MneDerivSet. */
    typedef QSharedPointer<const MneDerivSet> ConstSPtr;   /**< Const shared pointer type for MneDerivSet. */

    //=========================================================================================================
    /**
     * Constructs the MNE Derivation Set
     */
    MneDerivSet();

    //=========================================================================================================
    /**
     * Destroys the MNE Derivation Set
     * Refactored: mne_free_deriv_set (mne_derivations.c)
     */
    ~MneDerivSet();

public:
    QList<MneDeriv*> derivs;       /* List of deriv items */

// ### OLD STRUCT ###
//typedef struct {            /* A collection of derivations */
//    int      nderiv;        /* How many? */
//    mneDeriv *derivs;       /* Pointers to the items */
//} *mneDerivSet,mneDerivSetRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEDERIVSET_H
