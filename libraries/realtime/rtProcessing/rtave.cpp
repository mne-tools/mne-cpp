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
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace REALTIMELIB;
using namespace FIFFLIB;
using namespace IOBUFFER;
using namespace UTILSLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOABAL MEMBER METHODS
//=============================================================================================================

int             m_dValueVariance = 0.5;           /**< Variance value to detect artifacts */
double          m_dValueThreshold = 300e-6;          /**< Threshold to detect artifacts */


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
, m_bActivateThreshold(false)
, m_bActivateVariance(false)
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
    // ToDo handle change buffersize
    if(!m_pRawMatrixBuffer) {
        QMutexLocker locker(&m_qMutex);
        m_pRawMatrixBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(30, p_DataSegment.rows(), p_DataSegment.cols()));
    }

    m_pRawMatrixBuffer->push(&p_DataSegment);
}


//*************************************************************************************************************

void RtAve::setAverages(qint32 numAve)
{
    QMutexLocker locker(&m_qMutex);
    m_iNewNumAverages = numAve;
}


//*************************************************************************************************************

void RtAve::setAverageMode(qint32 mode)
{
    QMutexLocker locker(&m_qMutex);
    m_iNewAverageMode = mode;
}


//*************************************************************************************************************

void RtAve::setPreStim(qint32 samples, qint32 secs)
{
    Q_UNUSED(secs);

    QMutexLocker locker(&m_qMutex);
    m_iNewPreStimSamples = samples;
}


//*************************************************************************************************************

void RtAve::setPostStim(qint32 samples, qint32 secs)
{
    Q_UNUSED(samples);

    QMutexLocker locker(&m_qMutex);
    m_iNewPostStimSamples = samples;
}


//*************************************************************************************************************

void RtAve::setTriggerChIndx(qint32 idx)
{
    QMutexLocker locker(&m_qMutex);
    m_iNewTriggerIndex = idx;
}


//*************************************************************************************************************

void RtAve::setArtifactReduction(bool bActivateThreshold, double dValueThreshold, bool bActivateVariance, double dValueVariance)
{
    QMutexLocker locker(&m_qMutex);
    m_dValueVariance = dValueVariance;
    m_dValueThreshold = dValueThreshold;

    m_bActivateThreshold = bActivateThreshold;
    m_bActivateVariance = bActivateVariance;
}


//*************************************************************************************************************

void RtAve::setBaselineActive(bool activate)
{
    QMutexLocker locker(&m_qMutex);

    m_bDoBaselineCorrection = activate;

    if(!m_bDoBaselineCorrection) {
        for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
            m_pStimEvokedSet->evoked[i].baseline = qMakePair(QVariant("None"), QVariant("None"));
        }
    }

    for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
        m_pStimEvokedSet->evoked[i].baseline = m_pairBaselineSec;
    }
}


//*************************************************************************************************************

void RtAve::setBaselineFrom(int fromSamp, int fromMSec)
{
    QMutexLocker locker(&m_qMutex);

    m_pairBaselineSec.first = QVariant(QString::number(float(fromMSec)/1000));
    m_pairBaselineSamp.first = QVariant(QString::number(fromSamp));

    for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
        m_pStimEvokedSet->evoked[i].baseline.first = QVariant(QString::number(float(fromMSec)/1000));
    }
}


//*************************************************************************************************************

void RtAve::setBaselineTo(int toSamp, int toMSec)
{
    QMutexLocker locker(&m_qMutex);

    m_pairBaselineSec.second = QVariant(QString::number(float(toMSec)/1000));
    m_pairBaselineSamp.second = QVariant(QString::number(toSamp));

    for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
        m_pStimEvokedSet->evoked[i].baseline.second = QVariant(QString::number(float(toMSec)/1000));
    }
}


//*************************************************************************************************************

bool RtAve::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_bIsRunning = true;

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RtAve::stop()
{
    QMutexLocker locker(&m_qMutex);

    m_bIsRunning = false;

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
        //Wait for first data block to arrive
        if(isDataBufferInit()) {
            if(controlValuesChanged()) {
                reset();
            }

            //QElapsedTimer time;
            //time.start();

            //Acquire Data m_pRawMatrixBuffer is thread safe
            MatrixXd rawSegment = m_pRawMatrixBuffer->pop();

            //QMutexLocker locker(&m_qMutex);
            doAveraging(rawSegment);

            //qDebug()<<"RtAve::run() - time.elapsed()"<<time.elapsed();
        }
    }
}


//*************************************************************************************************************

