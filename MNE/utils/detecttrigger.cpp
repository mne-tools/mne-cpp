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
// STL INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMapIterator>
#include <QTime>


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

QMap<int,QList<int> > DetectTrigger::detectTriggerFlanksMax(const MatrixXd &data, const QList<int>& lTriggerChannels, int iOffsetIndex, double dThreshold, bool bRemoveOffset)
{
    QMap<int,QList<int> > qMapDetectedTrigger;

    //TODO: This only can detect one trigger per data block. What if there are more than one trigger in the data block?
    for(int i = 0; i < lTriggerChannels.size(); ++i)
    {
//        QTime time;
//        time.start();

        //detect the actual triggers in the current data matrix
        if(lTriggerChannels.at(i) > data.rows() || lTriggerChannels.at(i) < 0)
        {
            return qMapDetectedTrigger;
        }

        RowVectorXd::Index indexMaxCoeff;
        int dMax = data.row(lTriggerChannels.at(i)).maxCoeff(&indexMaxCoeff);
        Q_UNUSED(dMax);

        //Find trigger using gradient/difference. Also subtract first value as offset, like in the display
        double maxValue = data.row(lTriggerChannels.at(i))(indexMaxCoeff);
        if(bRemoveOffset)
        {
            maxValue -= data.row(lTriggerChannels.at(i))(0);
        }

        if(maxValue>dThreshold)
        {
            qMapDetectedTrigger[lTriggerChannels.at(i)].append((int)iOffsetIndex+indexMaxCoeff);
        }

//        int timeElapsed = time.elapsed();
//        std::cout<<"timeElapsed: "<<timeElapsed<<std::endl;
    }

    return qMapDetectedTrigger;
}


//*************************************************************************************************************

QList<int> DetectTrigger::detectTriggerFlanksMax(const MatrixXd &data, int iTriggerChannelIdx, int iOffsetIndex, double dThreshold, bool bRemoveOffset)
{
    QList<int> lDetectedTriggers;

    //TODO: This only can detect one trigger per data block. What if there are more than one trigger in the data block?
//        QTime time;
//        time.start();

    //detect the actual triggers in the current data matrix
    if(iTriggerChannelIdx > data.rows() || iTriggerChannelIdx < 0)
    {
        return lDetectedTriggers;
    }

    RowVectorXd::Index indexMaxCoeff;
    int dMax = data.row(iTriggerChannelIdx).maxCoeff(&indexMaxCoeff);
    Q_UNUSED(dMax);

    //Find trigger using gradient/difference. Also subtract first value as offset, like in the display
    double maxValue = data.row(iTriggerChannelIdx)(indexMaxCoeff);
    if(bRemoveOffset)
        maxValue -= data.row(iTriggerChannelIdx)(0);

    if(maxValue>dThreshold) {
        lDetectedTriggers.append((int)iOffsetIndex+indexMaxCoeff);
    }

//        int timeElapsed = time.elapsed();
//        std::cout<<"timeElapsed: "<<timeElapsed<<std::endl;

    return lDetectedTriggers;
}


//*************************************************************************************************************

QMap<int,QList<int> > DetectTrigger::detectTriggerFlanksGrad(const MatrixXd &data, const QList<int>& lTriggerChannels, int iOffsetIndex, double dThreshold, bool bRemoveOffset, const QString& type)
{
    QMap<int,QList<int> > qMapDetectedTrigger;
    RowVectorXd tGradient = RowVectorXd::Zero(data.cols());

    //TODO: This only can detect one trigger per data block. What if there are more than one trigger in the data block?
    for(int i = 0; i < lTriggerChannels.size(); ++i)
    {
//        QTime time;
//        time.start();

        //detect the actual triggers in the current data matrix
        if(lTriggerChannels.at(i) > data.rows() || lTriggerChannels.at(i) < 0)
        {
            return qMapDetectedTrigger;
        }

        //Compute gradient
        for(int t = 1; t<tGradient.cols(); t++)
        {
            tGradient(t) = data.row(lTriggerChannels.at(i))(t)-data.row(lTriggerChannels.at(i))(t-1);
        }

        // If falling flanks are to be detected flip the gradient's sign
        if(type == "Falling")
        {
            tGradient = tGradient * -1;
        }

        //Find positive maximum in gradient vector. This position is equal to the rising trigger flank.
        RowVectorXd::Index indexMaxGrad;
        int dMax = tGradient.maxCoeff(&indexMaxGrad);
        Q_UNUSED(dMax);

        //Compare to calculated threshold
        double maxValue = data.row(lTriggerChannels.at(i))(indexMaxGrad);
        if(bRemoveOffset)
        {
            maxValue -= data.row(lTriggerChannels.at(i))(0);
        }

        if(maxValue>dThreshold)
        {
            qMapDetectedTrigger[lTriggerChannels.at(i)].append((int)iOffsetIndex+indexMaxGrad);
        }

//        int timeElapsed = time.elapsed();
//        std::cout<<"timeElapsed: "<<timeElapsed<<std::endl;
    }

    return qMapDetectedTrigger;
}


//*************************************************************************************************************

QList<int> DetectTrigger::detectTriggerFlanksGrad(const MatrixXd &data, int iTriggerChannelIdx, int iOffsetIndex, double dThreshold, bool bRemoveOffset, const QString& type)
{
    QList<int> lDetectedTriggers;

    //TODO: This only can detect one trigger per data block. What if there are more than one trigger in the data block?
    RowVectorXd tGradient = RowVectorXd::Zero(data.cols());

//        QTime time;
//        time.start();

    //detect the actual triggers in the current data matrix
    if(iTriggerChannelIdx > data.rows() || iTriggerChannelIdx < 0)
    {
        return lDetectedTriggers;
    }

    //Compute gradient
    for(int t = 1; t<tGradient.cols(); t++)
    {
        tGradient(t) = data.row(iTriggerChannelIdx)(t)-data.row(iTriggerChannelIdx)(t-1);
    }

    // If falling flanks are to be detected flip the gradient's sign
    if(type == "Falling")
    {
        tGradient = tGradient * -1;
    }

    //Find positive maximum in gradient vector. This position is equal to the rising trigger flank.
    RowVectorXd::Index indexMaxGrad;
    int dMax = tGradient.maxCoeff(&indexMaxGrad);
    Q_UNUSED(dMax);

    //Compare to calculated threshold
    double maxValue = data.row(iTriggerChannelIdx)(indexMaxGrad);
    if(bRemoveOffset)
    {
        maxValue -= data.row(iTriggerChannelIdx)(0);
    }

    if(maxValue>dThreshold)
    {
        lDetectedTriggers.append((int)iOffsetIndex+indexMaxGrad);
    }

//        int timeElapsed = time.elapsed();
//        std::cout<<"timeElapsed: "<<timeElapsed<<std::endl;

    return lDetectedTriggers;
}



