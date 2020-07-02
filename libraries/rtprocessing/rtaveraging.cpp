//=============================================================================================================
/**
 * @file     rtaveraging.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.3
 * @date     June, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief     Definition of the RtAveraging and RtAveragingWorker classes.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtaveraging.h"

#include <mne/mne_epoch_data_list.h>

#include <utils/ioutils.h>
#include <rtprocessing/detecttrigger.h>
#include <utils/mnemath.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS RtAveragingWorker
//=============================================================================================================

RtAveragingWorker::RtAveragingWorker(quint32 numAverages,
                                     quint32 iPreStimSamples,
                                     quint32 iPostStimSamples,
                                     quint32 iBaselineFromMSecs,
                                     quint32 iBaselineToMSecs,
                                     quint32 iTriggerIndex,
                                     FiffInfo::SPtr pFiffInfo)
: QObject()
, m_iNumAverages(numAverages)
, m_iPreStimSamples(iPreStimSamples)
, m_iPostStimSamples(iPostStimSamples)
, m_pFiffInfo(pFiffInfo)
, m_fTriggerThreshold(0.5f)
, m_iTriggerChIndex(-1)
, m_iNewTriggerIndex(iTriggerIndex)
, m_bDoBaselineCorrection(false)
, m_pairBaselineSec(qMakePair(float(iBaselineFromMSecs),float(iBaselineToMSecs)))
, m_bActivateThreshold(false)
{
    m_mapThresholds["eog"] = 300e-6;

    m_stimEvokedSet.info = *m_pFiffInfo.data();

    m_iNewPreStimSamples = m_iPreStimSamples;
    m_iNewPostStimSamples = m_iPostStimSamples;

    if(m_iNumAverages <= 0) {
        qDebug() << "RtAveragingWorker::RtAveragingWorker - Number of averages <= 0. Setting to 1 as default.";
        m_iNumAverages = 1;
    }
}

//=============================================================================================================

void RtAveragingWorker::doWork(const MatrixXd& rawSegment)
{
    if(this->thread()->isInterruptionRequested()) {
        return;
    }

    if(controlValuesChanged()) {
        reset();
    }

    doAveraging(rawSegment);
}

//=============================================================================================================

void RtAveragingWorker::setAverageNumber(qint32 numAve)
{
    if(numAve <= 0) {
        qDebug() << "[RtAveragingWorker::setAverageNumber] Number of averages <= 0 are not allowed. Returning.";
        return;
    }

    if(numAve < m_iNumAverages) {
        int iDiff = m_iNumAverages - numAve;

        //Do averaging for each trigger type
        QMutableMapIterator<double,QList<Eigen::MatrixXd> > idx(m_mapStimAve);

        while(idx.hasNext()) {
            idx.next();

            if(idx.value().size() > iDiff) {
                //Pop data from buffer
                for(int i = 0; i < iDiff; ++i) {
                    idx.value().pop_front();
                }
            }
        }
    }

    m_iNumAverages = numAve;
}

//=============================================================================================================

void RtAveragingWorker::setPreStim(qint32 samples, qint32 secs)
{
    Q_UNUSED(secs);

    m_iNewPreStimSamples = samples;
}

//=============================================================================================================

void RtAveragingWorker::setPostStim(qint32 samples, qint32 secs)
{
    Q_UNUSED(secs);

    m_iNewPostStimSamples = samples;
}

//=============================================================================================================

void RtAveragingWorker::setTriggerChIndx(qint32 idx)
{
    m_iNewTriggerIndex = idx;
}

//=============================================================================================================

void RtAveragingWorker::setArtifactReduction(const QMap<QString, double> &mapThresholds)
{
    if(mapThresholds["Active"] == 0.0) {
        m_bActivateThreshold = false;
    } else {
        m_bActivateThreshold = true;
    }

    m_mapThresholds = mapThresholds;
}

//=============================================================================================================

void RtAveragingWorker::setBaselineActive(bool activate)
{
    m_bDoBaselineCorrection = activate;

    if(!m_bDoBaselineCorrection) {
        for(int i = 0; i < m_stimEvokedSet.evoked.size(); ++i) {
            m_stimEvokedSet.evoked[i].baseline = qMakePair(-1.0f, -1.0f);
        }
    } else {
        for(int i = 0; i < m_stimEvokedSet.evoked.size(); ++i) {
            m_stimEvokedSet.evoked[i].baseline = m_pairBaselineSec;
        }
    }
}

//=============================================================================================================

void RtAveragingWorker::setBaselineFrom(int fromSamp,
                                        int fromMSec)
{
    m_pairBaselineSec.first = float(fromMSec)/1000.0f;
    m_pairBaselineSamp.first = float(fromSamp);

    for(int i = 0; i < m_stimEvokedSet.evoked.size(); ++i) {
        m_stimEvokedSet.evoked[i].baseline.first = float(fromMSec)/1000.0f;
    }
}

//=============================================================================================================

void RtAveragingWorker::setBaselineTo(int toSamp,
                                      int toMSec)
{
    m_pairBaselineSec.second = float(toMSec)/1000.0f;
    m_pairBaselineSamp.second = float(toSamp);

    for(int i = 0; i < m_stimEvokedSet.evoked.size(); ++i) {
        m_stimEvokedSet.evoked[i].baseline.second = float(toMSec)/1000.0f;
    }
}

//=============================================================================================================

void RtAveragingWorker::doAveraging(const MatrixXd& rawSegment)
{
    //Detect trigger
    QList<QPair<int,double> > lDetectedTriggers = RTPROCESSINGLIB::detectTriggerFlanksMax(rawSegment, m_iTriggerChIndex, 0, m_fTriggerThreshold, true);

    //TODO: This does not permit the same trigger type twice in one data block
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
    QStringList lResponsibleTriggerTypes;

    while(idx.hasNext()) {
        idx.next();

        double dTriggerType = idx.key();

        //Fill front / pre stim buffer
        if(lDetectedTriggers.isEmpty()) {
            fillFrontBuffer(rawSegment, -1.0);
        }

        //Fill back buffer and decide when to do the data packing of the different buffers
        if(m_mapFillingBackBuffer[dTriggerType]) {
            if(m_mapMatDataPostIdx[dTriggerType] != m_iPostStimSamples) {
                fillBackBuffer(rawSegment, dTriggerType);
            }

            if(m_mapMatDataPostIdx[dTriggerType] == m_iPostStimSamples) {
                m_mapMatDataPostIdx[dTriggerType] = 0;
                m_mapFillingBackBuffer[dTriggerType] = false;
                emitEvoked(dTriggerType, lResponsibleTriggerTypes);
            }
        } else {
            if(lDetectedTriggers.isEmpty()) {
                //Fill front / pre stim buffer
                fillFrontBuffer(rawSegment, dTriggerType);
            } else {
                for(int i = 0; i < lDetectedTriggers.size(); ++i) {
                    if(dTriggerType == lDetectedTriggers.at(i).second) {
                        int iTriggerPos = lDetectedTriggers.at(i).first;

                        //Do front buffer stuff
                        MatrixXd tempMat;

                        if(iTriggerPos >= m_iPreStimSamples) {
                            tempMat = rawSegment.block(0,
                                                       iTriggerPos - m_iPreStimSamples,
                                                       rawSegment.rows(),
                                                       m_iPreStimSamples);
                        } else {
                            tempMat = rawSegment.block(0,
                                                       0,
                                                       rawSegment.rows(),
                                                       iTriggerPos);
                        }

                        fillFrontBuffer(tempMat, dTriggerType);

                        //Do back buffer stuff
                        if(rawSegment.cols() - iTriggerPos >= m_mapDataPost[dTriggerType].cols()) {
                            m_mapDataPost[dTriggerType] = rawSegment.block(0,
                                                                           iTriggerPos,
                                                                           m_mapDataPost[dTriggerType].rows(),
                                                                           m_mapDataPost[dTriggerType].cols());
                            emitEvoked(dTriggerType, lResponsibleTriggerTypes);
                            m_mapMatDataPostIdx[dTriggerType] = 0;
                            m_mapFillingBackBuffer[dTriggerType] = false;
                        } else {
                            m_mapDataPost[dTriggerType].block(0,
                                                              0,
                                                              m_mapDataPost[dTriggerType].rows(),
                                                              rawSegment.cols() - iTriggerPos) = rawSegment.block(0,
                                                                                                                  iTriggerPos,
                                                                                                                  rawSegment.rows(),
                                                                                                                  rawSegment.cols() - iTriggerPos);
                            m_mapMatDataPostIdx[dTriggerType] = rawSegment.cols() - iTriggerPos;
                            m_mapFillingBackBuffer[dTriggerType] = true;
                        }

                        //qDebug()<<"Trigger type "<<dTriggerType<<" found at "<<iTriggerPos;
                    }
                }
            }
        }
    }

    //qDebug()<<"RtAveragingWorker::doAveraging() - time for procesing"<<time.elapsed();
}

//=============================================================================================================

void RtAveragingWorker::emitEvoked(double dTriggerType, QStringList& lResponsibleTriggerTypes)
{
    //Merge the different buffers
    mergeData(dTriggerType);

    //Calculate the final average/evoked data
    generateEvoked(dTriggerType);

    //List of all trigger types which lead to the recent emit of a new evoked set. */
    if(!lResponsibleTriggerTypes.contains(QString::number(dTriggerType))) {
        lResponsibleTriggerTypes << QString::number(dTriggerType);
    }

    if(m_stimEvokedSet.evoked.size() > 0) {
        emit resultReady(m_stimEvokedSet, lResponsibleTriggerTypes);
    }

