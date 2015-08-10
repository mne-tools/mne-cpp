//=============================================================================================================
/**
* @file     detecttrigger.cpp
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
* @brief    Implementation of the DetectTrigger class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "detecttrigger.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DetectTrigger::DetectTrigger()
{

}


//*************************************************************************************************************

void DetectTrigger::detectTriggerFlanksMax(const MatrixXd &data, QMap<int,QList<int> >& qMapDetectedTrigger, int iOffsetIndex, double dThreshold, bool bRemoveOffset)
{
    //TODO: This only can detect one trigger per data block. What iff there are more than one trigger in the data block?
    QMapIterator<int,QList<int> > i(qMapDetectedTrigger);
    while (i.hasNext()) {
        i.next();
        //detect the actual triggers in the current data matrix
        if(i.key() > data.rows()) {
            return;
        }

        RowVectorXd::Index indexMaxCoeff;
        int dMax = data.row(i.key()).maxCoeff(&indexMaxCoeff);
        Q_UNUSED(dMax);

        //Find trigger using gradient/difference. Also subtract first value as offset, like in the display
        double maxValue = data.row(i.key())(indexMaxCoeff);
        if(bRemoveOffset)
            maxValue -= data.row(i.key())(0);

        if(maxValue>dThreshold)
            qMapDetectedTrigger[i.key()].append((int)iOffsetIndex+indexMaxCoeff);
    }
}


//*************************************************************************************************************

void DetectTrigger::detectTriggerFlanksGrad(const MatrixXd &data, QMap<int,QList<int> >& qMapDetectedTrigger, int iOffsetIndex)
{
    //TODO: This only can detect one trigger per data block. What iff there are more than one trigger in the data block?
    RowVectorXd tGradient = RowVectorXd::Zero(data.cols());

    QMapIterator<int,QList<int> > i(qMapDetectedTrigger);
    while (i.hasNext()) {
        QTime time;
        time.start();

        i.next();
        //detect the actual triggers in the current data matrix
        if(i.key() > data.rows()) {
            return;
        }

        //Compute gradient
        for(int t = 1; t<tGradient.cols(); t++)
            tGradient(t) = data.row(i.key())(t)-data.row(i.key())(t-1);

        //Find psotive maximum in gradient vector. This position is equal to the rising trigger flank.
        RowVectorXd::Index indexMaxGrad;
        int dMax = tGradient.maxCoeff(&indexMaxGrad);
        Q_UNUSED(dMax);

        //Calculate dynamic threshold
        RowVectorXd::Index indexMinGrad;
        int dMin = data.row(i.key()).minCoeff(&indexMinGrad);
        Q_UNUSED(dMin);

        double tThreshold = 0;
        if(indexMinGrad-1<0)
            tThreshold = data.row(i.key())(indexMinGrad+1) - data.row(i.key())(indexMinGrad);
        else
            tThreshold = data.row(i.key())(indexMinGrad) - data.row(i.key())(indexMinGrad-1);

        //Compare to calculated threshold
        if(tGradient(indexMaxGrad)>2*tThreshold)
            qMapDetectedTrigger[i.key()].append((int)iOffsetIndex+indexMaxGrad);

        std::cout<<"tGradient(indexMaxGrad): "<<tGradient(indexMaxGrad)<<std::endl;
        std::cout<<"tThreshold: "<<tThreshold<<std::endl;

        int timeElapsed = time.elapsed();
        std::cout<<"timeElapsed: "<<timeElapsed<<std::endl;
    }
}


