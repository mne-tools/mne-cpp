//=============================================================================================================
/**
* @file     realtimemultisamplearrayworker.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Declaration of the RealTimeMultiSampleArrayWorker Class.
*
*/

#ifndef REALTIMEMULTISAMPLEARRAYWORKER_H
#define REALTIMEMULTISAMPLEARRAYWORKER_H

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QVector>
#include <QSharedPointer>
#include <QMutex>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


using namespace Eigen;


class RealTimeMultiSampleArrayWorker : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<RealTimeMultiSampleArrayWorker> SPtr;            /**< Shared pointer type for RealTimeMultiSampleArrayWorker class. */
    typedef QSharedPointer<const RealTimeMultiSampleArrayWorker> ConstSPtr; /**< Const shared pointer type for RealTimeMultiSampleArrayWorker class. */

    RealTimeMultiSampleArrayWorker(QObject *parent = 0);

    void addData(const QVector<VectorXd> &data);

    void clear();

    void process();

    void setDownSampling(qint32 ds);

signals:
    void stcSample(VectorXd sample);

private:
    QMutex m_qMutex;
    QVector<VectorXd> m_data;   /**< List that holds the fiff matrix data <n_channels x n_samples> */

    qint32 m_iCurrentSample;    /**< Accurate Downsampling */
    qint32 m_iDownSampling;
};

#endif // REALTIMEMULTISAMPLEARRAYWORKER_H
