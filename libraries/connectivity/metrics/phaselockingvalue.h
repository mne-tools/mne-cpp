//=============================================================================================================
/**
* @file     phaselockingvalue.h
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
* @brief     PhaseLockingValue class declaration.
*
*/

#ifndef PHASELOCKINGVALUE_H
#define PHASELOCKINGVALUE_H


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
class ConnectivitySettings;


//=============================================================================================================
/**
* This class computes the phase locking value connectivity metric.
*
* @brief This class computes the phase locking value connectivity metric.
*/
class CONNECTIVITYSHARED_EXPORT PhaseLockingValue : public AbstractMetric
{    

public:
    typedef QSharedPointer<PhaseLockingValue> SPtr;            /**< Shared pointer type for PhaseLockingValue. */
    typedef QSharedPointer<const PhaseLockingValue> ConstSPtr; /**< Const shared pointer type for PhaseLockingValue. */

    //=========================================================================================================
    /**
    * Constructs a PhaseLockingValue object.
    */
    explicit PhaseLockingValue();

    //=========================================================================================================
    /**
    * Calculates the phase locking value between the rows of the data matrix.
    *
    * @param[in] connectivitySettings   The input data and parameters.
    *
    * @return                   The connectivity information in form of a network structure.
    */
    static Network phaseLockingValue(const ConnectivitySettings &connectivitySettings);

    //==========================================================================================================
    /**
    * Calculates the actual phase locking value between two data vectors.
    *
    * @param[in] matDataList    The input data.
    * @param[in] iNfft          The FFT length.
    * @param[in] sWindowType    The type of the window function used to compute tapered spectra.
    *
    * @return                   The PLV value.
    */
    static QVector<Eigen::MatrixXd> computePLV(const QList<Eigen::MatrixXd> &matDataList,
                                               int iNfft,
                                               const QString &sWindowType);
protected:
    //=========================================================================================================
    /**
    * Computes the PLV values. This function gets called in parallel.
    *
    * @param[in] matInputData           The input data.
    * @param[in] iNRows                 The number of rows.
    * @param[in] iNFreqs                The number of frequenciy bins.
    * @param[in] iNfft                  The FFT length.
    * @param[in] tapers                 The taper information.
    *
    * @return            The coherency result in form of AbstractMetricResultData.
    */
    static QVector<Eigen::MatrixXcd> compute(const Eigen::MatrixXd& matInputData,
                                             int iNRows,
                                             int iNFreqs,
                                             int iNfft,
                                             const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);

    //=========================================================================================================
    /**
    * Reduces the PLV computation to a final result. This function gets called in parallel.
    *
    * @param[out] finalData    The final data.
    * @param[in]  resultData   The resulting data from the computation step.
    */
    static void reduce(QVector<Eigen::MatrixXcd>& finalData,
                       const QVector<Eigen::MatrixXcd>& resultData);
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // PHASELOCKINGVALUE_H
