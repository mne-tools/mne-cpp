//=============================================================================================================
/**
 * @file     rtinvop.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief     Definition of the RtInvOp Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtinvop.h"

#include <fiff/fiff_info.h>

#include <mne/mne_forwardsolution.h>
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
