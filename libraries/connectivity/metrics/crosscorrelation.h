//=============================================================================================================
/**
* @file     crosscorrelation.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     CrossCorrelation class declaration.
*
*/

#ifndef CROSSCORRELATION_H
#define CROSSCORRELATION_H


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
* This class computes the cross correlation connectivity metric.
*
* @brief This class computes the cross correlation connectivity metric.
*/
class CONNECTIVITYSHARED_EXPORT CrossCorrelation : public AbstractMetric
{    

public:
    typedef QSharedPointer<CrossCorrelation> SPtr;            /**< Shared pointer type for CrossCorrelation. */
    typedef QSharedPointer<const CrossCorrelation> ConstSPtr; /**< Const shared pointer type for CrossCorrelation. */

    //=========================================================================================================
    /**
    * Constructs a CrossCorrelation object.
    */
    explicit CrossCorrelation();

    //=========================================================================================================
    /**
    * Calculates the cross correlation between the rows of the data matrix.
    *
    * @param[in] matDataList    The input data.
    * @param[in] matVert        The vertices of each network node.
    *
    * @return                   The connectivity information in form of a network structure.
    */
    static Network crossCorrelation(const QList<Eigen::MatrixXd> &matDataList, const Eigen::MatrixX3f& matVert);

protected:
    //=========================================================================================================
    /**
    * Calculates the connectivity matrix for a given input data matrix based on the cross correlation coefficient.
    *
    * @param[in] data       The input data.
    *
    * @return               The connectivity matrix.
    */
    static Eigen::MatrixXd calculate(const AbstractMetricInputData& inputData);

    //=========================================================================================================
    /**
    * Sums up (reduces) the in parallel processed connectivity matrix.
    *
    * @param[out] resultData    The result data.
    * @param[in]  data          The incoming and temporary result data.
    */
    static void sum(Eigen::MatrixXd &resultData, const Eigen::MatrixXd &data);

    //=========================================================================================================
    /**
    * Calculates the actual correlation coefficient between two data vectors.
    *
    * @param[in] vecFirst    The first input data row.
    * @param[in] vecSecond   The second input data row.
    *
    * @return                The cross position where the maximum correlation was computed.
    */
    static QPair<int,double> calcCrossCorrelation(const Eigen::MatrixXcd &matDataFirst,
                                                  const Eigen::MatrixXcd &matDataSecond);

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // CROSSCORRELATION_H