//    qDebug()<<"RtAveragingWorker::emitEvoked() - dTriggerType:" << dTriggerType;
//    qDebug()<<"RtAveragingWorker::emitEvoked() - m_mapStimAve[dTriggerType].size():" << m_mapStimAve[dTriggerType].size();
}

//=============================================================================================================

void RtAveragingWorker::fillBackBuffer(const MatrixXd &data, double dTriggerType)
{
    int iResidualCols = data.cols();
    if(m_mapMatDataPostIdx[dTriggerType] + data.cols() > m_iPostStimSamples) {
        iResidualCols = m_iPostStimSamples - m_mapMatDataPostIdx[dTriggerType];
        m_mapDataPost[dTriggerType].block(0,m_mapMatDataPostIdx[dTriggerType],m_mapDataPost[dTriggerType].rows(),iResidualCols) = data.block(0,0,data.rows(),iResidualCols);
    } else {
        m_mapDataPost[dTriggerType].block(0,m_mapMatDataPostIdx[dTriggerType],m_mapDataPost[dTriggerType].rows(),iResidualCols) = data;
    }

    m_mapMatDataPostIdx[dTriggerType] += iResidualCols;
}

//=============================================================================================================

void RtAveragingWorker::fillFrontBuffer(const MatrixXd &data, double dTriggerType)
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
        if(m_iPreStimSamples > 0 && data.cols() >= m_iPreStimSamples) {
            m_mapDataPre[dTriggerType] = data.block(0,
                                                    data.cols() - m_iPreStimSamples,
                                                    data.rows(),
                                                    m_iPreStimSamples);
        }
    } else {
        int residual = m_mapDataPre[dTriggerType].cols() - data.cols();

        //Copy shift data
        m_mapDataPre[dTriggerType].block(0,
                                         0,
                                         m_mapDataPre[dTriggerType].rows(),
                                         residual) = m_mapDataPre[dTriggerType].block(0,
                                                                                      m_mapDataPre[dTriggerType].cols() - residual,
                                                                                      m_mapDataPre[dTriggerType].rows(),
                                                                                      residual);

        //Copy new data in
        m_mapDataPre[dTriggerType].block(0,
                                         residual,
                                         m_mapDataPre[dTriggerType].rows(),
                                         data.cols()) = data;
    }
}

