//=============================================================================================================
/**
 * @file     mne_mne_data.h
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
 * @brief    MNE MNE Data (MneMneData) class declaration.
 *
 */

#ifndef MNEMNEDATA_H
#define MNEMNEDATA_H

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
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * Implements MNE Mne Data (Replaces *mneMneData,mneMneDataRec; struct of MNE-C mne_types.h).
 *
 * @brief Data associated with MNE computations for each mneMeasDataSet
 */
class MNESHARED_EXPORT MneMneData
{
public:
    typedef QSharedPointer<MneMneData> SPtr;              /**< Shared pointer type for MneMneData. */
    typedef QSharedPointer<const MneMneData> ConstSPtr;   /**< Const shared pointer type for MneMneData. */

    //=========================================================================================================
    /**
     * Constructs the MNE Mne Data
     */
    MneMneData();

    //=========================================================================================================
    /**
     * Destroys the MNE Mne Data description
     * Refactored: mne_free_mne_data (mne_inverse_util.c)
     */
    ~MneMneData();

public:
    float **datap;          /* Projection of the whitened data onto the field eigenvectors */
    float **predicted;      /* The predicted data */
    float *SNR;             /* Estimated power SNR as a function of time */
    float *lambda2_est;     /* Regularization parameter estimated from available data */
    float *lambda2;         /* Regularization parameter to be used (as a function of time) */

// ### OLD STRUCT ###
//typedef struct {        /* Data associated with MNE computations for each mneMeasDataSet */
//    float **datap;          /* Projection of the whitened data onto the field eigenvectors */
//    float **predicted;      /* The predicted data */
//    float *SNR;             /* Estimated power SNR as a function of time */
//    float *lambda2_est;     /* Regularization parameter estimated from available data */
//    float *lambda2;             /* Regularization parameter to be used (as a function of time) */
//} *mneMneData,mneMneDataRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMNEDATA_H
