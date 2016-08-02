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
, m_iTriggerChIndex(-1)
, m_iNewTriggerIndex(p_iTriggerIndex)
, m_iAverageMode(0)
, m_iNewAverageMode(0)
, m_bDoBaselineCorrection(false)
, m_pairBaselineSec(qMakePair(QVariant(QString::number(p_iBaselineFromSecs)),QVariant(QString::number(p_iBaselineToSecs))))
, m_pStimEvokedSet(FiffEvokedSet::SPtr(new FiffEvokedSet))
, m_iNumberCalcAverages(0)
, m_iCurrentBlockSize(0)
, m_dArtifactThreshold(300e-6)
, m_bDoArtifactReduction(false)
{
    qRegisterMetaType<FIFFLIB::FiffEvokedSet::SPtr>("FIFFLIB::FiffEvokedSet::SPtr");

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

    if(!m_bDoBaselineCorrection) {
        for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
            m_pStimEvokedSet->evoked[i].baseline = qMakePair(QVariant("None"), QVariant("None"));
        }
    }

    for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
        m_pStimEvokedSet->evoked[i].baseline = m_pairBaselineSec;
    }

    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setBaselineFrom(int fromSamp, int fromMSec)
{
    m_qMutex.lock();

    m_pairBaselineSec.first = QVariant(QString::number(float(fromMSec)/1000));
    m_pairBaselineSamp.first = QVariant(QString::number(fromSamp));

    for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
        m_pStimEvokedSet->evoked[i].baseline.first = QVariant(QString::number(float(fromMSec)/1000));
    }

    m_qMutex.unlock();
}


//*************************************************************************************************************

void RtAve::setBaselineTo(int toSamp, int toMSec)
{
    m_qMutex.lock();

    m_pairBaselineSec.second = QVariant(QString::number(float(toMSec)/1000));
    m_pairBaselineSamp.second = QVariant(QString::number(toSamp));

    for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
        m_pStimEvokedSet->evoked[i].baseline.second = QVariant(QString::number(float(toMSec)/1000));
    }

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
        if(m_pRawMatrixBuffer) {
            doProcessing = true;
        }
        m_qMutex.unlock();

        if(doProcessing) {
            if(m_iNewPreStimSamples != m_iPreStimSamples
                    || m_iNewPostStimSamples != m_iPostStimSamples
                    || m_iNewTriggerIndex != m_iTriggerChIndex
                    || m_iNewAverageMode != m_iAverageMode
                    || m_iNewNumAverages != m_iNumAverages) {
                reset();
            }

            //Acquire Data
            MatrixXd rawSegment = m_pRawMatrixBuffer->pop();

            doAveraging(rawSegment);
        }
    }
}


//*************************************************************************************************************