//=============================================================================================================

void RtAveragingWorker::mergeData(double dTriggerType)
{
    if(m_mapDataPre[dTriggerType].rows() != m_mapDataPost[dTriggerType].rows()) {
        qDebug() << "[RtAveragingWorker::mergeData] Rows of m_mapDataPre (" << m_mapDataPre[dTriggerType].rows() << ") and m_mapDataPost (" << m_mapDataPost[dTriggerType].rows() << ") are not the same. Returning.";
        return;
    }

    MatrixXd mergedData(m_mapDataPre[dTriggerType].rows(), m_mapDataPre[dTriggerType].cols() + m_mapDataPost[dTriggerType].cols());

    mergedData << m_mapDataPre[dTriggerType], m_mapDataPost[dTriggerType];

    //Perform artifact threshold
    bool bArtifactDetected = false;

    if(m_bActivateThreshold && m_pFiffInfo) {
        qDebug() << "[RtAveragingWorker::mergeData] Doing artifact reduction for" << m_mapThresholds;

        bArtifactDetected = MNEEpochDataList::checkForArtifact(mergedData,
                                                               *m_pFiffInfo,
                                                               m_mapThresholds);
    }

    if(!bArtifactDetected) {
        //Add cut data to average buffer
        m_mapStimAve[dTriggerType].append(mergedData);

        //Pop data from buffer
        int iDiff =  m_mapStimAve[dTriggerType].size() - m_iNumAverages;
        if(iDiff > 0) {
            for(int i = 0; i < iDiff; ++i) {
                m_mapStimAve[dTriggerType].pop_front();
            }
        }
    }
}

