//=============================================================================================================
/**
* @file     rthpis.cpp
* @author   Chiran Doshi <chiran.doshi@childrens.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@ntu-ilmenau.de>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*
* @version  1.0
* @date     November, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Chiran Doshi, Lorenz Esch, Limin Sun, and Matti Hamalainen. All rights reserved.
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
* @brief     Definition of the RtHPIS Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rthpis.h"

#include <inverse/hpiFit/hpifit.h>
#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace REALTIMELIB;
using namespace FIFFLIB;
using namespace Eigen;
using namespace IOBUFFER;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS RtHPISWorker
//=============================================================================================================

void RtHPISWorker::doWork(const Eigen::MatrixXd& matData,
            const Eigen::MatrixXd& m_matProjectors,
            const QVector<int>& vFreqs,
            QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo)
{
    if(this->thread()->isInterruptionRequested()) {
        return;
    }

    //Perform actual fitting
    FittingResult fitResult;
    fitResult.devHeadTrans.from = 1;
    fitResult.devHeadTrans.to = 4;

    HPIFit::fitHPI(matData,
                    m_matProjectors,
                    fitResult.devHeadTrans,
                    vFreqs,
                    fitResult.errorDistances,
                    fitResult.fittedCoils,
                    pFiffInfo);

    emit resultReady(fitResult);
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS RtHPIS
//=============================================================================================================

RtHPIS::RtHPIS(FiffInfo::SPtr p_pFiffInfo, QObject *parent)
: QObject(parent)
, m_pFiffInfo(p_pFiffInfo)
{
    qRegisterMetaType<REALTIMELIB::FittingResult>("REALTIMELIB::FittingResult");
    qRegisterMetaType<QVector<int> >("QVector<int>");
    qRegisterMetaType<QSharedPointer<FIFFLIB::FiffInfo> >("QSharedPointer<FIFFLIB::FiffInfo>");

    RtHPISWorker *worker = new RtHPISWorker;
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtHPIS::operate,
            worker, &RtHPISWorker::doWork);

    connect(worker, &RtHPISWorker::resultReady,
            this, &RtHPIS::handleResults);

    m_workerThread.start();
}


//*************************************************************************************************************

RtHPIS::~RtHPIS()
{
    stop();
}


//*************************************************************************************************************

void RtHPIS::append(const MatrixXd &data)
{
    emit operate(data,
                 m_matProjectors,
                 m_vCoilFreqs,
                 m_pFiffInfo);
}


//*************************************************************************************************************

void RtHPIS::setCoilFrequencies(const QVector<int>& vCoilFreqs)
{
    m_vCoilFreqs = vCoilFreqs;
}


//*************************************************************************************************************

void RtHPIS::setProjectionMatrix(const Eigen::MatrixXd& matProjectors)
{
    m_matProjectors = matProjectors;
}


//*************************************************************************************************************

void RtHPIS::handleResults(const REALTIMELIB::FittingResult& fitResult)
{
    emit newFittingResultAvailable(fitResult);
}


//*************************************************************************************************************

void RtHPIS::restart()
{
    stop();

    RtHPISWorker *worker = new RtHPISWorker;
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtHPIS::operate,
            worker, &RtHPISWorker::doWork);

    connect(worker, &RtHPISWorker::resultReady,
            this, &RtHPIS::handleResults);

    m_workerThread.start();
}


//*************************************************************************************************************

void RtHPIS::stop()
{
    m_workerThread.requestInterruption();
    m_workerThread.quit();
    m_workerThread.wait();
}
