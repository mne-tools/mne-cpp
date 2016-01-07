//=============================================================================================================
/**
* @file     rtdataworker.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2015
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
* @brief    RtDataWorker class declaration
*
*/

#ifndef RTDATAWORKER_H
#define RTDATAWORKER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

#include "../../disp3dnew_global.h"

#include <disp/helpers/colormap.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QList>
#include <QThread>
#include <QSharedPointer>
#include <QMutex>
#include <QTime>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace DISPLIB;


//=============================================================================================================
/**
* Worker which schedules data with the right timing
*
* @brief Data scheduler
*/
class DISP3DNEWSHARED_EXPORT RtDataWorker : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtDataWorker> SPtr;            /**< Shared pointer type for RtDataWorker class. */
    typedef QSharedPointer<const RtDataWorker> ConstSPtr; /**< Const shared pointer type for RtDataWorker class. */

    RtDataWorker(QObject* parent = 0);

    ~RtDataWorker();

    void addData(const MatrixXd& data);

    void clear();

    void setAverage(qint32 samples);

    void setInterval(const int& iMSec);

    void setColormapType(const QString& sColormapType);

    void setNormalization(const double& dValue);

    void setLoop(bool looping);

    void stop();

    void start();

signals:
    void stcSample(QByteArray colorSample);

protected:
    virtual void run();

private:
    QByteArray transformDataToColor(const VectorXd& data);

    QMutex              m_qMutex;

    MatrixXd            m_matData;        /**< List that holds the fiff matrix data <n_channels x n_samples> */

    bool        m_bIsRunning;
    bool        m_bIsLooping;

    qint32      m_iAverageSamples;
    qint32      m_iCurrentSample;
    qint32      m_iMSecIntervall;

    double      m_dNormalization;
    double      m_dNormalizationMax;

    QString     m_sColormap;
};

} // NAMESPACE

#endif // RtDataWorker_H