void RtAve::doAveraging(const MatrixXd& rawSegment)
{
    QMutexLocker locker(&m_qMutex);

    //qDebug()<<"";
    //qDebug()<<"";
    //qDebug()<<"";

    //Detect trigger
    QList<QPair<int,double> > lDetectedTriggers = DetectTrigger::detectTriggerFlanksGrad(rawSegment, m_iTriggerChIndex, 0, m_fTriggerThreshold, true, "Rising");
    for(int i = 0; i < lDetectedTriggers.size(); ++i) {
        if(!m_mapFillingBackBuffer.contains(lDetectedTriggers.at(i).second)) {
            double dTriggerType = lDetectedTriggers.at(i).second;

            //qDebug()<<"Adding dTriggerType"<<dTriggerType;

            m_mapFillingBackBuffer[dTriggerType] = false;
            m_mapMatDataPostIdx[dTriggerType] = 0;
            m_mapDataPost[dTriggerType].resize(m_pFiffInfo->chs.size(), m_iPostStimSamples);
            m_mapDataPost[dTriggerType].setZero();
        }
    }

    //Do averaging for each trigger type
    QMutableMapIterator<double,bool> idx(m_mapFillingBackBuffer);
    while(idx.hasNext()) {
        idx.next();

        double dTriggerType = idx.key();

        //qDebug()<<"1 dTriggerType"<<dTriggerType;

        //Fill back buffer and decide when to do the data packing of the different buffers
        if(m_mapFillingBackBuffer[dTriggerType]) {
            //qDebug()<<"2";
            if(m_mapMatDataPostIdx[dTriggerType] == m_iPostStimSamples) {
                //qDebug()<<"3";
                m_mapFillingBackBuffer[dTriggerType] = 0;

                //Merge the different buffers
                mergeData(dTriggerType);

                //Calculate the final average/evoked data
                generateEvoked(dTriggerType);

                //If number of averages was reached emit new average
                if(m_mapStimAve[dTriggerType].size() > 0) {
                    emit evokedStim(m_pStimEvokedSet);
                }

                m_mapFillingBackBuffer[dTriggerType] = false;

                //qDebug()<<"RtAve::run() - Number of calculated averages:" << m_iNumberCalcAverages;
                //qDebug()<<"RtAve::run() - dTriggerType:" << dTriggerType;
                //qDebug()<<"RtAve::run() - m_mapStimAve[dTriggerType].size():" << m_mapStimAve[dTriggerType].size();
            } else {
                //qDebug()<<"4";
                fillBackBuffer(rawSegment, dTriggerType);
            }
        } else {
            //qDebug()<<"5";

            if(lDetectedTriggers.isEmpty()) {
                //qDebug()<<"6";
                //Fill front / pre stim buffer
                fillFrontBuffer(rawSegment, -1.0);
            } else {
                //qDebug()<<"7";
                for(int i = 0; i < lDetectedTriggers.size(); ++i) {
                    if(dTriggerType == lDetectedTriggers.at(i).second) {
                        //qDebug()<<"8";
                        int iTriggerPos = lDetectedTriggers.at(i).first;

                        //If number of averages is equals zero do not perform averages
                        if(m_iNumAverages == 0) {
                            iTriggerPos = rawSegment.cols()-1;
                        }

                        //Do front buffer stuff
                        MatrixXd tempMat;

                        if(iTriggerPos >= m_iPreStimSamples) {
                            tempMat = rawSegment.block(0,iTriggerPos - m_iPreStimSamples,rawSegment.rows(),m_iPreStimSamples);
                            fillFrontBuffer(tempMat, dTriggerType);
                        } else {
                            tempMat = rawSegment.block(0,0,rawSegment.rows(),iTriggerPos);
                            fillFrontBuffer(tempMat, dTriggerType);
                        }

                        //Do back buffer stuff
                        if(rawSegment.cols() - iTriggerPos >= m_mapDataPost[dTriggerType].cols()) {
                            m_mapDataPost[dTriggerType] = rawSegment.block(0,iTriggerPos,m_mapDataPost[dTriggerType].rows(),m_mapDataPost[dTriggerType].cols());
                            m_mapMatDataPostIdx[dTriggerType] = m_iPostStimSamples;
                        } else {
                            m_mapDataPost[dTriggerType].block(0,0,m_mapDataPost[dTriggerType].rows(),rawSegment.cols() - iTriggerPos) = rawSegment.block(0,iTriggerPos,rawSegment.rows(),rawSegment.cols() - iTriggerPos);
                            m_mapMatDataPostIdx[dTriggerType] = rawSegment.cols() - iTriggerPos;
                        }

                        m_mapFillingBackBuffer[dTriggerType] = true;

                        //qDebug()<<"Trigger type "<<dTriggerType<<" found at "<<iTriggerPos;
                    }
                }
            }
        }
    }
}


//*************************************************************************************************************