//=============================================================================================================

void RtAveragingWorker::generateEvoked(double dTriggerType)
{
    if(m_mapStimAve[dTriggerType].isEmpty()) {
        qDebug() << "[RtAveragingWorker::generateEvoked] m_mapStimAve is empty for type" << dTriggerType << "Returning.";
        return;
    }

    //Init evoked
    m_stimEvokedSet.info = *m_pFiffInfo.data();
    FiffEvoked evoked;
    evoked.setInfo(*m_pFiffInfo.data());
    int iEvokedIdx = -1;

    for(int i = 0; i < m_stimEvokedSet.evoked.size(); ++i) {
        if(m_stimEvokedSet.evoked.at(i).comment == QString::number(dTriggerType)) {
            evoked = m_stimEvokedSet.evoked.at(i);
            iEvokedIdx = i;
            break;
        }
    }

    //If the evoked is not yet present add it here
    if(iEvokedIdx == -1) {
        evoked.baseline = m_pairBaselineSec;
        evoked.times.resize(m_iPreStimSamples + m_iPostStimSamples);
        evoked.times = RowVectorXf::LinSpaced(m_iPreStimSamples + m_iPostStimSamples,
                                              -1*m_iPreStimSamples/m_pFiffInfo->sfreq,
                                              m_iPostStimSamples/m_pFiffInfo->sfreq);
        evoked.times[m_iPreStimSamples] = 0.0;
        evoked.first = 0;
        evoked.last = m_iPreStimSamples + m_iPostStimSamples;
        evoked.comment = QString::number(dTriggerType);
    }

    // Generate final evoked
    MatrixXd finalAverage = MatrixXd::Zero(m_mapStimAve[dTriggerType].first().rows(), m_iPreStimSamples+m_iPostStimSamples);

    for(int i = 0; i < m_mapStimAve[dTriggerType].size(); ++i) {
        finalAverage += m_mapStimAve[dTriggerType].at(i);
    }

    if(!m_mapStimAve[dTriggerType].isEmpty()) {
        finalAverage = finalAverage/m_mapStimAve[dTriggerType].size();
    }

    if(m_bDoBaselineCorrection) {
        finalAverage = MNEMath::rescale(finalAverage, evoked.times, m_pairBaselineSec, QString("mean"));
    }

    evoked.data = finalAverage;

    evoked.nave = m_mapStimAve[dTriggerType].size();

    //Add new data to evoked data set
    if(iEvokedIdx != -1) {
        //Evoked data is already present
        m_stimEvokedSet.evoked[iEvokedIdx] = evoked;
    } else {
        //Evoked data is not present yet
        m_stimEvokedSet.evoked.append(evoked);
    }
}

