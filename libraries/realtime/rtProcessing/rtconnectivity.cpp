//=============================================================================================================
/**
* @file     rtconnectivity.cpp
* @author   Lorenz Esch <Lorenz.Esch@ntu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*
* @version  1.0
* @date     July, 2018
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
* @brief     Definition of the RtConnectivity Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtconnectivity.h"

#include <connectivity/connectivitysettings.h>
#include <connectivity/connectivity.h>
#include <connectivity/network/network.h>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace REALTIMELIB;
using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS RtConnectivityWorker
//=============================================================================================================

void RtConnectivityWorker::doWork(const ConnectivitySettings &connectivitySettings)
{
    QElapsedTimer time;
    int iTime = 0;
    time.start();

    Connectivity tConnectivity(connectivitySettings);
    Network finalNetwork = tConnectivity.calculateConnectivity();

    iTime = time.elapsed();

    emit resultReady(finalNetwork);
    qDebug()<<"----------------------------------------";
    qDebug()<<"----------------------------------------";
    qDebug()<<"RtConnectivityWorker::doWork()";
    qDebug()<<"Method:"<<connectivitySettings.m_sConnectivityMethods.first();
    qDebug()<<"Data dim:"<<connectivitySettings.m_matDataList.first().rows() << "x"<<connectivitySettings.m_matDataList.first().cols();
    qDebug()<<"Number trials:"<< connectivitySettings.m_matDataList.size();
    qDebug()<<"Total time:"<<iTime << "ms";
    qDebug()<<"----------------------------------------";
    qDebug()<<"----------------------------------------";
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS RtConnectivity
//=============================================================================================================

RtConnectivity::RtConnectivity(QObject *parent)
: QObject(parent)
{
    RtConnectivityWorker *worker = new RtConnectivityWorker;
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtConnectivity::operate,
            worker, &RtConnectivityWorker::doWork);

    connect(worker, &RtConnectivityWorker::resultReady,
            this, &RtConnectivity::handleResults);

    m_workerThread.start();
}


//*************************************************************************************************************

RtConnectivity::~RtConnectivity()
{
    m_workerThread.quit();
    m_workerThread.wait();
}


//*************************************************************************************************************

void RtConnectivity::append(const ConnectivitySettings& connectivitySettings)
{
    emit operate(connectivitySettings);
}


//*************************************************************************************************************

void RtConnectivity::handleResults(const Network& connectivityResult)
{
    emit newConnectivityResultAvailable(connectivityResult);
}


//*************************************************************************************************************

void RtConnectivity::reset()
{
    m_workerThread.requestInterruption();
}