void RtAve::fillBackBuffer(const MatrixXd &data, double dTriggerType)
{
    int iResidualCols = data.cols();
    if(m_mapMatDataPostIdx[dTriggerType] + data.cols() > m_iPostStimSamples) {
        iResidualCols = m_iPostStimSamples - m_mapMatDataPostIdx[dTriggerType];
        m_mapDataPost[dTriggerType].block(0,m_mapMatDataPostIdx[dTriggerType],m_mapDataPost[dTriggerType].rows(),iResidualCols) = data.block(0,0,data.rows(),iResidualCols);
    } else
        m_mapDataPost[dTriggerType].block(0,m_mapMatDataPostIdx[dTriggerType],m_mapDataPost[dTriggerType].rows(),iResidualCols) = data;

    m_mapMatDataPostIdx[dTriggerType] += iResidualCols;
}


//*************************************************************************************************************

void RtAve::fillFrontBuffer(const MatrixXd &data, double dTriggerType)
{
    //Init m_mapDataPre
    if(!m_mapDataPre.contains(dTriggerType)) {
        if(dTriggerType != -1.0) {
            m_mapDataPre[dTriggerType] = m_mapDataPre[-1.0];
        } else {
            m_mapDataPre[-1.0].resize(m_pFiffInfo->chs.size(), m_iPreStimSamples);
            m_mapDataPre[-1.0].setZero();
        }
    }

    if(m_mapDataPre[dTriggerType].cols() <= data.cols()) {
        m_mapDataPre[dTriggerType] = data.block(0,data.cols() - m_iPreStimSamples,data.rows(),m_iPreStimSamples);
    } else {
        int residual = m_mapDataPre[dTriggerType].cols() - data.cols();

        //Copy shift data
        m_mapDataPre[dTriggerType].block(0,0,m_mapDataPre[dTriggerType].rows(),residual) = m_mapDataPre[dTriggerType].block(0,m_mapDataPre[dTriggerType].cols() - residual,m_mapDataPre[dTriggerType].rows(),residual);

        //Copy new data in
        m_mapDataPre[dTriggerType].block(0,residual,m_mapDataPre[dTriggerType].rows(),data.cols()) = data;
    }
}


//*************************************************************************************************************