void RtAve::doAveraging(const MatrixXd& rawSegment)
{
    //qDebug()<<"";
    //qDebug()<<"";
    //qDebug()<<"";

    //Detect trigger

    QElapsedTimer time;
    time.start();

    QList<QPair<int,double> > lDetectedTriggers = DetectTrigger::detectTriggerFlanksMax(rawSegment, m_iTriggerChIndex, 0, m_fTriggerThreshold, true);

    //qDebug()<<"RtAve::doAveraging() - time for detection"<<time.elapsed();
    //time.start();

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

    //Fill front / pre stim buffer even if no triggers have been located at all yet
    if(m_mapFillingBackBuffer.isEmpty()) {
        fillFrontBuffer(rawSegment, -1.0);
    }

    //Do averaging for each trigger type
    QMutableMapIterator<double,bool> idx(m_mapFillingBackBuffer);
    while(idx.hasNext()) {
        idx.next();

        double dTriggerType = idx.key();

        //qDebug()<<"1 dTriggerType"<<dTriggerType;

        if(lDetectedTriggers.isEmpty()) {
            //qDebug()<<"6";
            //Fill front / pre stim buffer
            fillFrontBuffer(rawSegment, -1.0);
        }

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

                //qDebug()<<"RtAve::run() - Number of calculated averages:" << m_iNumberCalcAverages[dTriggerType];
                //qDebug()<<"RtAve::run() - dTriggerType:" << dTriggerType;
                //qDebug()<<"RtAve::run() - m_mapStimAve[dTriggerType].size():" << m_mapStimAve[dTriggerType].size();
            } else {
                //qDebug()<<"4";
                fillBackBuffer(rawSegment, dTriggerType);
            }
        } else {
            if(lDetectedTriggers.isEmpty()) {
                //qDebug()<<"6";
                //Fill front / pre stim buffer
                fillFrontBuffer(rawSegment, dTriggerType);
            } else {
                for(int i = 0; i < lDetectedTriggers.size(); ++i) {
                    if(dTriggerType == lDetectedTriggers.at(i).second) {
                        //qDebug()<<"8";
                        int iTriggerPos = lDetectedTriggers.at(i).first;

                        //If number of averages is equals zero do not perform averages
                        if(m_iNumAverages == 0) {
                            iTriggerPos = rawSegment.cols()-1;
                        }

                        //qDebug()<<"8.1";

                        //Do front buffer stuff
                        MatrixXd tempMat;

                        if(iTriggerPos >= m_iPreStimSamples) {
                            tempMat = rawSegment.block(0,iTriggerPos - m_iPreStimSamples,rawSegment.rows(),m_iPreStimSamples);
                            fillFrontBuffer(tempMat, dTriggerType);
                        } else {
                            tempMat = rawSegment.block(0,0,rawSegment.rows(),iTriggerPos);
                            fillFrontBuffer(tempMat, dTriggerType);
                        }

                        //qDebug()<<"8.2";

                        //Do back buffer stuff
                        if(rawSegment.cols() - iTriggerPos >= m_mapDataPost[dTriggerType].cols()) {
                            m_mapDataPost[dTriggerType] = rawSegment.block(0,iTriggerPos,m_mapDataPost[dTriggerType].rows(),m_mapDataPost[dTriggerType].cols());
                            m_mapMatDataPostIdx[dTriggerType] = m_iPostStimSamples;
                        } else {
                            m_mapDataPost[dTriggerType].block(0,0,m_mapDataPost[dTriggerType].rows(),rawSegment.cols() - iTriggerPos) = rawSegment.block(0,iTriggerPos,rawSegment.rows(),rawSegment.cols() - iTriggerPos);
                            m_mapMatDataPostIdx[dTriggerType] = rawSegment.cols() - iTriggerPos;
                        }

                        //qDebug()<<"8.3";

                        m_mapFillingBackBuffer[dTriggerType] = true;

                        //qDebug()<<"Trigger type "<<dTriggerType<<" found at "<<iTriggerPos;
                    }
                }
            }
        }
    }

    qInfo()<<time.elapsed()<<"Averaging Processed";
}


//*************************************************************************************************************

void RtAve::fillBackBuffer(const MatrixXd &data, double dTriggerType)
{
    QMutexLocker locker(&m_qMutex);

    int iResidualCols = data.cols();
    if(m_mapMatDataPostIdx[dTriggerType] + data.cols() > m_iPostStimSamples) {
        iResidualCols = m_iPostStimSamples - m_mapMatDataPostIdx[dTriggerType];
        m_mapDataPost[dTriggerType].block(0,m_mapMatDataPostIdx[dTriggerType],m_mapDataPost[dTriggerType].rows(),iResidualCols) = data.block(0,0,data.rows(),iResidualCols);
    } else {
        m_mapDataPost[dTriggerType].block(0,m_mapMatDataPostIdx[dTriggerType],m_mapDataPost[dTriggerType].rows(),iResidualCols) = data;
    }

    m_mapMatDataPostIdx[dTriggerType] += iResidualCols;
}


