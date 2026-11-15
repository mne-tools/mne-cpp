//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_hpis.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Definition of the RtHpi Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_hpis.h"
#include <memory>
#include <base/log.h>

#include <inv/hpi/inv_hpi_fit.h>
#include <inv/hpi/inv_sensor_set.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS RtHpiWorker
//=============================================================================================================

RtHpiWorker::RtHpiWorker(const InvSensorSet sensorSet)
{
    m_pHpiFit = std::make_shared<InvHpiFit>(sensorSet);
}

void RtHpiWorker::doWork(const Eigen::MatrixXd& matData,
                         const Eigen::MatrixXd& matProjectors,
                         const InvHpiModelParameters& hpiModelParameters,
                         const Eigen::MatrixXd& matCoilsHead)
{
    if (this->thread()->isInterruptionRequested()) {
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

RtHpi::RtHpi(const InvSensorSet sensorSet, QObject* parent)
: QObject(parent)
, m_sensorSet(sensorSet)
{
    qRegisterMetaType<mne::HpiFitResult>("mne::HpiFitResult");
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<std::shared_ptr<mne::FiffInfo>>();
    qRegisterMetaType<Eigen::MatrixXd>("Eigen::MatrixXd");

    RtHpiWorker* worker = new RtHpiWorker(m_sensorSet);
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

void RtHpi::append(const MatrixXd& data)
{
    if (m_modelParameters.iNHpiCoils() >= 3) {
        emit operate(data,
                     m_matProjectors,
                     m_modelParameters,
                     m_matCoilsHead);
    } else {
        mne::Log::warning("[RtHpi::append] Not enough coil frequencies set. At least three frequencies are needed.");
    }
}

//=============================================================================================================

void RtHpi::setModelParameters(InvHpiModelParameters hpiModelParameters)
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

void RtHpi::handleResults(const mne::HpiFitResult& fitResult)
{
    emit newHpiFitResultAvailable(fitResult);
}

//=============================================================================================================

void RtHpi::restart()
{
    stop();

    RtHpiWorker* worker = new RtHpiWorker(m_sensorSet);
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
