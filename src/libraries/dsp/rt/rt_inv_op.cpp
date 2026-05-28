//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rt_inv_op.cpp
 * @since March 2026
 * @brief Definition of the RtInvOp Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_inv_op.h"

#include <fiff/fiff_info.h>

#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace Eigen;
using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS RtInvOpWorker
//=============================================================================================================

void RtInvOpWorker::doWork(const RtInvOpInput &inputData)
{
    if(this->thread()->isInterruptionRequested()) {
        return;
    }

    // Restrict forward solution as necessary for MEG
    MNEForwardSolution forwardMeg = inputData.pFwd->pick_types(true, false);

    MNEInverseOperator invOpMeg(*inputData.pFiffInfo.data(),
                                forwardMeg,
                                inputData.noiseCov,
                                0.2f,
                                0.8f);

    emit resultReady(invOpMeg);
}

//=============================================================================================================
// DEFINE MEMBER METHODS RtInvOp
//=============================================================================================================

RtInvOp::RtInvOp(FiffInfo::SPtr &p_pFiffInfo,
                 MNEForwardSolution::SPtr &p_pFwd,
                 QObject *parent)
: QObject(parent)
, m_pFiffInfo(p_pFiffInfo)
, m_pFwd(p_pFwd)
{
    RtInvOpWorker *worker = new RtInvOpWorker;
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtInvOp::operate,
            worker, &RtInvOpWorker::doWork);

    connect(worker, &RtInvOpWorker::resultReady,
            this, &RtInvOp::handleResults);

    m_workerThread.start();

    qRegisterMetaType<RtInvOpInput>("RtInvOpInput");
}

//=============================================================================================================

RtInvOp::~RtInvOp()
{
    stop();
}

//=============================================================================================================

void RtInvOp::append(const FIFFLIB::FiffCov &noiseCov)
{
    RtInvOpInput inputData;
    inputData.noiseCov = noiseCov;
    inputData.pFiffInfo = m_pFiffInfo;
    inputData.pFwd = m_pFwd;

    emit operate(inputData);
}

//=============================================================================================================

void RtInvOp::setFwdSolution(QSharedPointer<MNELIB::MNEForwardSolution> pFwd)
{
    m_pFwd = pFwd;
}

//=============================================================================================================

void RtInvOp::handleResults(const MNELIB::MNEInverseOperator& invOp)
{
    emit invOperatorCalculated(invOp);
}

//=============================================================================================================

void RtInvOp::restart()
{
    stop();

    RtInvOpWorker *worker = new RtInvOpWorker;
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtInvOp::operate,
            worker, &RtInvOpWorker::doWork);

    connect(worker, &RtInvOpWorker::resultReady,
            this, &RtInvOp::handleResults);

    m_workerThread.start();
}

//=============================================================================================================

void RtInvOp::stop()
{
    m_workerThread.requestInterruption();
    m_workerThread.quit();
    m_workerThread.wait();
}
