//=============================================================================================================
/**
* @file     rtcov.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     Definition of the RtCov Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcov.h"

#include <fiff/fiff_cov.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS RtCovWorker
//=============================================================================================================

void RtCovWorker::doWork(const RtCovInput &inputData)
{
    QElapsedTimer time;
    time.start();

    if(this->thread()->isInterruptionRequested()) {
        return;
    }

    QFuture<RtCovComputeResult> result = QtConcurrent::mappedReduced(inputData.lData,
                                                                     compute,
                                                                     reduce);

    result.waitForFinished();

    RtCovComputeResult finalResult = result.result();

    //Final computation
    FiffCov computedCov;
    computedCov.data = finalResult.matData;

    QStringList exclude;
    for(int i = 0; i<inputData.fiffInfo.chs.size(); i++) {
        if(inputData.fiffInfo.chs.at(i).kind != FIFFV_MEG_CH &&
           inputData.fiffInfo.chs.at(i).kind != FIFFV_EEG_CH) {
            exclude << inputData.fiffInfo.chs.at(i).ch_name;
        }
    }
    bool doProj = true;

    if(inputData.iSamples > 0) {
        finalResult.mu /= (float)inputData.iSamples;
        computedCov.data.array() -= inputData.iSamples * (finalResult.mu * finalResult.mu.transpose()).array();
        computedCov.data.array() /= (inputData.iSamples - 1);

        computedCov.kind = FIFFV_MNE_NOISE_COV;
        computedCov.diag = false;
        computedCov.dim = computedCov.data.rows();

        //ToDo do picks
        computedCov.names = inputData.fiffInfo.ch_names;
        computedCov.projs = inputData.fiffInfo.projs;
        computedCov.bads = inputData.fiffInfo.bads;
        computedCov.nfree = inputData.iSamples;

        // regularize noise covariance
        computedCov = computedCov.regularize(inputData.fiffInfo, 0.05, 0.05, 0.1, doProj, exclude);

//            qint32 samples = rawSegment.cols();
//            VectorXf mu = rawSegment.rowwise().sum().array() / (float)samples;

//            MatrixXf noise_covariance = rawSegment * rawSegment.transpose();// noise_covariance == raw_covariance
//            noise_covariance.array() -= samples * (mu * mu.transpose()).array();
//            noise_covariance.array() /= (samples - 1);

//            std::cout << "Noise Covariance:\n" << noise_covariance.block(0,0,10,10) << std::endl;

//            printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());
        emit resultReady(computedCov);
    } else {
        qDebug() << "RtCovWorker::doWork - Number of samples equals zero. Regularization not possible. Returning without result.";
    }

    qInfo() << time.elapsed() << m_iBlockNumberReceived++ << "RtCovWorker Time";
}


//*************************************************************************************************************

RtCovComputeResult RtCovWorker::compute(const MatrixXd &matData)
{
    RtCovComputeResult result;
    result.mu = matData.rowwise().sum();
    result.matData = matData * matData.transpose();
    return result;
}


//*************************************************************************************************************

void RtCovWorker::reduce(RtCovComputeResult& finalResult, const RtCovComputeResult &tempResult)
{
    if(finalResult.matData.size() == 0 || finalResult.mu.size() == 0) {
        finalResult.mu = tempResult.mu;
        finalResult.matData = tempResult.matData;
    } else {
        finalResult.mu += tempResult.mu;
        finalResult.matData += tempResult.matData;
    }
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS RtCov
//=============================================================================================================

RtCov::RtCov(qint32 iMaxSamples,
             FiffInfo::SPtr pFiffInfo,
             QObject *parent)
: QObject(parent)
, m_iMaxSamples(iMaxSamples)
, m_pFiffInfo(pFiffInfo)
, m_iSamples(0)
{
    RtCovWorker *worker = new RtCovWorker;
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtCov::operate,
            worker, &RtCovWorker::doWork);

    connect(worker, &RtCovWorker::resultReady,
            this, &RtCov::handleResults);

    m_workerThread.start();

    qRegisterMetaType<RtCovInput>("RtCovInput");
}


//*************************************************************************************************************

RtCov::~RtCov()
{
    stop();
}


//*************************************************************************************************************

void RtCov::setSamples(qint32 samples)
{
    m_iMaxSamples = samples;
}


//*************************************************************************************************************

void RtCov::append(const MatrixXd &matDataSegment)
{
    m_lData.append(matDataSegment);
    m_iSamples += matDataSegment.cols();

    if(m_iSamples >= m_iMaxSamples) {
        RtCovInput inputData;
        inputData.lData = m_lData;
        inputData.fiffInfo = FiffInfo(*m_pFiffInfo);
        inputData.iSamples = m_iSamples;

        emit operate(inputData);

        m_iSamples = 0;
        m_lData.clear();
    }
}


//*************************************************************************************************************

void RtCov::handleResults(const FIFFLIB::FiffCov& computedCov)
{
    emit covCalculated(computedCov);
}


//*************************************************************************************************************

void RtCov::restart()
{
    stop();

    RtCovWorker *worker = new RtCovWorker;
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtCov::operate,
            worker, &RtCovWorker::doWork);

    connect(worker, &RtCovWorker::resultReady,
            this, &RtCov::handleResults);

    m_workerThread.start();
}


//*************************************************************************************************************

void RtCov::stop()
{
    m_workerThread.requestInterruption();
    m_workerThread.quit();
    m_workerThread.wait();
}
