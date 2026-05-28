//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_connectivity.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Definition of the RtConnectivity Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_connectivity.h"

#include <conn/connectivitysettings.h>
#include <conn/connectivity.h>

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
using namespace CONNLIB;

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