void RtAve::mergeData(double dTriggerType)
{
    MatrixXd mergedData(m_mapDataPre[dTriggerType].rows(), m_mapDataPre[dTriggerType].cols() + m_mapDataPost[dTriggerType].cols());

    mergedData << m_mapDataPre[dTriggerType], m_mapDataPost[dTriggerType];

    //Perform artifact threshold
    bool bArtifactedDetected = false;

    if(m_bDoArtifactReduction) {
       bArtifactedDetected = checkForArtifact(mergedData, m_dArtifactThreshold);
    }

    if(bArtifactedDetected == false) {
        //Add cut data to average buffer
        m_mapStimAve[dTriggerType].append(mergedData);

        //Pop data from buffer
        if(m_mapStimAve[dTriggerType].size() > m_iNumAverages && m_iNumAverages >= 1) {
            m_mapStimAve[dTriggerType].pop_front();
        }

        //Proceed a bit different if we use zero number of averages
        if(m_mapStimAve[dTriggerType].size() > 1 && m_iNumAverages == 0) {
            m_mapStimAve[dTriggerType].pop_front();
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

void RtAve::generateEvoked(double dTriggerType)
{
    if(m_mapStimAve[dTriggerType].isEmpty())
        return;

    //Init evoked
    FiffEvoked evoked;

    for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
        if(m_pStimEvokedSet->evoked.at(i).comment == QString::number(dTriggerType)) {
            evoked = m_pStimEvokedSet->evoked.at(i);
            break;
        }
    }

    // Generate final evoked
    MatrixXd finalAverage = MatrixXd::Zero(m_mapStimAve[dTriggerType].first().rows(), m_iPreStimSamples+m_iPostStimSamples);

    if(m_iAverageMode == 0) {
        for(int i = 0; i < m_mapStimAve[dTriggerType].size(); ++i) {
            finalAverage += m_mapStimAve[dTriggerType].at(i);
        }
        finalAverage = finalAverage/m_mapStimAve[dTriggerType].size();

        if(m_bDoBaselineCorrection) {
            finalAverage = MNEMath::rescale(finalAverage, evoked.times, m_pairBaselineSec, QString("mean"));
        }

        evoked.data = finalAverage;
        evoked.nave = m_iNumAverages;

        if(m_iNumberCalcAverages < m_iNumAverages) {
            m_iNumberCalcAverages++;
        }
    } else if(m_iAverageMode == 1) {
        MatrixXd tempMatrix = m_mapStimAve[dTriggerType].last();

        if(m_bDoBaselineCorrection) {
            tempMatrix = MNEMath::rescale(tempMatrix, evoked.times, m_pairBaselineSec, QString("mean"));
        }

        evoked += tempMatrix;

        m_iNumberCalcAverages++;
    }

    //Add evoked to evoked set
    bool bEvokedFound = false;

    for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
        if(m_pStimEvokedSet->evoked.at(i).comment == QString::number(dTriggerType)) {
            m_pStimEvokedSet->evoked[i] = evoked;

            bEvokedFound = true;
            break;
        }
    }

    //If the evoekd is not yet present add it here
    if(!bEvokedFound) {
        float T = 1.0/m_pFiffInfo->sfreq;

        evoked.setInfo(*m_pFiffInfo.data());
        evoked.baseline = m_pairBaselineSec;
        evoked.times.resize(m_iPreStimSamples + m_iPostStimSamples);
        evoked.times[0] = -T*m_iPreStimSamples;
        for(int i = 1; i < evoked.times.size(); ++i)
            evoked.times[i] = evoked.times[i-1] + T;
        evoked.first = evoked.times[0];
        evoked.last = evoked.times[evoked.times.size()-1];
        evoked.data.setZero();
        evoked.comment = QString::number(dTriggerType);
        if(m_iAverageMode == 0)
            evoked.nave = m_iNumAverages;
        else
            evoked.nave = 0;

        m_pStimEvokedSet->evoked.append(evoked);
    }
}


//*************************************************************************************************************

void RtAve::reset()
{
    QMutexLocker locker(&m_qMutex);

    //Reset
    m_iPreStimSamples = m_iNewPreStimSamples;
    m_iPostStimSamples = m_iNewPostStimSamples;
    m_iTriggerChIndex = m_iNewTriggerIndex;
    m_iAverageMode = m_iNewAverageMode;
    m_iNumAverages = m_iNewNumAverages;

//    m_iNumberCalcAverages = 0;

//    //Resize data matrices
//    m_mapDataPre.clear();
//    m_mapDataPost.clear();
//    m_mapFillingBackBuffer.clear();
//    m_mapStimAve.clear();

//    QMutableMapIterator<double,Eigen::MatrixXd> i0(m_mapDataPre);
//    while (i0.hasNext()) {
//        i0.next();

//        i0.value().resize(m_pFiffInfo->chs.size(), m_iPreStimSamples);
//        i0.value().setZero();
//    }

//    QMutableMapIterator<double,Eigen::MatrixXd> i1(m_mapDataPost);
//    while (i1.hasNext()) {
//        i1.next();

//        i1.value().resize(m_pFiffInfo->chs.size(), m_iPreStimSamples);
//        i1.value().setZero();
//    }

//    //Reset data matrix buffer
//    QMutableMapIterator<double,QList<Eigen::MatrixXd> > i2(m_mapStimAve);
//    while (i2.hasNext()) {
//        i2.next();

//        i2.value().clear();
//    }

//    //Reset fill back buffer flag map
//    QMutableMapIterator<double,bool> i3(m_mapFillingBackBuffer);
//    while (i3.hasNext()) {
//        i3.next();

//        i3.value() = false;
//    }
}


//*************************************************************************************************************

void RtAve::init()
{
    QMutexLocker locker(&m_qMutex);

    m_iNewPreStimSamples = m_iPreStimSamples;
    m_iNewPostStimSamples = m_iPostStimSamples;
    m_iNewNumAverages = m_iNumAverages;
}

