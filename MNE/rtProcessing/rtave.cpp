//=============================================================================================================
/**
* @file     rtave.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     implementation of the RtCov Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtave.h"

#include <utils/ioutils.h>
#include <utils/detecttrigger.h>
#include <utils/mnemath.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutexLocker>
#include <QDebug>
#include <QSet>
#include <QList>
#include <QVariant>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtAve::RtAve(quint32 numAverages,
             quint32 p_iPreStimSamples,
             quint32 p_iPostStimSamples,
             quint32 p_iBaselineFromSecs,
             quint32 p_iBaselineToSecs,
             quint32 p_iTriggerIndex,
             FiffInfo::SPtr p_pFiffInfo,
             QObject *parent)
: QThread(parent)
, m_iNumAverages(numAverages)
, m_iPreStimSamples(p_iPreStimSamples)
, m_iPostStimSamples(p_iPostStimSamples)
, m_iPreStimSeconds(p_iPreStimSamples/p_pFiffInfo->sfreq)
, m_iPostStimSeconds(p_iPostStimSamples/p_pFiffInfo->sfreq)
, m_pFiffInfo(p_pFiffInfo)
, m_bIsRunning(false)
, m_bAutoAspect(true)
, m_fTriggerThreshold(0.5)
, m_bFillingBackBuffer(false)
, m_iTriggerIndex(-1)
, m_iNewTriggerIndex(p_iTriggerIndex)
, m_iTriggerPos(-1)
, m_iAverageMode(0)
, m_iNewAverageMode(0)
, m_bDoBaselineCorrection(false)
, m_pairBaselineSec(qMakePair(QVariant(QString::number(p_iBaselineFromSecs)),QVariant(QString::number(p_iBaselineToSecs))))
, m_pStimEvoked(FiffEvoked::SPtr(new FiffEvoked))
, m_iMatDataPostIdx(0)
, m_iNumberCalcAverages(0)
, m_iCurrentBlockSize(0)
, m_dArtifactThreshold(300e-6)
, m_bDoArtifactReduction(false)
{
    qRegisterMetaType<FiffEvoked::SPtr>("FiffEvoked::SPtr");

    init();
}


//*************************************************************************************************************

RtAve::~RtAve()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

void RtAve::append(const MatrixXd &p_DataSegment)
{
    m_qMutex.lock();
    // ToDo handle change buffersize
    if(!m_pRawMatrixBuffer)
        m_pRawMatrixBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(32, p_DataSegment.rows(), p_DataSegment.cols()));

    m_pRawMatrixBuffer->push(&p_DataSegment);

    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setAverages(qint32 numAve)
{
    m_qMutex.lock();
    m_iNewNumAverages = numAve;
    m_qMutex.unlock();
    emit numAveragesChanged();
}


//*************************************************************************************************************

void RtAve::setAverageMode(qint32 mode)
{
    m_qMutex.lock();
    m_iNewAverageMode = mode;
    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setPreStim(qint32 samples, qint32 secs)
{
    m_qMutex.lock();
    m_iNewPreStimSamples = samples;
    m_iPreStimSeconds = secs;
    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setPostStim(qint32 samples, qint32 secs)
{
    m_qMutex.lock();
    m_iNewPostStimSamples = samples;
    m_iPostStimSeconds = secs;
    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setTriggerChIndx(qint32 idx)
{
    m_qMutex.lock();
    m_iNewTriggerIndex = idx;
    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setArtifactReduction(bool bActivate, double dThreshold)
{
    m_qMutex.lock();
    m_dArtifactThreshold = dThreshold;
    m_bDoArtifactReduction = bActivate;
    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setBaselineActive(bool activate)
{
    m_qMutex.lock();

    m_bDoBaselineCorrection = activate;

    if(!m_bDoBaselineCorrection)
        m_pStimEvoked->baseline = qMakePair(QVariant("None"), QVariant("None"));

    m_pStimEvoked->baseline = m_pairBaselineSec;

    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setBaselineFrom(int fromSamp, int fromMSec)
{
    m_qMutex.lock();

    m_pairBaselineSec.first = QVariant(QString::number(float(fromMSec)/1000));
    m_pairBaselineSamp.first = QVariant(QString::number(fromSamp));

    m_pStimEvoked->baseline.first = QVariant(QString::number(float(fromMSec)/1000));

    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setBaselineTo(int toSamp, int toMSec)
{
    m_qMutex.lock();

    m_pairBaselineSec.second = QVariant(QString::number(float(toMSec)/1000));
    m_pairBaselineSamp.second = QVariant(QString::number(toSamp));

    m_pStimEvoked->baseline.second = QVariant(QString::number(float(toMSec)/1000));

    m_qMutex.unlock();
}


//*************************************************************************************************************

bool RtAve::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_qMutex.lock();
    m_bIsRunning = true;
    m_qMutex.unlock();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RtAve::stop()
{
    m_qMutex.lock();
    m_bIsRunning = false;
    m_qMutex.unlock();

    m_pRawMatrixBuffer->releaseFromPop();

    m_pRawMatrixBuffer->clear();

    return true;
}


//*************************************************************************************************************

void RtAve::run()
{
    //Do initial reset
    reset();

    //Enter the main loop
    while(m_bIsRunning) {
        bool doProcessing = false;

        m_qMutex.lock();
        if(m_pRawMatrixBuffer)
            doProcessing = true;
        m_qMutex.unlock();

        if(doProcessing) {
            if(m_iNewPreStimSamples != m_iPreStimSamples
                    || m_iNewPostStimSamples != m_iPostStimSamples
                    || m_iNewTriggerIndex != m_iTriggerIndex
                    || m_iNewAverageMode != m_iAverageMode
                    || m_iNewNumAverages != m_iNumAverages)
                reset();

            //Acquire Data
            MatrixXd rawSegment = m_pRawMatrixBuffer->pop();

            //Fill back buffer and decide when to do the data packing of the different buffers
            if(m_bFillingBackBuffer) {
                if(m_iMatDataPostIdx == m_iPostStimSamples) {
                    m_iMatDataPostIdx = 0;

                    //Merge the different buffers
                    mergeData();

                    //Calculate the actual average
                    generateEvoked();

                    //If number of averages was reached emit new average
                    if(m_qListStimAve.size() > 0)
                        emit evokedStim(m_pStimEvoked);

                    m_bFillingBackBuffer = false;

                    qDebug()<<"RtAve::run() - Number of calculated averages:"<<m_iNumberCalcAverages;
                    qDebug()<<"RtAve::run() - m_qListStimAve.size():"<<m_qListStimAve.size();
                } else {
                    fillBackBuffer(rawSegment);
                }
            } else {
                clearDetectedTriggers();

                //Detect trigger
                m_iTriggerPos = -1;
                QList<QPair<int,double> > lDetectedTriggers = DetectTrigger::detectTriggerFlanksGrad(rawSegment, m_iTriggerIndex, 0, m_fTriggerThreshold, true, "Rising");

                if(!lDetectedTriggers.isEmpty())
                {
                    m_iTriggerPos = lDetectedTriggers.at(0).first;
                }

                //If number of averages is equals zero do not perform averages
                if(m_iNumAverages == 0) {
                    m_iTriggerPos = rawSegment.cols()-1;
                }

                //If detected turn on filling of the back / post stim buffer
                if(m_iTriggerPos != -1) {
                    //Do front buffer stuff
                    MatrixXd tempMat;

                    if(m_iTriggerPos >= m_iPreStimSamples) {
                        tempMat = rawSegment.block(0,m_iTriggerPos-m_iPreStimSamples,rawSegment.rows(),m_iPreStimSamples);
                        fillFrontBuffer(tempMat);
                    } else {
                        tempMat = rawSegment.block(0,0,rawSegment.rows(),m_iTriggerPos);
                        fillFrontBuffer(tempMat);
                    }

                    //Do back buffer stuff
                    if(rawSegment.cols()-m_iTriggerPos >= m_matDataPost.cols()) {
                        m_matDataPost = rawSegment.block(0,m_iTriggerPos,m_matDataPost.rows(),m_matDataPost.cols());
                        m_iMatDataPostIdx = m_iPostStimSamples;
                    } else {
                        m_matDataPost.block(0,0,m_matDataPost.rows(),rawSegment.cols()-m_iTriggerPos) = rawSegment.block(0,m_iTriggerPos,rawSegment.rows(),rawSegment.cols()-m_iTriggerPos);
                        m_iMatDataPostIdx = rawSegment.cols()-m_iTriggerPos;
                    }

                    m_bFillingBackBuffer = true;
                } else {
                    //Fill front / pre stim buffer
                    fillFrontBuffer(rawSegment);
                }

                //qDebug()<<"Trigger channel "<<m_iTriggerIndex<<" found at "<<m_iTriggerPos;
            }
        }
    }
}


//*************************************************************************************************************

void RtAve::clearDetectedTriggers()
{
    QMutableMapIterator<int,QList<int> > i(m_qMapDetectedTrigger);
    while (i.hasNext()) {
        i.next();
        i.value().clear();
    }
}


//*************************************************************************************************************

void RtAve::fillBackBuffer(MatrixXd &data)
{
    int iResidualCols = data.cols();
    if(m_iMatDataPostIdx+data.cols() > m_iPostStimSamples) {
        iResidualCols = m_iPostStimSamples-m_iMatDataPostIdx;
        m_matDataPost.block(0,m_iMatDataPostIdx,m_matDataPost.rows(),iResidualCols) = data.block(0,0,data.rows(),iResidualCols);
    } else
        m_matDataPost.block(0,m_iMatDataPostIdx,m_matDataPost.rows(),iResidualCols) = data;

    m_iMatDataPostIdx += iResidualCols;
}


//*************************************************************************************************************

void RtAve::fillFrontBuffer(MatrixXd &data)
{
    if(m_matDataPre.cols() <= data.cols()) {
        m_matDataPre = data.block(0,data.cols()-m_iPreStimSamples,data.rows(),m_iPreStimSamples);
    } else {
        int residual = m_matDataPre.cols()-data.cols();

        //Copy shift data
        m_matDataPre.block(0,0,m_matDataPre.rows(),residual) = m_matDataPre.block(0,m_matDataPre.cols()-residual,m_matDataPre.rows(),residual);

        //Copy new data in
        m_matDataPre.block(0,residual,m_matDataPre.rows(),data.cols()) = data;
    }
}


//*************************************************************************************************************

void RtAve::mergeData()
{
    MatrixXd mergedData(m_matDataPre.rows(), m_matDataPre.cols()+m_matDataPost.cols());

    mergedData << m_matDataPre, m_matDataPost;

    //Perform artifact threshold
    bool bArtifactedDetected = false;

    if(m_bDoArtifactReduction) {
       bArtifactedDetected = checkForArtifact(mergedData, m_dArtifactThreshold);
    }

    if(bArtifactedDetected == false) {
        //Add cut data to average buffer
        m_qListStimAve.append(mergedData);

        //Pop data from buffer
        if(m_qListStimAve.size() > m_iNumAverages && m_iNumAverages >= 1) {
            m_qListStimAve.pop_front();
        }

        //Proceed a bit different if we use zero number of averages
        if(m_qListStimAve.size() > 1 && m_iNumAverages == 0) {
            m_qListStimAve.pop_front();
        }
    }
}


//*************************************************************************************************************

bool RtAve::checkForArtifact(MatrixXd& data, double dThreshold)
{
    double min = 0;
    double max = 0;
//    double sub = 0;

//    VectorXd subs;

//    for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
//        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH)
//           && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)) {
//            min = data.row(i).minCoeff();
//            max = data.row(i).maxCoeff();
//            sub = max - min;

//            subs.resize(subs.rows() + 1);
//            subs(i) = abs(sub);
//        }
//    }

    //This is the channel index to scan for artifacts
    int i = 5;

    min = data.row(i).minCoeff();
    max = data.row(i).maxCoeff();
    double maxSubs = max - min;

    //qDebug()<<"RtAve::checkForArtifact - maxSubs"<<maxSubs;

    bool bArtifactDetected = false;

    if(maxSubs > dThreshold) {
        std::cout << '\a';
        bArtifactDetected = true;
    }

    return bArtifactDetected;
}


//*************************************************************************************************************

void RtAve::generateEvoked()
{
    if(m_qListStimAve.isEmpty())
        return;

    // Generate final evoked
    MatrixXd finalAverage = MatrixXd::Zero(m_qListStimAve.first().rows(), m_iPreStimSamples+m_iPostStimSamples);

    if(m_iAverageMode == 0) {
        for(int i = 0; i<m_qListStimAve.size(); i++) {
            finalAverage += m_qListStimAve.at(i);
        }
        finalAverage = finalAverage/m_qListStimAve.size();

        if(m_bDoBaselineCorrection)
            finalAverage = MNEMath::rescale(finalAverage, m_pStimEvoked->times, m_pairBaselineSec, QString("mean"));

        m_pStimEvoked->data = finalAverage;
        m_pStimEvoked->nave = m_iNumAverages;

        if(m_iNumberCalcAverages<m_iNumAverages)
            m_iNumberCalcAverages++;
    } else if(m_iAverageMode == 1) {
        MatrixXd tempMatrix = m_qListStimAve.last();

        if(m_bDoBaselineCorrection)
            tempMatrix = MNEMath::rescale(tempMatrix, m_pStimEvoked->times, m_pairBaselineSec, QString("mean"));

        *m_pStimEvoked.data() += tempMatrix;

        m_iNumberCalcAverages++;
    }
}


//*************************************************************************************************************

void RtAve::reset()
{
    //Reset
    m_qMutex.lock();

    float T = 1.0/m_pFiffInfo->sfreq;

    m_iPreStimSamples = m_iNewPreStimSamples;
    m_iPostStimSamples = m_iNewPostStimSamples;
    m_iTriggerIndex = m_iNewTriggerIndex;
    m_iAverageMode = m_iNewAverageMode;
    m_iNumAverages = m_iNewNumAverages;

    m_iNumberCalcAverages = 0;

    //Resize data matrices
    m_matDataPre.resize(m_pFiffInfo->chs.size(), m_iPreStimSamples);
    m_matDataPre.setZero();
    m_matDataPost.resize(m_pFiffInfo->chs.size(), m_iPostStimSamples);
    m_matDataPost.setZero();

    //Full real-time evoked response
    m_pStimEvoked->setInfo(*m_pFiffInfo.data());
    m_pStimEvoked->baseline = m_pairBaselineSec;
    m_pStimEvoked->times.resize(m_iPreStimSamples+m_iPostStimSamples);
    m_pStimEvoked->times[0] = -T*m_iPreStimSamples;
    for(int i = 1; i < m_pStimEvoked->times.size(); ++i)
        m_pStimEvoked->times[i] = m_pStimEvoked->times[i-1] + T;
    m_pStimEvoked->first = m_pStimEvoked->times[0];
    m_pStimEvoked->last = m_pStimEvoked->times[m_pStimEvoked->times.size()-1];
    m_pStimEvoked->data.setZero();

    if(m_iAverageMode == 0)
        m_pStimEvoked->nave = m_iNumAverages;
    else
        m_pStimEvoked->nave = 0;

    m_qListStimAve.clear();
    clearDetectedTriggers();

    m_bFillingBackBuffer = false;

    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::init()
{
    m_qMutex.lock();

    QList<int> tempList;
    for(int i = 0; i < m_pFiffInfo->nchan; ++i) {
        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH && (m_pFiffInfo->chs[i].ch_name != QString("STI 014")))
            m_qMapDetectedTrigger.insert(i,tempList);
    }

    m_iNewPreStimSamples = m_iPreStimSamples;
    m_iNewPostStimSamples = m_iPostStimSamples;
    m_iNewNumAverages = m_iNumAverages;

    //Resize data matrices
    m_matDataPre.resize(m_pFiffInfo->chs.size(), m_iPreStimSamples);
    m_matDataPre.setZero();
    m_matDataPost.resize(m_pFiffInfo->chs.size(), m_iPostStimSamples);
    m_matDataPost.setZero();

    m_qMutex.unlock();
}

