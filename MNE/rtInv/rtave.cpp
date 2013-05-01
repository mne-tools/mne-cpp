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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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

RtAve::RtAve(quint32 p_iPreStimSamples, quint32 p_iPostStimSamples, FiffInfo::SPtr p_pFiffInfo, QObject *parent)
: QThread(parent)
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
    stop();
}


//*************************************************************************************************************

void RtAve::append(const MatrixXd &p_DataSegment)
{
//    if(m_pRawMatrixBuffer) // ToDo handle change buffersize

    if(!m_pRawMatrixBuffer)
        m_pRawMatrixBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(128, p_DataSegment.rows(), p_DataSegment.cols()));

    m_pRawMatrixBuffer->push(&p_DataSegment);
}


//*************************************************************************************************************

bool RtAve::stop()
{
    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void RtAve::run()
{
    m_bIsRunning = true;


    //
    //
    //
    quint32 t_nSamplesPerBuf = 0;
    QList<QPair<QList<qint32>, MatrixXd> > t_qListRawMatBuf;

    FiffEvoked::SPtr evoked(new FiffEvoked());
    VectorXd mu;
    qint32 i = 0;

    //get stim channels
    m_qListStimChannelIdcs.clear();
    for(i = 0; i < m_pFiffInfo->nchan; ++i)
        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH && (m_pFiffInfo->chs[i].ch_name != QString("STI 014")))
            m_qListStimChannelIdcs.append(i);

    qint32 count = 0;

    //Enter the main loop
    while(m_bIsRunning)
    {
        if(m_pRawMatrixBuffer)
        {
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
                    t_qListStimuli.append(idx);
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
//                qDebug() << t_qListRawMatBuf.size()/2;
//                qDebug() << (float)(m_iPreStimSamples + t_nSamplesPerBuf)/((float)t_nSamplesPerBuf);
                qint32 t_iMidIdx = t_qListRawMatBuf.size()/2;

                if(t_iMidIdx > 0 && t_qListRawMatBuf[t_iMidIdx].first.size() != 0)
                {
                    for(i = 0; i < t_qListRawMatBuf[t_iMidIdx].first.size(); ++i)
                    {
                        if(!t_qListRawMatBuf[t_iMidIdx-1].first.contains(t_qListRawMatBuf[t_iMidIdx].first[i]))//make sure that previous buffer does not conatin this stim - prevent multiple detection
                        {
                            std::cout << "Count: " << count << std::endl;

                            std::cout << t_qListRawMatBuf[t_iMidIdx].first[i] << " " << t_qListRawMatBuf[t_iMidIdx].second.row(t_qListRawMatBuf[t_iMidIdx].first[i]) << std::endl;


                            qint32 pos = 0;
                            t_qListRawMatBuf[t_iMidIdx].second.row(t_qListRawMatBuf[t_iMidIdx].first[i]).maxCoeff(&pos);
                            std::cout << "Position: " << pos << std::endl;

                        }
                    }
                }


                //
                //dump oldest buffer
                //
                t_qListRawMatBuf.pop_front();
            }






//            emit evokedCalculated(evoked);
        }
    }
}