//*************************************************************************************************************

void RtAve::fillFrontBuffer(const MatrixXd &data, double dTriggerType)
{
    QMutexLocker locker(&m_qMutex);

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
    QMutexLocker locker(&m_qMutex);

    MatrixXd mergedData(m_mapDataPre[dTriggerType].rows(), m_mapDataPre[dTriggerType].cols() + m_mapDataPost[dTriggerType].cols());

    mergedData << m_mapDataPre[dTriggerType], m_mapDataPost[dTriggerType];

    //Perform artifact threshold
    bool bArtifactedDetected = checkForArtifact(mergedData);

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

void checkChVariance(QPair<bool, RowVectorXd>& pairData)
{
    RowVectorXd temp = pairData.second;

    double dMedian = temp.norm() / temp.cols();

    temp = temp.array() - dMedian;
    temp.array().square();

//    qDebug() << "RtAve::checkForArtifact - dMedian" << abs(dMedian);
//    qDebug() << "RtAve::checkForArtifact - m_iValueVariance * dMedian" << m_iValueVariance * abs(dMedian);
//    qDebug() << "RtAve::checkForArtifact - compare value " << abs(pairData.second.norm() / pairData.second.cols());

    //If variance is 3 times bigger than median -> reject
    if(temp.norm() / temp.cols() > (m_dValueVariance * std::fabs(dMedian))) {
        pairData.first = true;
    } else {
        pairData.first = false;
    }
}



//*************************************************************************************************************

void checkChThreshold(QPair<bool, RowVectorXd>& pairData)
{
    RowVectorXd temp = pairData.second;

    temp = temp.array() - temp(0);

    double min = temp.minCoeff();
    double max = temp.maxCoeff();

//    qDebug() << "RtAve::checkForArtifact - min" << min;
//    qDebug() << "RtAve::checkForArtifact - max" << max;
//    qDebug() << "RtAve::checkForArtifact - m_dValueThreshold" << m_dValueThreshold;

    //If absolute vaue of min or max if bigger than threshold -> reject
    if((std::fabs(min) > m_dValueThreshold) || (std::fabs(max) > m_dValueThreshold)) {
        pairData.first = true;
    } else {
        pairData.first = false;
    }
}


//*************************************************************************************************************

bool RtAve::checkForArtifact(MatrixXd& data)
{
    bool bReject = false;

    if(m_bActivateThreshold || m_bActivateVariance) {
        //Prepare concurrent data handling
        QList<QPair<bool, RowVectorXd> > lchData;

        for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
            if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH)
                    && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name) && m_pFiffInfo->chs.at(i).chpos.coil_type != FIFFV_COIL_BABY_REF_MAG
                    && m_pFiffInfo->chs.at(i).chpos.coil_type != FIFFV_COIL_BABY_REF_MAG2) {
                QPair<bool, RowVectorXd> pair;
                pair.first = false;
                pair.second = data.row(i);
                lchData.append(pair);
            }
        }

        //qDebug() << "RtAve::checkForArtifact - lchData.size()" << lchData.size();

        if(m_bActivateVariance) {
            //Start the concurrent processing
            QFuture<void> future = QtConcurrent::map(lchData, checkChVariance);
            future.waitForFinished();

            for(int i = 0; i < lchData.size(); ++i) {
                if(lchData.at(i).first) {
                    bReject = true;
                    qDebug() << "RtAve::checkForArtifact - Reject trial";
                    break;
                }
            }
        }

        if(m_bActivateThreshold && !bReject) {
            //Start the concurrent processing
            QFuture<void> future = QtConcurrent::map(lchData, checkChThreshold);
            future.waitForFinished();

            for(int i = 0; i < lchData.size(); ++i) {
                if(lchData.at(i).first) {
                    bReject = true;
                    qDebug() << "RtAve::checkForArtifact - Reject trial";
                    break;
                }
            }
        }
    }

    if(bReject) {
        qDebug() << "RtAve::checkForArtifact - Reject trial";
    }

    return bReject;
}


//*************************************************************************************************************

