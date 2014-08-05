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
    // ToDo handle change buffersize
    if(!m_pRawMatrixBuffer)
        m_pRawMatrixBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(128, p_DataSegment.rows(), p_DataSegment.cols()));

    m_pRawMatrixBuffer->push(&p_DataSegment);
}


//*************************************************************************************************************

void RtAve::setAverages(qint32 numAve)
{
    m_iNumAverages = numAve;
    emit numAveragesChanged();
}


//*************************************************************************************************************

void RtAve::setPreStim(qint32 samples)
{
    qDebug() << "RtAve::setPreStim\n" << samples;
    m_iNewPreStimSamples = samples;
}


//*************************************************************************************************************

void RtAve::setPostStim(qint32 samples)
{
    qDebug() << "RtAve::setPostStim\n" << samples;
    m_iNewPostStimSamples = samples;
}


//*************************************************************************************************************

void RtAve::assemblePostStimulus(const QList<QPair<QList<qint32>, MatrixXd> > &p_qListRawMatBuf, qint32 p_iStimIdx)
{
    if(m_iPreStimSamples > 0)
    {
        // middle of the assembled buffers
        float ratio = (float)m_iPreStimSamples/((float)(m_iPreStimSamples+m_iPostStimSamples));
        qint32 t_iBuffWithStim = p_qListRawMatBuf.size()*ratio;

        qint32 nrows = p_qListRawMatBuf[t_iBuffWithStim].second.rows();
        qint32 ncols = p_qListRawMatBuf[t_iBuffWithStim].second.cols();

        //Stimulus channel row
        qint32 t_iRowIdx = m_qListStimChannelIdcs[p_iStimIdx];

        qint32 nSampleCount = 0;

        MatrixXd t_matPostStim(nrows, m_iPostStimSamples);
        qint32 t_curBufIdx = t_iBuffWithStim;

        qint32 t_iSize = 0;

        qint32 pos = 0;
        p_qListRawMatBuf[t_iBuffWithStim].second.row(t_iRowIdx).maxCoeff(&pos);

        //
        // assemble poststimulus
        //

        // start from the stimulus itself
        if(pos > 0)
        {
            t_iSize = ncols - pos;
            if(t_iSize <= m_iPostStimSamples)
            {
                t_matPostStim.block(0, 0, nrows, t_iSize) = p_qListRawMatBuf[t_iBuffWithStim].second.block(0, pos, nrows, t_iSize);
                nSampleCount += t_iSize;
            }
            else
            {
                t_matPostStim.block(0, 0, nrows, m_iPostStimSamples) = p_qListRawMatBuf[t_iBuffWithStim].second.block(0, pos, nrows, m_iPostStimSamples);
                nSampleCount = m_iPostStimSamples;
            }

            ++t_curBufIdx;
        }

        // remaining samples
        while(nSampleCount < m_iPostStimSamples)
        {
            t_iSize = m_iPostStimSamples-nSampleCount;

            if(ncols <= t_iSize)
            {
                t_matPostStim.block(0, nSampleCount, nrows, ncols) = p_qListRawMatBuf[t_curBufIdx].second.block(0, 0, nrows, ncols);
                nSampleCount += ncols;
            }
            else
            {
                t_matPostStim.block(0, nSampleCount, nrows, t_iSize) = p_qListRawMatBuf[t_curBufIdx].second.block(0, 0, nrows, t_iSize);
                nSampleCount = m_iPostStimSamples;
            }

            ++t_curBufIdx;
        }

        //
        //Store in right post stimulus buffer vector
        //
        m_qListQListPostStimBuf[p_iStimIdx].append(t_matPostStim);
    }
}


//*************************************************************************************************************

