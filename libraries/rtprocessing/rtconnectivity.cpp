//=============================================================================================================
/**
 * @file     rtconnectivity.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtconnectivity.h"

#include <connectivity/connectivitysettings.h>
#include <connectivity/connectivity.h>
#include <connectivity/network/network.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace CONNECTIVITYLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS RtConnectivityWorker
//=============================================================================================================

void RtConnectivityWorker::doWork(const ConnectivitySettings &connectivitySettings)
{
    if(this->thread()->isInterruptionRequested()) {
        return;
    }

    if(connectivitySettings.getConnectivityMethods().isEmpty()) {
        qDebug()<<"RtConnectivityWorker::doWork() - Network methods are empty";
        return;
    }

    ConnectivitySettings connectivitySettingsTemp = connectivitySettings;

    QElapsedTimer time;
    qint64 iTime = 0;
    time.start();

    QList<Network> finalNetworks = Connectivity::calculate(connectivitySettingsTemp);

//    iTime = time.elapsed();

//    qDebug()<<"----------------------------------------";
//    qDebug()<<"----------------------------------------";
//    qDebug()<<"------RtConnectivityWorker::doWork()";
//    qDebug()<<"------Method:"<<connectivitySettings.getConnectivityMethods().first();
//    qDebug()<<"------Data dim:"<<connectivitySettings.at(0).matData.rows() << "x" << connectivitySettings.at(0).matData.cols();
//    qDebug()<<"------Number trials:"<< connectivitySettings.size();
//    qDebug()<<"------Total time:"<<iTime << "ms";
//    qDebug()<<"----------------------------------------";
//    qDebug()<<"----------------------------------------";

    emit resultReady(finalNetworks, connectivitySettingsTemp);
}

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
            this, &RtConnectivity::newConnectivityResultAvailable);

    m_workerThread.start();
}

//=============================================================================================================

RtConnectivity::~RtConnectivity()
{
    stop();
}

//=============================================================================================================

void RtConnectivity::append(const ConnectivitySettings& connectivitySettings)
{
    emit operate(connectivitySettings);
}

//=============================================================================================================

void RtConnectivity::restart()
{
    stop();

    RtConnectivityWorker *worker = new RtConnectivityWorker;
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtConnectivity::operate,
            worker, &RtConnectivityWorker::doWork);

    connect(worker, &RtConnectivityWorker::resultReady,
            this, &RtConnectivity::newConnectivityResultAvailable);

    m_workerThread.start();
}

//=============================================================================================================

void RtConnectivity::stop()
{
    m_workerThread.requestInterruption();
    m_workerThread.quit();
    m_workerThread.wait();
}
