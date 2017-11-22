//=============================================================================================================
/**
* @file     rtgpusensordataworker.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    RtGpuSensorDataWorker class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtgpusensordataworker.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTime>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtGpuSensorDataWorker::RtGpuSensorDataWorker(QObject *parent)
    : QThread(parent)
    , m_bIsRunning(false)
    , m_bIsLooping(true)
    , m_iAverageSamples(1)
    , m_iMSecIntervall(16)
    , m_dSFreq(1000.0)
{
}


//*************************************************************************************************************

RtGpuSensorDataWorker::~RtGpuSensorDataWorker()
{
    if(this->isRunning()) {
        stop();
    }
}


//*************************************************************************************************************

void RtGpuSensorDataWorker::addData(const Eigen::MatrixXf &tData)
{
    QMutexLocker locker(&m_qMutex);
    if(tData.rows() == 0) {
        return;
    }

    //Transform from matrix to list for easier handling in non loop mode
    for(int i = 0; i < tData.cols(); i++) {
        if(m_lDataQ.size() < m_dSFreq) {
            m_lDataQ.push_back(tData.col(i));
        } else {
            qDebug() <<"RtGpuSensorDataWorker::addData - worker is full!";
            break;
        }
    }
}


//*************************************************************************************************************

void RtGpuSensorDataWorker::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_lDataQ.clear();
}


//*************************************************************************************************************

void RtGpuSensorDataWorker::setNumberAverages(const uint tNumAvr)
{
    QMutexLocker locker(&m_qMutex);
    m_iAverageSamples = tNumAvr;
}


//*************************************************************************************************************

void RtGpuSensorDataWorker::setInterval(const uint tMSec)
{
    QMutexLocker locker(&m_qMutex);
    m_iMSecIntervall = tMSec;
}


//*************************************************************************************************************

void RtGpuSensorDataWorker::setLoop(const bool tLooping)
{
    QMutexLocker locker(&m_qMutex);
    m_bIsLooping = tLooping;
}


//*************************************************************************************************************

void RtGpuSensorDataWorker::setSFreq(const double tSFreq)
{
    QMutexLocker locker(&m_qMutex);
    m_dSFreq = tSFreq;
}


//*************************************************************************************************************

void RtGpuSensorDataWorker::stop()
{
    m_qMutex.lock();
    m_bIsRunning = false;
    m_qMutex.unlock();

    QThread::wait();
}


//*************************************************************************************************************

void RtGpuSensorDataWorker::start()
{
    m_qMutex.lock();
    m_itCurrentSample = m_lDataQ.cbegin();
    m_qMutex.unlock();
    QThread::start();
}


//*************************************************************************************************************

void RtGpuSensorDataWorker::run()
{
    VectorXf vecAverage;
    uint iAverageCtr = 0;

    m_bIsRunning = true;
    QTime timer;

    while(true) {
        timer.start();

        {
            QMutexLocker locker(&m_qMutex);
            if(!m_bIsRunning)
            {
                break;
            }
        }

        bool doProcessing = false;

        {
            QMutexLocker locker(&m_qMutex);
            if(!m_lDataQ.empty())
            {
                doProcessing = true;
            }
        }

        if(doProcessing) {
            if(m_bIsLooping) {
                m_qMutex.lock();

                //Down sampling in loop mode
                if(vecAverage.rows() != m_lDataQ.front().rows()) {
                    vecAverage = *m_itCurrentSample;
                } else {
                    vecAverage += *m_itCurrentSample;
                }

                m_qMutex.unlock();
            } else {
                m_qMutex.lock();

                //Down sampling in stream mode
                if(vecAverage.rows() != m_lDataQ.front().rows()) {
                    vecAverage = m_lDataQ.front();
                } else {
                    vecAverage += m_lDataQ.front();
                }

                m_lDataQ.pop_front();

                m_qMutex.unlock();
            }

            m_qMutex.lock();

            iAverageCtr++;
            m_itCurrentSample++;

            if(m_itCurrentSample == m_lDataQ.cend())
            {
                m_itCurrentSample = m_lDataQ.cbegin();
            }

            if(iAverageCtr % m_iAverageSamples == 0) {
                //Perform the actual interpolation and send signal
                vecAverage /= static_cast<float>(m_iAverageSamples);
                emit newRtData(vecAverage);
                vecAverage.setZero(vecAverage.rows());
                iAverageCtr = 0;

                //Sleep specified amount of time
                const uint iTimeLeft = m_iMSecIntervall - timer.elapsed();

                if(iTimeLeft > 0) {
                    QThread::msleep(iTimeLeft);
                }
            }

            m_qMutex.unlock();
        }
    }
}


//*************************************************************************************************************
