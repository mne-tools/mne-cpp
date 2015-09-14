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

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutexLocker>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTINVLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtAve::RtAve(quint32 numAverages, quint32 p_iPreStimSamples, quint32 p_iPostStimSamples, FiffInfo::SPtr p_pFiffInfo, QObject *parent)
: QThread(parent)
, m_iNumAverages(numAverages)
, m_iPreStimSamples(p_iPreStimSamples)
, m_iPostStimSamples(p_iPostStimSamples)
, m_pFiffInfo(p_pFiffInfo)
, m_bIsRunning(false)
, m_bAutoAspect(true)
, m_fTriggerThreshold(0.02)
, m_bFillingBackBuffer(false)
, m_iTriggerIndex(306)
, m_iTriggerPos(-1)
, m_bRunningAverage(false)
, m_bDoBaselineCorrection(true)
, m_baseline(qMakePair(0,m_iPreStimSamples))
{
    qRegisterMetaType<FiffEvoked::SPtr>("FiffEvoked::SPtr");
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
    QMutexLocker locker(&m_qMutex);
    // ToDo handle change buffersize
    if(!m_pRawMatrixBuffer)
        m_pRawMatrixBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(128, p_DataSegment.rows(), p_DataSegment.cols()));

    m_pRawMatrixBuffer->push(&p_DataSegment);
}


//*************************************************************************************************************

void RtAve::setAverages(qint32 numAve)
{
    m_qMutex.lock();
    m_iNumAverages = numAve;
    m_qMutex.unlock();
    emit numAveragesChanged();
}


//*************************************************************************************************************

void RtAve::setPreStim(qint32 samples)
{
    QMutexLocker locker(&m_qMutex);
    m_iNewPreStimSamples = samples;
}


//*************************************************************************************************************

void RtAve::setPostStim(qint32 samples)
{
    QMutexLocker locker(&m_qMutex);
    m_iNewPostStimSamples = samples;
}


//*************************************************************************************************************

void RtAve::setTriggerChIndx(qint32 idx)
{
    m_iNewTriggerIndex = idx;
}


//*************************************************************************************************************

void RtAve::setBaselineActive(bool activate)
{
    m_bDoBaselineCorrection = activate;

    if(!m_bDoBaselineCorrection)
        m_pStimEvoked->baseline = qMakePair(QVariant("None"), QVariant("None"));
}


//*************************************************************************************************************

void RtAve::setBaselineMin(int min)
{
    if(min<0)
        m_baseline.first = 0;
    else if(min>m_iPostStimSamples+m_iPreStimSamples)
        m_baseline.first = m_iPostStimSamples+m_iPreStimSamples;
    else
        m_baseline.first = min;

    if(m_bDoBaselineCorrection)
        m_pStimEvoked->baseline = m_baseline;
}


//*************************************************************************************************************

