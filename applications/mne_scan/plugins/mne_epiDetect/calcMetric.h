//=============================================================================================================
/**
* @file     calcMetric.h
* @author   Louis Eichhorst <louis.eichhorst@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     %{currentdate} Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Your name and Matti Hamalainen. All rights reserved.
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
* @brief     calcMetric class declaration.
*
*/

#ifndef CALCMETRIC_H
#define CALCMETRIC_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QFuture>
#include <QList>
//#include <generics/circularmatrixbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE Cannot convert result of " Cpp.namespaces('calcMetric')[0]" to string.
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Cannot convert result of " Cpp.namespaces('calcMetric')[0]" to string. FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/

class calcMetric
{

public:
    typedef QSharedPointer<calcMetric> SPtr;            /**< Shared pointer type for calcMetric. */
    typedef QSharedPointer<const calcMetric> ConstSPtr; /**< Const shared pointer type for calcMetric. */

    //=========================================================================================================
    /**
    * Constructs a calcMetric object.
    */
    calcMetric();

    bool calcEn;
    bool historyReady;

    int m_iListLength;
    int m_iFuzzyEnStep;

    void setData(Eigen::MatrixXd input);
    void calcAll(Eigen::MatrixXd input, int dim, double r, double n);
    void calcP2P();
    void calcKurtosis(int start, int end);
    //void calcFuzzyEn(int start, int step, int end, int dim, double r, double n);

    Eigen::VectorXd onSeizureDetection(int dim, double r, double n, QList<int> checkChs);

    Eigen::VectorXd getKurtosis();
    Eigen::VectorXd getP2P();
    Eigen::VectorXd getFuzzyEn();

    Eigen::MatrixXd getKurtosisHistory();
    Eigen::MatrixXd getP2PHistory();
    Eigen::MatrixXd getFuzzyEnHistory();

protected:

private:

    Eigen::MatrixXd m_dmatData;
    Eigen::Matrix<bool, Eigen::Dynamic, 1> m_bFuzzyEnCalc;
    int m_iChannelCount;
    int m_iDataLength;
    int m_iKurtosisHistoryPosition;
    int m_iP2PHistoryPosition;
    int m_iFuzzyEnHistoryPosition;
    int m_iFuzzyEnStart;

    bool setNewP2P;
    bool setNewKurtosis;
    bool setNewFuzzyEn;

    QList<int> fuzzyEnUsedChs;

    Eigen::VectorXd m_dvecP2P;
    Eigen::VectorXd m_dvecKurtosis;
    Eigen::VectorXd m_dvecFuzzyEn;

    Eigen::MatrixXd m_dmatP2PHistory;
    Eigen::MatrixXd m_dmatKurtosisHistory;
    Eigen::MatrixXd m_dmatFuzzyEnHistory;

    Eigen::VectorXd m_dvecStdDev;
    Eigen::VectorXd m_dvecMean;

signals:


};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


#endif // CALCMETRIC_H