//=============================================================================================================

void RtAveragingWorker::reset()
{
    //Reset
    m_iPreStimSamples = m_iNewPreStimSamples;
    m_iPostStimSamples = m_iNewPostStimSamples;
    m_iTriggerChIndex = m_iNewTriggerIndex;

    //Clear all evoked data information
    m_stimEvokedSet.evoked.clear();

    //Clear all maps
    m_mapStimAve.clear();
    m_mapDataPre.clear();
    m_mapDataPre[-1.0] = MatrixXd::Zero(m_pFiffInfo->chs.size(), m_iPreStimSamples);
    m_mapDataPost.clear();
    m_mapMatDataPostIdx.clear();
    m_mapFillingBackBuffer.clear();
}

//=============================================================================================================
// DEFINE MEMBER METHODS RtAveraging
//=============================================================================================================

RtAveraging::RtAveraging(quint32 numAverages,
                         quint32 iPreStimSamples,
                         quint32 iPostStimSamples,
                         quint32 iBaselineFromSecs,
                         quint32 iBaselineToSecs,
                         quint32 iTriggerIndex,
                         FiffInfo::SPtr pFiffInfo,
                         QObject *parent)
: QObject(parent)
{
    qRegisterMetaType<Eigen::MatrixXd>("Eigen::MatrixXd");

    RtAveragingWorker *worker = new RtAveragingWorker(numAverages,
                                                      iPreStimSamples,
                                                      iPostStimSamples,
                                                      iBaselineFromSecs,
                                                      iBaselineToSecs,
                                                      iTriggerIndex,
                                                      pFiffInfo);
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtAveraging::operate,
            worker, &RtAveragingWorker::doWork);

    connect(worker, &RtAveragingWorker::resultReady,
            this, &RtAveraging::handleResults, Qt::DirectConnection);

    connect(this, &RtAveraging::averageNumberChanged,
            worker, &RtAveragingWorker::setAverageNumber);
    connect(this, &RtAveraging::averagePreStimChanged,
            worker, &RtAveragingWorker::setPreStim);
    connect(this, &RtAveraging::averagePostStimChanged,
            worker, &RtAveragingWorker::setPostStim);
    connect(this, &RtAveraging::averageTriggerChIdxChanged,
            worker, &RtAveragingWorker::setTriggerChIndx);
    connect(this, &RtAveraging::averageArtifactReductionChanged,
            worker, &RtAveragingWorker::setArtifactReduction);
    connect(this, &RtAveraging::averageBaselineActiveChanged,
            worker, &RtAveragingWorker::setBaselineActive);
    connect(this, &RtAveraging::averageBaselineFromChanged,
            worker, &RtAveragingWorker::setBaselineFrom);
    connect(this, &RtAveraging::averageBaselineToChanged,
            worker, &RtAveragingWorker::setBaselineTo);
    connect(this, &RtAveraging::averageResetRequested,
            worker, &RtAveragingWorker::reset);

    m_workerThread.start();
}

//=============================================================================================================

RtAveraging::~RtAveraging()
{
    stop();
}

//=============================================================================================================

void RtAveraging::append(const MatrixXd &data)
{
    emit operate(data);
}

//=============================================================================================================

