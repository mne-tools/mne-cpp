//=============================================================================================================
/**
* @file     detecttrigger.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     July, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    DetectTrigger class declaration
*
*/

#ifndef DETECTTRIGGER_H
#define DETECTTRIGGER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMapIterator>
#include <QSharedPointer>
#include <QTime>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace UTILSLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Routines for detecting trigger flanks in a given signal
*
* @brief Trigger flank detection
*/
class UTILSSHARED_EXPORT DetectTrigger
{

public:
    typedef QSharedPointer<DetectTrigger> SPtr;            /**< Shared pointer type for DetectTrigger class. */
    typedef QSharedPointer<const DetectTrigger> ConstSPtr; /**< Const shared pointer type for DetectTrigger class. */

    //=========================================================================================================
    /**
    * Destroys the DetectTrigger class.
    */
    DetectTrigger();

    //=========================================================================================================
    /**
    * detectTriggerFlanks detects flanks from a given data matrix in row wise order. This function uses a simple maxCoeff function implemented by eigen to locate the triggers.
    *
    * @param[in]    data  the data used to find the trigger flanks
    * @param[out]   qMapDetectedTrigger  This map holds the indices of the channels which are to be read from data. For each index/channel the found triggers are written to the value of the map.
    * @param[in]    iOffsetIndex  the offset index gets added to the found trigger flank index
    * @param[in]    iThreshold  the signal threshold value used to find the trigger flank
    * @param[in]    bRemoveOffset  remove the first sample as offset
    */
    static bool detectTriggerFlanksMax(const MatrixXd &data, QMap<int,QList<int> >& qMapDetectedTrigger, int iOffsetIndex, double dThreshold, bool bRemoveOffset);

    //=========================================================================================================
    /**
    * detectTriggerFlanks detects flanks from a given data matrix in row wise order. This function uses a simple maxCoeff function implemented by eigen to locate the triggers.
    *
    * @param[in]    data  the data used to find the trigger flanks
    * @param[in]    iTriggerChannelIdx  the index of the trigger channel in the matrix.
    * @param[out]   iDetectedTrigger The detected trigger
    * @param[in]    iOffsetIndex  the offset index gets added to the found trigger flank index
    * @param[in]    iThreshold  the signal threshold value used to find the trigger flank
    * @param[in]    bRemoveOffset  remove the first sample as offset
    */
    static bool detectTriggerFlanksMax(const MatrixXd &data, int iTriggerChannelIdx, int &iDetectedTrigger, int iOffsetIndex, double dThreshold, bool bRemoveOffset);

    //=========================================================================================================
    /**
    * detectTriggerFlanksGrad detects flanks from a given data matrix in row wise order. This function uses a simple gradient to locate the triggers.
    *
    * @param[in]    data  the data used to find the trigger flanks
    * @param[out]   qMapDetectedTrigger  This map holds the indices of the channels which are to be read from data. For each index/channel the found triggers are written to the value of the map.
    * @param[in]    iOffsetIndex  the offset index gets added to the found trigger flank index
    * @param[in]    iThreshold  the gradient threshold value used to find the trigger flank
    * @param[in]    bRemoveOffset  remove the first sample as offset
    * @param[in]    type  detect rising or falling flank. Use "Rising" or "Falling" as input
    */
    static bool detectTriggerFlanksGrad(const MatrixXd &data, QMap<int,QList<int> >& qMapDetectedTrigger, int iOffsetIndex, double dThreshold, bool bRemoveOffset, QString type = "Rising");

    //=========================================================================================================
    /**
    * detectTriggerFlanksGrad detects flanks from a given data matrix in row wise order. This function uses a simple gradient to locate the triggers.
    *
    * @param[in]    data  the data used to find the trigger flanks
    * @param[in]    iTriggerChannelIdx  the index of the trigger channel in the matrix.
    * @param[out]   iDetectedTrigger The detected trigger
    * @param[in]    iOffsetIndex  the offset index gets added to the found trigger flank index
    * @param[in]    iThreshold  the gradient threshold value used to find the trigger flank
    * @param[in]    bRemoveOffset  remove the first sample as offset
    * @param[in]    type  detect rising or falling flank. Use "Rising" or "Falling" as input
    */
    static bool detectTriggerFlanksGrad(const MatrixXd &data, int iTriggerChannelIdx, int &iDetectedTrigger, int iOffsetIndex, double dThreshold, bool bRemoveOffset, QString type = "Rising");
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // DETECTTRIGGER_H
