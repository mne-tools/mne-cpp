//=============================================================================================================
/**
* @file     debiasedsquaredweightedphaselagindex.h
* @author   Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Daniel Strohmeier and Matti Hamalainen. All rights reserved.
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
* @note Notes:
* - Some of this code was adapted from mne-python (https://martinos.org/mne) with permission from Alexandre Gramfort.
*
*
* @brief    DebiasedSquaredWeightedPhaseLagIndex class declaration.
*
*/

#ifndef DEBIASEDSQUAREDWEIGHTEDPHASELAGINDEX_H
#define DEBIASEDSQUAREDWEIGHTEDPHASELAGINDEX_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

#include "abstractmetric.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {


//*************************************************************************************************************
//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

class Network;


//=============================================================================================================
/**
* This class computes the weighted phase lag index connectivity metric.
*
* @brief This class computes the weighted phase lag index connectivity metric.
*/
class CONNECTIVITYSHARED_EXPORT DebiasedSquaredWeightedPhaseLagIndex : public AbstractMetric
{    

public:
    typedef QSharedPointer<DebiasedSquaredWeightedPhaseLagIndex> SPtr;            /**< Shared pointer type for DebiasedSquaredWeightedPhaseLagIndex. */
    typedef QSharedPointer<const DebiasedSquaredWeightedPhaseLagIndex> ConstSPtr; /**< Const shared pointer type for DebiasedSquaredWeightedPhaseLagIndex. */

    //=========================================================================================================
    /**
    * Constructs a DebiasedSquaredWeightedPhaseLagIndex object.
    */
    explicit DebiasedSquaredWeightedPhaseLagIndex();

    //=========================================================================================================
    /**
    * Calculates the debiased squared weighted phase lag index between the rows of the data matrix.
    *
    * @param[in] matDataList    The input data.
    * @param[in] matVert        The vertices of each network node.
    * @param[in] iNfft          The FFT length.
    * @param[in] sWindowType    The type of the window function used to compute tapered spectra.
    *
    * @return                   The connectivity information in form of a network structure.
    */
    static Network debiasedSquaredWeightedPhaseLagIndex(const QList<Eigen::MatrixXd> &matDataList,
                                                        const Eigen::MatrixX3f& matVert,
                                                        int iNfft=-1, const QString &sWindowType="hanning");

    //==========================================================================================================
    /**
    * Calculates the actual debiased squared weighted phase lag index between two data vectors.
    *
    * @param[in] matDataList    The input data.
    * @param[in] iNfft          The FFT length.
    * @param[in] sWindowType    The type of the window function used to compute tapered spectra.
    *
    * @return                   The DebiasedSquaredWPLI value.
    */
    static QVector<Eigen::MatrixXd> computeDebiasedSquaredWPLI(const QList<Eigen::MatrixXd> &matDataList,
                                                               int iNfft, const QString &sWindowType);
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // DEBIASEDSQUAREDWEIGHTEDPHASELAGINDEX_H