void RtAve::setBaselineMax(int max)
{
    if(max<0)
        m_baseline.second = 0;
    else if(max>m_iPostStimSamples+m_iPreStimSamples)
        m_baseline.second = m_iPostStimSamples+m_iPreStimSamples;
    else
        m_baseline.second = max;

    if(m_bDoBaselineCorrection)
        m_pStimEvoked->baseline = m_baseline;
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
    //Inits & Clears
    m_qMutex.lock();
    m_qListStimAve.clear();

    MatrixXd t_mat;
    QList<int> tempList;

    for(int i = 0; i < m_pFiffInfo->nchan; ++i) {
        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH && (m_pFiffInfo->chs[i].ch_name != QString("STI 014"))) {
            m_qListStimAve.push_back(t_mat);
            m_qMapDetectedTrigger.insert(i,tempList);
        }
    }

    float T = 1.0/m_pFiffInfo->sfreq;

    m_pStimEvoked = FiffEvoked::SPtr(new FiffEvoked);
    m_pStimEvoked->setInfo(*m_pFiffInfo.data());
    m_pStimEvoked->nave = 0;
    m_pStimEvoked->aspect_kind = FIFFV_ASPECT_AVERAGE;
    m_pStimEvoked->times.resize(m_iPreStimSamples+m_iPostStimSamples);
    m_pStimEvoked->times[0] = -T*m_iPreStimSamples;
    for(int i = 1; i < m_pStimEvoked->times.size(); ++i)
        m_pStimEvoked->times[i] = m_pStimEvoked->times[i-1] + T;
    m_pStimEvoked->first = m_pStimEvoked->times[0];
    m_pStimEvoked->last = m_pStimEvoked->times[m_pStimEvoked->times.size()-1];

    m_iNewPreStimSamples = m_iPreStimSamples;
    m_iNewPostStimSamples = m_iPostStimSamples;

    m_baseline = qMakePair(0,m_iNewPreStimSamples);

    m_qMutex.unlock();

    //Enter the main loop
    while(m_bIsRunning)
    {
        bool doProcessing = false;

        if(m_pRawMatrixBuffer)
            doProcessing = true;

        if(doProcessing) {
            //Reset when stim size changed
            m_qMutex.lock();
            if(m_iNewPreStimSamples != m_iPreStimSamples
                    || m_iNewPostStimSamples != m_iPostStimSamples
                    || m_iNewTriggerIndex != m_iTriggerIndex)
            {
                m_iPreStimSamples = m_iNewPreStimSamples;
                m_iPostStimSamples = m_iNewPostStimSamples;
                m_iTriggerIndex = m_iNewTriggerIndex;

                //Full real-time evoked response
                m_pStimEvoked->times.resize(m_iPreStimSamples+m_iPostStimSamples);
                m_pStimEvoked->times[0] = -T*m_iPreStimSamples;
                for(int i = 1; i < m_pStimEvoked->times.size(); ++i)
                    m_pStimEvoked->times[i] = m_pStimEvoked->times[i-1] + T;
                m_pStimEvoked->first = m_pStimEvoked->times[0];
                m_pStimEvoked->last = m_pStimEvoked->times[m_pStimEvoked->times.size()-1];

                m_qListStimAve.clear();
                m_matBufferFront.clear();
                m_matBufferBack.clear();

                m_bFillingBackBuffer = false;
            }
            m_qMutex.unlock();

            //Acquire Data
            MatrixXd rawSegment = m_pRawMatrixBuffer->pop();
            m_iCurrentBlockSize = rawSegment.cols();

            //Fill back buffer and decide when to do the data packing of the different buffers
            if(m_bFillingBackBuffer) {
                if(fillBackBuffer(rawSegment) == m_iPostStimSamples) {
                    //Merge the different buffers
                    mergeData();

                    //Clear data
                    m_matBufferBack.clear();
                    m_matBufferFront.clear();

                    //Calculate the actual average
                    generateEvoked();

                    //If number of averages was reached emit new average
                    if(m_qListStimAve.size() == m_iNumAverages && m_bRunningAverage) {
                        m_pStimEvoked->nave = m_iNumAverages;
                        emit evokedStim(m_pStimEvoked);
                    }
                    else if(m_qListStimAve.size()>0)
                        emit evokedStim(m_pStimEvoked);

                    m_bFillingBackBuffer = false;
                }
            } else {
                clearDetectedTriggers();

                //Fill front / pre stim buffer
                fillFrontBuffer(rawSegment);

                //Detect trigger for all stim channels. If detected turn on filling of the back / post stim buffer
                if(DetectTrigger::detectTriggerFlanksMax(rawSegment, m_iTriggerIndex, m_iTriggerPos, 0, m_fTriggerThreshold, true)) {
                    m_matStimData = rawSegment;
                    m_bFillingBackBuffer = true;
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

int RtAve::fillBackBuffer(MatrixXd &data)
{
    int iTotalBufSize = 0;
    int iResidualCols = data.cols();
    for(int i = 0; i < m_matBufferBack.size(); i++)
        iTotalBufSize += m_matBufferBack.at(i).cols();

    if(iTotalBufSize < m_iPostStimSamples) {
        if(iTotalBufSize+data.cols() > m_iPostStimSamples) {
            iResidualCols = m_iPostStimSamples-iTotalBufSize;
            m_matBufferBack.append(data.block(0,0,data.cols(),iResidualCols));
        } else
            m_matBufferBack.append(data);
    }

    //DEBUG
    //qDebug()<<"m_matBufferBack: "<<iTotalBufSize+iResidualCols;

    return iTotalBufSize+iResidualCols;
}


//*************************************************************************************************************

void RtAve::fillFrontBuffer(MatrixXd &data)
{
    if(m_iCurrentMatBufferIndex == 0)
        m_matBufferFront.clear();

    if(m_iPreStimSamples <= m_iCurrentBlockSize) {
        m_matBufferFront.prepend(data.block(0,m_iCurrentBlockSize-m_iPreStimSamples,data.rows(),m_iPreStimSamples));
        m_iCurrentMatBufferIndex = 0;
    } else {
        if(m_iCurrentMatBufferIndex+m_iCurrentBlockSize > m_iPreStimSamples) {
            int numberCols = m_iPreStimSamples % m_iCurrentBlockSize;
            m_matBufferFront.prepend(data.block(0,m_iCurrentBlockSize-numberCols,data.rows(),numberCols));
            m_iCurrentMatBufferIndex = 0;
        } else {
            m_matBufferFront.prepend(data);
            m_iCurrentMatBufferIndex += m_iCurrentBlockSize;
        }
    }

    // DEBUG - Check total data length of front buffer
    int size = 0;
    for(int i = 0; i<m_matBufferFront.size(); i++)
        size += m_matBufferFront.at(i).cols();
    //qDebug()<<"m_matBufferFront: "<<size;
}


//*************************************************************************************************************

void RtAve::mergeData()
{
    int rows = m_matStimData.rows();
    MatrixXd cutData(rows, m_iPreStimSamples+m_iPostStimSamples);
    MatrixXd mergedData(rows, m_iPreStimSamples+m_iPostStimSamples+m_matStimData.cols());

    for(int i = 0; i < m_matBufferFront.size(); i++)
        mergedData.block(0,i*m_iCurrentBlockSize,rows,m_matBufferFront.at(i).cols()) =  m_matBufferFront.at(i);

    mergedData.block(0,m_iPreStimSamples,rows,m_matStimData.cols()) =  m_matStimData;

    for(int i = 0; i < m_matBufferBack.size(); i++)
        mergedData.block(0,m_iPreStimSamples+m_matStimData.cols()+i*m_iCurrentBlockSize,rows,m_matBufferBack.at(i).cols()) =  m_matBufferBack.at(i);

    cutData = mergedData.block(0,m_iPreStimSamples+m_iTriggerPos-m_iPreStimSamples,rows,m_iPreStimSamples+m_iPostStimSamples);

    //Add cut data to average buffer
    m_qListStimAve.append(cutData);
    if(m_qListStimAve.size()>m_iNumAverages && m_bRunningAverage)
        m_qListStimAve.pop_front();
}


//*************************************************************************************************************

void RtAve::generateEvoked()
{
    // Emit final evoked
    MatrixXd finalAverage = MatrixXd::Zero(m_matStimData.rows(), m_iPreStimSamples+m_iPostStimSamples);

    if(m_bRunningAverage) {
        for(int i = 0; i<m_qListStimAve.size(); i++) {
            finalAverage += m_qListStimAve.at(i);
        }
        finalAverage = finalAverage/m_qListStimAve.size();

        if(m_bDoBaselineCorrection)
            finalAverage = MNEMath::rescale(finalAverage, m_pStimEvoked->times, m_baseline, QString("mean"));

        m_pStimEvoked->data = finalAverage;
    } else {
        MatrixXd tempMatrix = m_qListStimAve.last();

        if(m_bDoBaselineCorrection)
            tempMatrix = MNEMath::rescale(tempMatrix, m_pStimEvoked->times, m_baseline, QString("mean"));

        *m_pStimEvoked.data() += tempMatrix;
    }

    //qDebug()<<"nave "<<m_pStimEvoked->nave;
}


