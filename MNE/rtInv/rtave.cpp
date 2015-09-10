//=============================================================================================================
/**
* @file     rtave.cpp
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
    // Inits & Clears
    m_qMutex.lock();

    m_qListStimAve.clear();

    // Init
    MatrixXd t_mat;
    QList<int> tempList;

    for(int i = 0; i < m_pFiffInfo->nchan; ++i) {
        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH && (m_pFiffInfo->chs[i].ch_name != QString("STI 014"))) {
            m_qListStimAve.push_back(t_mat);
            m_qMapDetectedTrigger.insert(i,tempList);
        }
    }

    float T = 1.0/m_pFiffInfo->sfreq;

    FiffEvoked t_stimEvoked;
    t_stimEvoked.setInfo(*m_pFiffInfo.data());
    t_stimEvoked.nave = m_iNumAverages;
    t_stimEvoked.aspect_kind = FIFFV_ASPECT_AVERAGE;
    t_stimEvoked.times.resize(m_iPreStimSamples+m_iPostStimSamples);
    t_stimEvoked.times[0] = -T*m_iPreStimSamples;
    for(int i = 1; i < t_stimEvoked.times.size(); ++i)
        t_stimEvoked.times[i] = t_stimEvoked.times[i-1] + T;
    t_stimEvoked.first = t_stimEvoked.times[0];
    t_stimEvoked.last = t_stimEvoked.times[t_stimEvoked.times.size()-1];

    m_iNewPreStimSamples = m_iPreStimSamples;
    m_iNewPostStimSamples = m_iPostStimSamples;

    m_qMutex.unlock();

    //Enter the main loop
    while(m_bIsRunning)
    {
        qDebug()<<"Running";

        bool doProcessing = false;

        if(m_pRawMatrixBuffer)
            doProcessing = true;

        if(doProcessing) {
            // Reset when stim size changed
            m_qMutex.lock();
            if(m_iNewPreStimSamples != m_iPreStimSamples || m_iNewPostStimSamples != m_iPostStimSamples)
            {
                m_iPreStimSamples = m_iNewPreStimSamples;
                m_iPostStimSamples = m_iNewPostStimSamples;

                // Full real-time evoked response
                t_stimEvoked.times.resize(m_iPreStimSamples+m_iPostStimSamples);
                t_stimEvoked.times[0] = -T*m_iPreStimSamples;
                for(int i = 1; i < t_stimEvoked.times.size(); ++i)
                    t_stimEvoked.times[i] = t_stimEvoked.times[i-1] + T;
                t_stimEvoked.first = t_stimEvoked.times[0];
                t_stimEvoked.last = t_stimEvoked.times[t_stimEvoked.times.size()-1];

                m_qListStimAve.clear();
                m_matBufferFront.clear();
            }
            m_qMutex.unlock();

            // Acquire Data
            MatrixXd rawSegment = m_pRawMatrixBuffer->pop();
            m_iCurrentBlockSize = rawSegment.cols();

            // Clear old detected triggers
            clearDetectedTriggers();

            // Detect trigger for all stim channels
            if(DetectTrigger::detectTriggerFlanksMax(rawSegment, m_qMapDetectedTrigger, 0, m_fTriggerThreshold, true)) {
                // If triggers were detected

            } else {
                // If no triggers were detected
            }

            // DEBUG - Detect trigger for all stim channels
            QMutableMapIterator<int,QList<int> > iteratorDetectedTrigger(m_qMapDetectedTrigger);
            while (iteratorDetectedTrigger.hasNext()) {
                iteratorDetectedTrigger.next();
                qDebug()<<"Channel "<<iteratorDetectedTrigger.key()<<" has "<<iteratorDetectedTrigger.value().size()<<" triggers";
                iteratorDetectedTrigger.value().clear();
            }

            // Fill front/pre stim buffer
            fillFrontBuffer(rawSegment);

//            // Emit final evoked
//            FiffEvoked::SPtr t_pEvokedStim(new FiffEvoked(t_stimEvoked));
//            t_pEvokedStim->nave = m_iNumAverages;
//            t_pEvokedStim->comment = t_sStimChName;
//            t_pEvokedStim->data = m_qListStimAve[t_iStimIndex];
//            emit evokedStim(t_pEvokedStim);
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

    int size = 0;
    for(int i = 0; i<m_matBufferFront.size(); i++)
        size+=m_matBufferFront.at(i).cols();

    qDebug()<<size;
}
