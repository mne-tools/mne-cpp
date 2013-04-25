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

#include "rtcov.h"

#include <iostream>
#include <fiff/fiff_cov.h>


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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtCov::RtCov(qint32 p_iMaxSamples, FiffInfo::SPtr p_pFiffInfo, QObject *parent)
: QThread(parent)
, m_iMaxSamples(p_iMaxSamples)
, m_pFiffInfo(p_pFiffInfo)
, m_bIsRunning(false)
, m_bIsRawBufferInit(false)
, m_pRawMatrixBuffer(NULL)
{

}


//*************************************************************************************************************

RtCov::~RtCov()
{
    stop();
    if(m_pRawMatrixBuffer)
        delete m_pRawMatrixBuffer;
}


//*************************************************************************************************************

void RtCov::append(const MatrixXf &p_DataSegment)
{

}


//*************************************************************************************************************

void RtCov::receiveDataSegment(MatrixXf p_DataSegment)
{
//    if(m_pRawMatrixBuffer) // ToDo handle change buffersize

    if(!m_pRawMatrixBuffer)
    {
        mutex.lock();
        m_pRawMatrixBuffer = new RawMatrixBuffer(10, p_DataSegment.rows(), p_DataSegment.cols());

        m_bIsRawBufferInit = true;
        mutex.unlock();
    }

    m_pRawMatrixBuffer->push(&p_DataSegment);
}


//*************************************************************************************************************

bool RtCov::stop()
{
    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void RtCov::run()
{
    m_bIsRunning = true;


    quint32 n_samples = 0;

    FiffCov cov;
    VectorXd mu;

    while(m_bIsRunning)
    {
        if(m_bIsRawBufferInit)
        {
            MatrixXd rawSegment = m_pRawMatrixBuffer->pop().cast<double>();

            if(n_samples == 0)
            {
                mu = rawSegment.rowwise().sum();
                cov.data = rawSegment * rawSegment.transpose();
            }
            else
            {
                mu.array() += rawSegment.rowwise().sum().array();
                cov.data += rawSegment * rawSegment.transpose();
            }
            n_samples += rawSegment.cols();

            if(n_samples > m_iMaxSamples)
            {
                mu /= (float)n_samples;
                cov.data.array() -= n_samples * (mu * mu.transpose()).array();
                cov.data.array() /= (n_samples - 1);

                std::cout << "Covariance:\n" << cov.data.block(0,0,10,10) << std::endl;

                cov.kind = FIFFV_MNE_NOISE_COV;
                cov.diag = false;
                cov.dim = cov.data.rows();


                //ToDo do picks
                cov.names = m_pFiffInfo->ch_names;
                cov.projs = m_pFiffInfo->projs;
                cov.bads  = m_pFiffInfo->bads;
                cov.nfree  = n_samples;

                emit covCalculated(cov);

                cov.clear();
                n_samples = 0;
            }


//            qint32 samples = rawSegment.cols();
//            VectorXf mu = rawSegment.rowwise().sum().array() / (float)samples;

//            MatrixXf noise_covariance = rawSegment * rawSegment.transpose();// noise_covariance == raw_covariance
//            noise_covariance.array() -= samples * (mu * mu.transpose()).array();
//            noise_covariance.array() /= (samples - 1);

//            std::cout << "Noise Covariance:\n" << noise_covariance.block(0,0,10,10) << std::endl;

//            printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());

        }
    }
}