void RtAve::generateEvoked(double dTriggerType)
{
    QMutexLocker locker(&m_qMutex);

    if(m_mapStimAve[dTriggerType].isEmpty()) {
        return;
    }

    //Init evoked
    FiffEvoked evoked;    
    int iEvokedIdx = -1;

    for(int i = 0; i < m_pStimEvokedSet->evoked.size(); ++i) {
        if(m_pStimEvokedSet->evoked.at(i).comment == QString::number(dTriggerType)) {
            evoked = m_pStimEvokedSet->evoked.at(i);
            iEvokedIdx = i;
            break;
        }
    }

    //If the evoked is not yet present add it here
    if(iEvokedIdx == -1) {
        float T = 1.0/m_pFiffInfo->sfreq;

        evoked.setInfo(*m_pFiffInfo.data());
        evoked.baseline = m_pairBaselineSec;
        evoked.times.resize(m_iPreStimSamples + m_iPostStimSamples);
        evoked.times[0] = -T*m_iPreStimSamples;
        for(int i = 1; i < evoked.times.size(); ++i)
            evoked.times[i] = evoked.times[i-1] + T;
        evoked.first = evoked.times[0];
        evoked.last = evoked.times[evoked.times.size()-1];
        evoked.comment = QString::number(dTriggerType);
    }

    // Generate final evoked
    MatrixXd finalAverage = MatrixXd::Zero(m_mapStimAve[dTriggerType].first().rows(), m_iPreStimSamples+m_iPostStimSamples);

    if(m_iAverageMode == 0) {
        for(int i = 0; i < m_mapStimAve[dTriggerType].size(); ++i) {
            finalAverage += m_mapStimAve[dTriggerType].at(i);
        }

        if(m_mapStimAve[dTriggerType].isEmpty()) {
            finalAverage = finalAverage/1;
        } else {
            finalAverage = finalAverage/m_mapStimAve[dTriggerType].size();
        }

        if(m_bDoBaselineCorrection) {
            finalAverage = MNEMath::rescale(finalAverage, evoked.times, m_pairBaselineSec, QString("mean"));
        }

        evoked.data = finalAverage;

        if(m_mapNumberCalcAverages[dTriggerType] < m_iNumAverages) {
            m_mapNumberCalcAverages[dTriggerType]++;
        }

        evoked.nave = m_mapNumberCalcAverages[dTriggerType];
    } else if(m_iAverageMode == 1) {
        MatrixXd tempMatrix = m_mapStimAve[dTriggerType].last();

        if(m_bDoBaselineCorrection) {
            tempMatrix = MNEMath::rescale(tempMatrix, evoked.times, m_pairBaselineSec, QString("mean"));
        }

        evoked += tempMatrix;

        m_mapNumberCalcAverages[dTriggerType]++;
    }

    //Add new data to evoked data set
    if(iEvokedIdx != -1) {
        //Evoked data is already present
        m_pStimEvokedSet->evoked[iEvokedIdx] = evoked;
    } else {
        //Evoked data is not present yet
        m_pStimEvokedSet->evoked.append(evoked);
    }
}


//*************************************************************************************************************

void RtAve::reset()
{
//    qDebug()<<"RtAve::reset()";
    QMutexLocker locker(&m_qMutex);

    qDebug()<<"RtAve::reset() - 1";

    //Reset
    m_iPreStimSamples = m_iNewPreStimSamples;
    m_iPostStimSamples = m_iNewPostStimSamples;
    m_iTriggerChIndex = m_iNewTriggerIndex;
    m_iAverageMode = m_iNewAverageMode;
    m_iNumAverages = m_iNewNumAverages;

    qDebug()<<"RtAve::reset() - 2";

    //Clear all evoked data information
    m_pStimEvokedSet->evoked.clear();

    qDebug()<<"RtAve::reset() - 3";

    //Clear all maps
//    m_mapDataPre.clear();
//    m_mapDataPost.clear();
//    m_mapFillingBackBuffer.clear();
//    m_mapStimAve.clear();
//    m_mapNumberCalcAverages.clear();

    m_qMapDetectedTrigger.clear();
    m_mapStimAve.clear();
    m_mapDataPre.clear();
    m_mapDataPost.clear();
    m_mapMatDataPostIdx.clear();
    m_mapFillingBackBuffer.clear();
    m_mapNumberCalcAverages.clear();

    qDebug()<<"RtAve::reset() - 4";

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

//    qDebug()<<"RtAve::reset() - END";
}


//*************************************************************************************************************

void RtAve::init()
{
    QMutexLocker locker(&m_qMutex);

    m_iNewPreStimSamples = m_iPreStimSamples;
    m_iNewPostStimSamples = m_iPostStimSamples;
    m_iNewNumAverages = m_iNumAverages;
}

