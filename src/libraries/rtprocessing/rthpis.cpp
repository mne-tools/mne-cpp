//=============================================================================================================
/**
 * @file     rthpis.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief     Definition of the RtHpi Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rthpis.h"

#include <inverse/hpiFit/hpifit.h>
#include <inverse/hpiFit/sensorset.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;
using namespace INVERSELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS RtHpiWorker
//=============================================================================================================

RtHpiWorker::RtHpiWorker(const SensorSet sensorSet)
{
    m_pHpiFit = QSharedPointer<INVERSELIB::HPIFit>(new HPIFit(sensorSet));
}

void RtHpiWorker::doWork(const Eigen::MatrixXd& matData,
                         const Eigen::MatrixXd& matProjectors,
                         const HpiModelParameters& hpiModelParameters,
                         const Eigen::MatrixXd& matCoilsHead)
{
    if(this->thread()->isInterruptionRequested()) {
        return;
    }

    //Perform actual fitting
    HpiFitResult fitResult;
    fitResult.devHeadTrans.from = 1;
    fitResult.devHeadTrans.to = 4;

    m_pHpiFit->fit(matData,
                   matProjectors,
                   hpiModelParameters,
                   matCoilsHead,
                   fitResult);

    emit resultReady(fitResult);
}

//=============================================================================================================
// DEFINE MEMBER METHODS RtHpi
//=============================================================================================================

RtHpi::RtHpi(const SensorSet sensorSet, QObject *parent)
: QObject(parent),
  m_sensorSet(sensorSet)
{
    qRegisterMetaType<INVERSELIB::HpiFitResult>("INVERSELIB::HpiFitResult");
    qRegisterMetaType<QVector<int> >("QVector<int>");
    qRegisterMetaType<QSharedPointer<FIFFLIB::FiffInfo> >("QSharedPointer<FIFFLIB::FiffInfo>");
    qRegisterMetaType<Eigen::MatrixXd>("Eigen::MatrixXd");

    RtHpiWorker *worker = new RtHpiWorker(m_sensorSet);
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtHpi::operate,
            worker, &RtHpiWorker::doWork);

    connect(worker, &RtHpiWorker::resultReady,
            this, &RtHpi::handleResults);

    m_workerThread.start();
}

//=============================================================================================================

RtHpi::~RtHpi()
{
    stop();
}

//=============================================================================================================

void RtHpi::append(const MatrixXd &data)
{
    if(m_modelParameters.iNHpiCoils() >= 3) {
        emit operate(data,
                     m_matProjectors,
                     m_modelParameters,
                     m_matCoilsHead);
    } else {
        qWarning() << "[RtHpi::append] Not enough coil frequencies set. At least three frequencies are needed.";
    }
}

//=============================================================================================================

void RtHpi::setModelParameters(HpiModelParameters hpiModelParameters)
{
    m_modelParameters = hpiModelParameters;
}

//=============================================================================================================

void RtHpi::setProjectionMatrix(const Eigen::MatrixXd& matProjectors)
{
    m_matProjectors = matProjectors;
}

//=============================================================================================================

void RtHpi::setHpiDigitizer(const Eigen::MatrixXd& matCoilsHead)
{
    m_matCoilsHead = matCoilsHead;
}

//=============================================================================================================

void RtHpi::handleResults(const INVERSELIB::HpiFitResult& fitResult)
{
    emit newHpiFitResultAvailable(fitResult);
}

//=============================================================================================================

void RtHpi::restart()
{
    stop();

    RtHpiWorker *worker = new RtHpiWorker(m_sensorSet);
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtHpi::operate,
            worker, &RtHpiWorker::doWork);

    connect(worker, &RtHpiWorker::resultReady,
            this, &RtHpi::handleResults);

    m_workerThread.start();
}

//=============================================================================================================

void RtHpi::stop()
{
    m_workerThread.requestInterruption();
    m_workerThread.quit();
    m_workerThread.wait();
}