void RtAveraging::handleResults(const FiffEvokedSet& evokedStimSet,
                          const QStringList &lResponsibleTriggerTypes)
{
    emit evokedStim(evokedStimSet,
                    lResponsibleTriggerTypes);
}

//=============================================================================================================

void RtAveraging::restart(quint32 numAverages,
                          quint32 iPreStimSamples,
                          quint32 iPostStimSamples,
                          quint32 iBaselineFromSecs,
                          quint32 iBaselineToSecs,
                          quint32 iTriggerIndex,
                          FiffInfo::SPtr pFiffInfo)
{
    stop();

    RtAveragingWorker *worker = new RtAveragingWorker(numAverages,
                                                      iPreStimSamples,
                                                      iPostStimSamples,
                                                      iBaselineFromSecs,
                                                      iBaselineToSecs,
                                                      iTriggerIndex,
                                                      pFiffInfo);
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    connect(this, &RtAveraging::operate,
            worker, &RtAveragingWorker::doWork);

    connect(worker, &RtAveragingWorker::resultReady,
            this, &RtAveraging::handleResults, Qt::DirectConnection);

    connect(this, &RtAveraging::averageNumberChanged,
            worker, &RtAveragingWorker::setAverageNumber);
    connect(this, &RtAveraging::averagePreStimChanged,
            worker, &RtAveragingWorker::setPreStim);
    connect(this, &RtAveraging::averagePostStimChanged,
            worker, &RtAveragingWorker::setPostStim);
    connect(this, &RtAveraging::averageTriggerChIdxChanged,
            worker, &RtAveragingWorker::setTriggerChIndx);
    connect(this, &RtAveraging::averageArtifactReductionChanged,
            worker, &RtAveragingWorker::setArtifactReduction);
    connect(this, &RtAveraging::averageBaselineActiveChanged,
            worker, &RtAveragingWorker::setBaselineActive);
    connect(this, &RtAveraging::averageBaselineFromChanged,
            worker, &RtAveragingWorker::setBaselineFrom);
    connect(this, &RtAveraging::averageBaselineToChanged,
            worker, &RtAveragingWorker::setBaselineTo);
    connect(this, &RtAveraging::averageResetRequested,
            worker, &RtAveragingWorker::reset);

    m_workerThread.start();
}

//=============================================================================================================

void RtAveraging::stop()
{
    m_workerThread.requestInterruption();
    m_workerThread.quit();
    m_workerThread.wait();
}

//=============================================================================================================

void RtAveraging::setAverageNumber(qint32 numAve)
{
    emit averageNumberChanged(numAve);
}

//=============================================================================================================

void RtAveraging::setPreStim(qint32 samples,
                             qint32 secs)
{
    Q_UNUSED(secs);

    emit averagePreStimChanged(samples,
                               secs);
}

//=============================================================================================================

void RtAveraging::setPostStim(qint32 samples,
                              qint32 secs)
{
    Q_UNUSED(secs);

    emit averagePostStimChanged(samples,
                                secs);
}

//=============================================================================================================

void RtAveraging::setTriggerChIndx(qint32 idx)
{
    emit averageTriggerChIdxChanged(idx);
}

//=============================================================================================================

void RtAveraging::setArtifactReduction(const QMap<QString,double>& mapThresholds)
{
    emit averageArtifactReductionChanged(mapThresholds);
}

//=============================================================================================================

void RtAveraging::setBaselineActive(bool activate)
{
    emit averageBaselineActiveChanged(activate);
}

//=============================================================================================================

void RtAveraging::setBaselineFrom(int fromSamp,
                                  int fromMSec)
{
    emit averageBaselineFromChanged(fromSamp,
                                    fromMSec);
}

//=============================================================================================================

void RtAveraging::setBaselineTo(int toSamp,
                                int toMSec)
{
    emit averageBaselineToChanged(toSamp,
                                  toMSec);
}

//=============================================================================================================

void RtAveraging::reset()
{
    emit averageResetRequested();
}
