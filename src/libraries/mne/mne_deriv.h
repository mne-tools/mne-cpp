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
 * @brief    MNE Derivation (MNEDeriv) class declaration.
 *
 */

#ifndef MNEDERIV_H
#define MNEDERIV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_sparse_named_matrix.h"

#include <fiff/fiff_types.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief One item in a derivation data set.
 *
 * Holds a sparse named matrix of derivation coefficients together with
 * validity and usage metadata and matched channel information.
 */
class MNESHARED_EXPORT MNEDeriv
{
public:
    typedef QSharedPointer<MNEDeriv> SPtr;              /**< Shared pointer type for MNEDeriv. */
    typedef QSharedPointer<const MNEDeriv> ConstSPtr;   /**< Const shared pointer type for MNEDeriv. */

    //=========================================================================================================
    /**
     * Constructs an empty MNE Derivation.
     */
    MNEDeriv();

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNEDeriv();

public:
    QString                  filename;   /**< Source file name the derivation was loaded from. */
    QString                  shortname;  /**< Short nickname for this derivation. */
    std::unique_ptr<MNESparseNamedMatrix> deriv_data; /**< The derivation data itself (sparse named matrix). */
    Eigen::VectorXi          in_use;     /**< Per-column count of non-zero elements in the derivation data. */
    Eigen::VectorXi          valid;      /**< Per-derivation validity flags considering input channel units. */
    QList<FIFFLIB::FiffChInfo> chs;      /**< First matching channel info for each derivation. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEDERIV_H
