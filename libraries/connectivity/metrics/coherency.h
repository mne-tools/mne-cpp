//=============================================================================================================
/**
* @file     coherency.h
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
* - QtConcurrent can be used to speed up computation.
*
* @brief     Coherency class declaration.
*
*/

#ifndef COHERENCY_H
#define COHERENCY_H


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
#include <QMutex>


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
* This class computes the coherency connectivity metric.
*
* @brief This class computes the coherency connectivity metric.
*/
class CONNECTIVITYSHARED_EXPORT Coherency : public AbstractMetric
{    

public:
    typedef QSharedPointer<Coherency> SPtr;            /**< Shared pointer type for Coherency. */
    typedef QSharedPointer<const Coherency> ConstSPtr; /**< Const shared pointer type for Coherency. */

    //=========================================================================================================
    /**
    * Constructs a Coherency object.
    */
    explicit Coherency();

    //=========================================================================================================
    /**
    * Calculates the real part of coherency of the rows of the data matrix.
    *
    * @param[out] finalNetwork          The resulting network.
    * @param[in] connectivitySettings   The input data and parameters.
    */
    static void calculateReal(Network& finalNetwork,
                                     const ConnectivitySettings &connectivitySettings);

    //=========================================================================================================
    /**
    * Calculates the imaginary part of coherency of the rows of the data matrix.
    *
    * @param[out] finalNetwork          The resulting network.
    * @param[in] connectivitySettings   The input data and parameters.
    */
    static void calculateImag(Network& finalNetwork,
                                     const ConnectivitySettings &connectivitySettings);

    //=========================================================================================================
    /**
    * Calculates the coherency of the rows of the data matrix.
    *
    * @param[out] finalNetwork          The resulting network.
    * @param[in] connectivitySettings   The input data and parameters.
    */
    static void calculate(QVector<QPair<int,Eigen::MatrixXcd> >& vecCoherency,
                                 const ConnectivitySettings &connectivitySettings);

private:
    //=========================================================================================================
    /**
    * Computes the coherency values. This function gets called in parallel.
    *
    * @param[in] matInputData           The input data.
    * @param[in] iNRows                 The number of rows.
    * @param[in] iNFreqs                The number of frequenciy bins.
    * @param[in] iNfft                  The FFT length.
    * @param[in] tapers                 The taper information.
    *
    * @return            The coherency result in form of AbstractMetricResultData.
    */
    static AbstractMetricResultData compute(const Eigen::MatrixXd& matInputData,
                                            int iNRows,
                                            int iNFreqs,
                                            int iNfft,
                                            const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);

    //=========================================================================================================
    /**
    * Reduces the coherency computation to a final result. This function gets called in parallel.
    *
    * @param[out] finalData    The final data data.
    * @param[in]  resultData   The resulting data from the computation step.
    */
    static void reduce(AbstractMetricResultData &finalData,
                       const AbstractMetricResultData& resultData);

    //=========================================================================================================
    /**
    * Computes the PSD and CSD. This function gets called in parallel.
    *
    * @param[out] pairInput     The in/out data.
    * @param[in]  matPsdAvg     The averaged PSD.
    */
    static void computePSDCSD(QPair<int,Eigen::MatrixXcd>& pairInput,
                              const Eigen::MatrixXd& matPsdAvg);
    static void computePSDCSDReal(QMutex& mutex,
                                  Network& finalNetwork,
                                  QPair<int,Eigen::MatrixXcd>& pairInput,
                                  const Eigen::MatrixXd& matPsdAvg);
    static void computePSDCSDImag(QMutex& mutex,
                                  Network& finalNetwork,
                                  QPair<int,Eigen::MatrixXcd>& pairInput,
                                  const Eigen::MatrixXd& matPsdAvg);
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // COHERENCY_H
