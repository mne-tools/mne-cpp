//=============================================================================================================
/**
* @file     connectivitymeasures.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     ConnectivityMeasures class declaration.
*
*/

#ifndef CONNECTIVITYMEASURES_H
#define CONNECTIVITYMEASURES_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPair>
#include <QString>


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
* This class computes basic (functional) connectivity measures.
*
* @brief This class computes basic (functional) connectivity measures.
*/
class CONNECTIVITYSHARED_EXPORT ConnectivityMeasures
{    

public:
    typedef QSharedPointer<ConnectivityMeasures> SPtr;            /**< Shared pointer type for ConnectivityMeasures. */
    typedef QSharedPointer<const ConnectivityMeasures> ConstSPtr; /**< Const shared pointer type for ConnectivityMeasures. */

    //=========================================================================================================
    /**
    * Constructs a ConnectivityMeasures object.
    */
    explicit ConnectivityMeasures();

    //=========================================================================================================
    /**
    * Calculates the Pearson's correlation coefficient between the rows of the data matrix.
    *
    * @param[in] matData    The input data for whicht the cross correlation is to be calculated.
    * @param[in] matVert    The vertices of each network node.
    *
    * @return               The connectivity information in form of a network structure.
    */
    static Network pearsonsCorrelationCoeff(const Eigen::MatrixXd& matData, const Eigen::MatrixX3f& matVert);

    //=========================================================================================================
    /**
    * Calculates the cross correlation between the rows of the data matrix.
    *
    * @param[in] matData    The input data for which the cross correlation is to be calculated.
    * @param[in] matVert    The vertices of each network node.
    *
    * @return               The connectivity information in form of a network structure.
    */
    static Network crossCorrelation(const Eigen::MatrixXd& matData, const Eigen::MatrixX3f& matVert);

    //=========================================================================================================
    /**
        * Calculates the Phase Lag Index between the rows of the data matrix.
        *
        * @param[in] matData    The input data for whicht the phase lag index is to be calculated.
        * @param[in] matVert    The vertices of each network node.
        *
        * @return               The connectivity information in form of a network structure.
        */
    static Network phaseLagIndex(const Eigen::MatrixXd& matData, const Eigen::MatrixX3f& matVert);

protected:
    //=========================================================================================================
    /**
    * Calculates the actual Pearson's correlation coefficient between two data vectors.
    *
    * @param[in] vecFirst    The first input data row.
    * @param[in] vecSecond   The second input data row.
    *
    * @return               The Pearson's correlation coefficient.
    */
    static double calcPearsonsCorrelationCoeff(const Eigen::RowVectorXd &vecFirst, const Eigen::RowVectorXd &vecSecond);

    //=========================================================================================================
    /**
    * Calculates the actual cross correlation between two data vectors.
    *
    * @param[in] vecFirst    The first input data row.
    * @param[in] vecSecond   The second input data row.
    *
    * @return               The result in form of a QPair. First element represents the index of the maximum. Second element represents the actual correlation value.
    */
    static QPair<int,double> calcCrossCorrelation(const Eigen::RowVectorXd &vecFirst, const Eigen::RowVectorXd &vecSecond);

    //==========================================================================================================
    /**
        * Calculates the actual Phase Lag Index between two data vectors.
        *
        * @param[in] vecFirst    The first input data row.
        * @param[in] vecSecond   The second input data row.
        *
        * @return               The Pearson's correlation coefficient.
        */
    static double calcPhaseLagIndex(const Eigen::RowVectorXd &vecFirst, const Eigen::RowVectorXd &vecSecond);
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // CONNECTIVITYMEASURES_H
