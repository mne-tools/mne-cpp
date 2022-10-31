//=============================================================================================================
/**
 * @file     detecttrigger.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.3
 * @date     June, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    DetectTrigger definitions.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "detecttrigger.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMapIterator>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE RTPROCESSINGLIB GLOBAL METHODS
//=============================================================================================================

QList<MatrixXi> RTPROCESSINGLIB::toEventMatrix(QMap<int,QList<QPair<int,double> > > mapTriggers)
{
    QList<MatrixXi> lMatDetectedTrigger;

    QMapIterator<int,QList<QPair<int,double> > > idx(mapTriggers);

    while (idx.hasNext()) {
        idx.next();
        MatrixXi matDetectedTrigger(idx.value().size(),3);

        for(int i = 0; i < idx.value().size(); ++i) {
            matDetectedTrigger(i,0) = idx.value().at(i).first;
            matDetectedTrigger(i,1) = 0;
            matDetectedTrigger(i,2) = idx.value().at(i).second;
        }

        lMatDetectedTrigger << matDetectedTrigger;
    }

    return lMatDetectedTrigger;
}

//=============================================================================================================

QMap<int,QList<QPair<int,double> > > RTPROCESSINGLIB::detectTriggerFlanksMax(const MatrixXd &data,
                                                                             const QList<int>& lTriggerChannels,
                                                                             int iOffsetIndex,
                                                                             double dThreshold,
                                                                             bool bRemoveOffset,
                                                                             int iBurstLengthSamp)
{
    QMap<int,QList<QPair<int,double> > > qMapDetectedTrigger;

    //Find all triggers above threshold in the data block
    for(int i = 0; i < lTriggerChannels.size(); ++i) {
        int iChIdx = lTriggerChannels.at(i);

        //Add empty list to map
        QList<QPair<int,double> > temp;
        qMapDetectedTrigger.insert(iChIdx, temp);

        //detect the actual triggers in the current data matrix
        if(iChIdx > data.rows() || iChIdx < 0) {
            return qMapDetectedTrigger;
        }

        //Find positive maximum in data vector.
        for(int j = 0; j < data.cols(); ++j) {
            double dMatVal = bRemoveOffset ? data(iChIdx,j) - data(iChIdx,0) : data(iChIdx,j);

            if(dMatVal >= dThreshold) {
                QPair<int,double> pair;
                pair.first = iOffsetIndex+j;
                pair.second = data(iChIdx,j);

                qMapDetectedTrigger[iChIdx].append(pair);

                j += iBurstLengthSamp;
            }
        }
    }

    return qMapDetectedTrigger;
}

//=============================================================================================================

QList<QPair<int,double> > RTPROCESSINGLIB::detectTriggerFlanksMax(const MatrixXd &data,
                                                                  int iTriggerChannelIdx,
                                                                  int iOffsetIndex,
                                                                  double dThreshold,
                                                                  bool bRemoveOffset,
                                                                  int iBurstLengthSamp)
{
    QList<QPair<int,double> > lDetectedTriggers;

    //Find all triggers above threshold in the data block
    //detect the actual triggers in the current data matrix
    if(iTriggerChannelIdx > data.rows() || iTriggerChannelIdx < 0) {
        return lDetectedTriggers;
    }

    //Find positive maximum in data vector.
    for(int j = 0; j < data.cols(); ++j) {
        double dMatVal = bRemoveOffset ? data(iTriggerChannelIdx,j) - data(iTriggerChannelIdx,0) : data(iTriggerChannelIdx,j);

        if(dMatVal >= dThreshold) {
            QPair<int,double> pair;
            pair.first = iOffsetIndex+j;
            pair.second = data(iTriggerChannelIdx,j);

            lDetectedTriggers.append(pair);

            j += iBurstLengthSamp;
        }
    }

    return lDetectedTriggers;
}

//=============================================================================================================

QMap<int,QList<QPair<int,double> > > RTPROCESSINGLIB::detectTriggerFlanksGrad(const MatrixXd& data,
                                                                              const QList<int>& lTriggerChannels,
                                                                              int iOffsetIndex,
                                                                              double dThreshold,
                                                                              bool bRemoveOffset,
                                                                              const QString& type,
                                                                              int iBurstLengthSamp)
{
    QMap<int,QList<QPair<int,double> > > qMapDetectedTrigger;
    RowVectorXd tGradient = RowVectorXd::Zero(data.cols());

    //Find all triggers above threshold in the data block
    for(int i = 0; i < lTriggerChannels.size(); ++i) {
        int iChIdx = lTriggerChannels.at(i);

        //Add empty list to map
        QList<QPair<int,double> > temp;
        qMapDetectedTrigger.insert(iChIdx, temp);

        //detect the actual triggers in the current data matrix
        if(iChIdx > data.rows() || iChIdx < 0) {
            return qMapDetectedTrigger;
        }

        //Compute gradient
        for(int t = 1; t<tGradient.cols(); t++) {
            tGradient(t) = data(iChIdx,t)-data(iChIdx,t-1);
        }

        // If falling flanks are to be detected flip the gradient's sign
        if(type == "Falling") {
            tGradient = tGradient * -1;
        }

        //Find positive maximum in gradient vector. This position is equal to the rising trigger flank.
        for(int j = 0; j < tGradient.cols(); ++j) {
            double dMatVal = bRemoveOffset ? tGradient(j) - data(iChIdx,0) : tGradient(j);

            if(dMatVal >= dThreshold) {
                QPair<int,double> pair;
                pair.first = iOffsetIndex+j;
                pair.second = tGradient(j);

                qMapDetectedTrigger[iChIdx].append(pair);

                j += iBurstLengthSamp;
            }
        }
    }

    return qMapDetectedTrigger;
}

//=============================================================================================================

QList<QPair<int,double> > RTPROCESSINGLIB::detectTriggerFlanksGrad(const MatrixXd &data,
                                                                   int iTriggerChannelIdx,
                                                                   int iOffsetIndex,
                                                                   double dThreshold,
                                                                   bool bRemoveOffset,
                                                                   const QString& type,
                                                                   int iBurstLengthSamp)
{
    QList<QPair<int,double> > lDetectedTriggers;

    RowVectorXd tGradient = RowVectorXd::Zero(data.cols());

    //detect the actual triggers in the current data matrix
    if(iTriggerChannelIdx > data.rows() || iTriggerChannelIdx < 0) {
        return lDetectedTriggers;
    }

    //Compute gradient
    for(int t = 1; t < tGradient.cols(); ++t) {
        tGradient(t) = data(iTriggerChannelIdx,t) - data(iTriggerChannelIdx,t-1);
    }

    //If falling flanks are to be detected flip the gradient's sign
    if(type == "Falling") {
        tGradient = tGradient * -1;
    }

    //Find all triggers above threshold in the data block
    for(int j = 0; j < tGradient.cols(); ++j) {
        double dMatVal = bRemoveOffset ? tGradient(j) - data(iTriggerChannelIdx,0) : tGradient(j);

        if(dMatVal >= dThreshold) {
            QPair<int,double> pair;
            pair.first = iOffsetIndex+j;
            pair.second = tGradient(j);

            lDetectedTriggers.append(pair);

            j += iBurstLengthSamp;
        }
    }

    return lDetectedTriggers;
}