void RtAve::assemblePreStimulus(const QList<QPair<QList<qint32>, MatrixXd> > &p_qListRawMatBuf, qint32 p_iStimIdx)
{
    if(m_iPreStimSamples > 0)
    {
        // stimulus containing buffer of the assembled buffers
        float ratio = (float)m_iPreStimSamples/((float)(m_iPreStimSamples+m_iPostStimSamples));
        qint32 t_iBuffWithStim = p_qListRawMatBuf.size()*ratio;

        qint32 nrows = p_qListRawMatBuf[t_iBuffWithStim].second.rows();
        qint32 ncols = p_qListRawMatBuf[t_iBuffWithStim].second.cols();

        //Stimulus channel row
        qint32 t_iRowIdx = m_qListStimChannelIdcs[p_iStimIdx];

        qint32 nSampleCount = m_iPreStimSamples;

        MatrixXd t_matPreStim(nrows, m_iPreStimSamples);
        qint32 t_curBufIdx = t_iBuffWithStim;

        qint32 t_iStart = 0;

        qint32 pos = 0;
        p_qListRawMatBuf[t_iBuffWithStim].second.row(t_iRowIdx).maxCoeff(&pos);

        //
        // assemble prestimulus
        //

        // start from the stimulus itself
        if(pos > 0)
        {
            t_iStart = m_iPreStimSamples - pos;
            if(t_iStart >= 0)
            {
                t_matPreStim.block(0, t_iStart, nrows, pos) = p_qListRawMatBuf[t_iBuffWithStim].second.block(0, 0, nrows, pos);
                nSampleCount -= pos;
            }
            else
            {
                t_matPreStim.block(0, 0, nrows, m_iPreStimSamples) = p_qListRawMatBuf[t_iBuffWithStim].second.block(0, -t_iStart, nrows, m_iPreStimSamples);
                nSampleCount = 0;
            }

            --t_curBufIdx;
        }

        // remaining samples
        while(nSampleCount > 0)
        {
            t_iStart = nSampleCount - ncols;

            if(t_iStart >= 0)
            {
                t_matPreStim.block(0, t_iStart, nrows, ncols) = p_qListRawMatBuf[t_curBufIdx].second.block(0, 0, nrows, ncols);
                nSampleCount -= ncols;
            }
            else
            {
                t_matPreStim.block(0, 0, nrows, nSampleCount) = p_qListRawMatBuf[t_curBufIdx].second.block(0, -t_iStart, nrows, nSampleCount);
                nSampleCount = 0;
            }

            --t_curBufIdx;
        }

        //
        //Store in right pre stimulus buffer vector
        //
        m_qListQListPreStimBuf[p_iStimIdx].append(t_matPreStim);
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
    m_bIsRunning = false;

    m_pRawMatrixBuffer->releaseFromPop();

    m_pRawMatrixBuffer->clear();

    return true;
}


//*************************************************************************************************************

void RtAve::run()
{
    //
    // Inits & Clears
    //
    quint32 t_nSamplesPerBuf = 0;
    QList<QPair<QList<qint32>, MatrixXd> > t_qListRawMatBuf;

    FiffEvoked::SPtr evoked(new FiffEvoked());
    VectorXd mu;
    qint32 i = 0;
    qint32 j = 0;

    m_qListQListPreStimBuf.clear();
    m_qListQListPostStimBuf.clear();
    m_qListPreStimAve.clear();
    m_qListPostStimAve.clear();
    m_qListStimAve.clear();

    //
    // get num stim channels
    //
    m_qListStimChannelIdcs.clear();
    MatrixXd t_mat;
    QList<MatrixXd> t_qListMat;
    for(i = 0; i < m_pFiffInfo->nchan; ++i)
    {
        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH && (m_pFiffInfo->chs[i].ch_name != QString("STI 014")))
        {
            m_qListStimChannelIdcs.append(i);

            m_qListQListPreStimBuf.push_back(t_qListMat);
            m_qListQListPostStimBuf.push_back(t_qListMat);
            m_qListPreStimAve.push_back(t_mat);
            m_qListPostStimAve.push_back(t_mat);
            m_qListStimAve.push_back(t_mat);
        }
    }


    float T = 1.0/m_pFiffInfo->sfreq;

    // pre real-time evoked response
    FiffEvoked t_preStimEvoked;
    t_preStimEvoked.setInfo(*m_pFiffInfo.data());
    t_preStimEvoked.nave = m_iNumAverages;
    t_preStimEvoked.aspect_kind = FIFFV_ASPECT_AVERAGE;
    t_preStimEvoked.times.resize(m_iPreStimSamples);
    t_preStimEvoked.times[0] = -T*m_iPreStimSamples;
    for(i = 1; i < t_preStimEvoked.times.size(); ++i)
        t_preStimEvoked.times[i] = t_preStimEvoked.times[i-1] + T;
    t_preStimEvoked.first = t_preStimEvoked.times[0];
    t_preStimEvoked.last = t_preStimEvoked.times[t_preStimEvoked.times.size()-1];

    // post real-time evoked full response
    FiffEvoked t_postStimEvoked;
    t_postStimEvoked.setInfo(*m_pFiffInfo.data());
    t_postStimEvoked.nave = m_iNumAverages;
    t_postStimEvoked.aspect_kind = FIFFV_ASPECT_AVERAGE;
    t_postStimEvoked.times.resize(m_iPostStimSamples);
    t_postStimEvoked.times[0] = 0;
    for(i = 1; i < t_postStimEvoked.times.size(); ++i)
        t_postStimEvoked.times[i] = t_postStimEvoked.times[i-1] + T;
    t_postStimEvoked.first = t_postStimEvoked.times[0];
    t_postStimEvoked.last = t_postStimEvoked.times[t_postStimEvoked.times.size()-1];

    // Full real-time evoked response
    FiffEvoked t_stimEvoked;
    t_stimEvoked.setInfo(*m_pFiffInfo.data());
    t_stimEvoked.nave = m_iNumAverages;
    t_stimEvoked.aspect_kind = FIFFV_ASPECT_AVERAGE;
    t_stimEvoked.times.resize(m_iPreStimSamples+m_iPostStimSamples);
    t_stimEvoked.times[0] = -T*m_iPreStimSamples;
    for(i = 1; i < t_stimEvoked.times.size(); ++i)
        t_stimEvoked.times[i] = t_stimEvoked.times[i-1] + T;
    t_stimEvoked.first = t_stimEvoked.times[0];
    t_stimEvoked.last = t_stimEvoked.times[t_stimEvoked.times.size()-1];


    qint32 count = 0;

    m_iNewPreStimSamples = m_iPreStimSamples;
    m_iNewPostStimSamples = m_iPostStimSamples;

    //Enter the main loop
    while(m_bIsRunning)
    {
        if(m_pRawMatrixBuffer)
        {
            //
            // Reset when stim size changed
            //
            if(m_iNewPreStimSamples != m_iPreStimSamples || m_iNewPostStimSamples != m_iPostStimSamples)
            {
                m_iPreStimSamples = m_iNewPreStimSamples;
                m_iPostStimSamples = m_iNewPostStimSamples;

                // pre real-time evoked response
                t_preStimEvoked.times.resize(m_iPreStimSamples);
                t_preStimEvoked.times[0] = -T*m_iPreStimSamples;
                for(i = 1; i < t_preStimEvoked.times.size(); ++i)
                    t_preStimEvoked.times[i] = t_preStimEvoked.times[i-1] + T;
                t_preStimEvoked.first = t_preStimEvoked.times[0];
                t_preStimEvoked.last = t_preStimEvoked.times[t_preStimEvoked.times.size()-1];

                // post real-time evoked full response
                t_postStimEvoked.times.resize(m_iPostStimSamples);
                t_postStimEvoked.times[0] = 0;
                for(i = 1; i < t_postStimEvoked.times.size(); ++i)
                    t_postStimEvoked.times[i] = t_postStimEvoked.times[i-1] + T;
                t_postStimEvoked.first = t_postStimEvoked.times[0];
                t_postStimEvoked.last = t_postStimEvoked.times[t_postStimEvoked.times.size()-1];

                // Full real-time evoked response
                t_stimEvoked.times.resize(m_iPreStimSamples+m_iPostStimSamples);
                t_stimEvoked.times[0] = -T*m_iPreStimSamples;
                for(i = 1; i < t_stimEvoked.times.size(); ++i)
                    t_stimEvoked.times[i] = t_stimEvoked.times[i-1] + T;
                t_stimEvoked.first = t_stimEvoked.times[0];
                t_stimEvoked.last = t_stimEvoked.times[t_stimEvoked.times.size()-1];


                MatrixXd t_resetMat;
                t_qListRawMatBuf.clear();
                m_qListPreStimAve.clear();
                m_qListPostStimAve.clear();
                m_qListStimAve.clear();

                for(qint32 i = 0; i < m_qListQListPreStimBuf.size(); ++i)
                {
                    m_qListQListPreStimBuf[i].clear();
                    m_qListQListPostStimBuf[i].clear();
                    m_qListPreStimAve.push_back(t_resetMat);
                    m_qListPostStimAve.push_back(t_resetMat);
                    m_qListStimAve.push_back(t_resetMat);
                }
            }

            //
            // Acquire Data
            //
            MatrixXd rawSegment = m_pRawMatrixBuffer->pop();
            if(t_nSamplesPerBuf == 0)
                t_nSamplesPerBuf = rawSegment.cols();

            ++count;

            //
            // Detect Stimuli
            //
            QList<qint32> t_qListStimuli;
            for(i = 0; i < m_qListStimChannelIdcs.size(); ++i)
            {
                qint32 idx = m_qListStimChannelIdcs[i];
                RowVectorXi stimSegment = rawSegment.row(idx).cast<int>();
                int iMax = stimSegment.maxCoeff();

                if(iMax > 0)
                    t_qListStimuli.append(i);
            }

            //
            // Store
            //
            t_qListRawMatBuf.push_back(qMakePair(t_qListStimuli, rawSegment));

            if(t_nSamplesPerBuf*t_qListRawMatBuf.size() > (m_iPreStimSamples+m_iPostStimSamples + (2 * t_nSamplesPerBuf)))
            {
                //
                // Average
                //
                float ratio = (float)m_iPreStimSamples/((float)(m_iPreStimSamples+m_iPostStimSamples));
                qint32 t_iBuffWithStim = t_qListRawMatBuf.size()*ratio;

                if(t_iBuffWithStim > 0 && t_qListRawMatBuf[t_iBuffWithStim].first.size() != 0)
                {
                    for(i = 0; i < t_qListRawMatBuf[t_iBuffWithStim].first.size(); ++i)
                    {
                        if(!t_qListRawMatBuf[t_iBuffWithStim-1].first.contains(t_qListRawMatBuf[t_iBuffWithStim].first[i]))//make sure that previous buffer does not conatin this stim - prevent multiple detection
                        {
                            qint32 t_iStimIndex = t_qListRawMatBuf[t_iBuffWithStim].first[i];

                            //
                            // assemble prestimulus
                            //
                            this->assemblePreStimulus(t_qListRawMatBuf, t_iStimIndex);

                            //
                            // assemble poststimulus
                            //
                            this->assemblePostStimulus(t_qListRawMatBuf, t_iStimIndex);

                            qDebug() << m_pFiffInfo->ch_names[m_qListStimChannelIdcs[t_iStimIndex]] << ": Buffer Size" << m_qListQListPreStimBuf[t_iStimIndex].size();
                            //
                            // Prestimulus average
                            //
                            if(m_qListQListPreStimBuf[t_iStimIndex].size() >= m_iNumAverages)
                            {
                                while(m_qListQListPreStimBuf[t_iStimIndex].size() >= (m_iNumAverages+1))//if meanwhile number of averages was reduced
                                    m_qListQListPreStimBuf[t_iStimIndex].pop_front();

                                m_qListPreStimAve[t_iStimIndex] = m_qListQListPreStimBuf[t_iStimIndex][0];
                                for(j = 1; j < m_qListQListPreStimBuf[t_iStimIndex].size(); ++j)
                                    m_qListPreStimAve[t_iStimIndex] += m_qListQListPreStimBuf[t_iStimIndex][j];

                                m_qListPreStimAve[t_iStimIndex].array() /= (double)m_iNumAverages;

                                m_qListQListPreStimBuf[t_iStimIndex].pop_front();
                            }

                            //
                            // Poststimulus average
                            //
                            if(m_qListQListPostStimBuf[t_iStimIndex].size() >= m_iNumAverages)
                            {
                                while(m_qListQListPostStimBuf[t_iStimIndex].size() >= (m_iNumAverages+1))//if meanwhile number of averages was reduced
                                    m_qListQListPostStimBuf[t_iStimIndex].pop_front();

                                m_qListPostStimAve[t_iStimIndex] = m_qListQListPostStimBuf[t_iStimIndex][0];
                                for(j = 1; j < m_qListQListPostStimBuf[t_iStimIndex].size(); ++j)
                                    m_qListPostStimAve[t_iStimIndex] += m_qListQListPostStimBuf[t_iStimIndex][j];

                                m_qListPostStimAve[t_iStimIndex].array() /= (double)m_iNumAverages;

                                m_qListQListPostStimBuf[t_iStimIndex].pop_front();
                            }

                            //if averages are available -> buffers are filled and first average is stored
                            if(m_qListPreStimAve[t_iStimIndex].size() > 0)
                            {
                                //
                                // concatenate pre + post stimulus to full stimulus
                                //
                                m_qListStimAve[t_iStimIndex].resize(m_qListPreStimAve[t_iStimIndex].rows(), m_qListPreStimAve[t_iStimIndex].cols() + m_qListPostStimAve[t_iStimIndex].cols());
                                // Pre
                                m_qListStimAve[t_iStimIndex].block(0,0,m_qListPreStimAve[t_iStimIndex].rows(),m_qListPreStimAve[t_iStimIndex].cols()) = m_qListPreStimAve[t_iStimIndex];
                                // Post
                                m_qListStimAve[t_iStimIndex].block(0,m_qListPreStimAve[t_iStimIndex].cols(),m_qListPostStimAve[t_iStimIndex].rows(),m_qListPostStimAve[t_iStimIndex].cols()) = m_qListPostStimAve[t_iStimIndex];

                                //
                                // Emit evoked
                                //
                                QString t_sStimChName = m_pFiffInfo->ch_names[m_qListStimChannelIdcs[t_iStimIndex]];
                                FiffEvoked::SPtr t_pEvokedPreStim(new FiffEvoked(t_preStimEvoked));
                                t_pEvokedPreStim->nave = m_iNumAverages;
                                t_pEvokedPreStim->comment = t_sStimChName;
                                t_pEvokedPreStim->data = m_qListPreStimAve[t_iStimIndex];
                                emit evokedPreStim(t_pEvokedPreStim);

                                FiffEvoked::SPtr t_pEvokedPostStim(new FiffEvoked(t_postStimEvoked));
                                t_pEvokedPostStim->nave = m_iNumAverages;
                                t_pEvokedPostStim->comment = t_sStimChName;
                                t_pEvokedPostStim->data = m_qListPostStimAve[t_iStimIndex];
                                emit evokedPostStim(t_pEvokedPostStim);

                                FiffEvoked::SPtr t_pEvokedStim(new FiffEvoked(t_stimEvoked));
                                t_pEvokedStim->nave = m_iNumAverages;
                                t_pEvokedStim->comment = t_sStimChName;
                                t_pEvokedStim->data = m_qListStimAve[t_iStimIndex];
                                emit evokedStim(t_pEvokedStim);
                            }
                        }
                    }
                }


                //
                //dump oldest buffer
                //
                t_qListRawMatBuf.pop_front();
            }
        }
    }
}
