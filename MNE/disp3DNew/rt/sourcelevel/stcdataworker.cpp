//=============================================================================================================
/**
* @file     stcdataworker.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the StcDataWorker class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "stcdataworker.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

StcDataWorker::StcDataWorker(QObject* parent)
: QThread(parent)
, m_bIsRunning(false)
, m_bIsLooping(true)
, m_iAverageSamples(1)
, m_iCurrentSample(0)
, m_iMSecIntervall(1000)
, m_sColormap("Hot")
{
}


//*************************************************************************************************************

StcDataWorker::~StcDataWorker()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

void StcDataWorker::addData(const MatrixXd& data, const QString& sColormap)
{
    QMutexLocker locker(&m_qMutex);
    if(data.size() == 0)
        return;

    m_matData = data;
    m_sColormap = sColormap;

    //TODO: Transform to colors here!
    m_lDataColor = transformDataToColor(data, m_sColormap);
}


//*************************************************************************************************************

void StcDataWorker::clear()
{
    QMutexLocker locker(&m_qMutex);
}


//*************************************************************************************************************

void StcDataWorker::run()
{
    VectorXd t_vecAverage(0,0);

    m_bIsRunning = true;

    while(true)
    {
        QTime timer;
        timer.start();

        //std::cout<<"StcDataWorker Running ... "<<std::endl;
        {
            QMutexLocker locker(&m_qMutex);
            if(!m_bIsRunning)
                break;
        }

        bool doProcessing = false;
        {
            QMutexLocker locker(&m_qMutex);
            if(m_matData.cols() > 0)
                doProcessing = true;
        }

        if(doProcessing)
        {
            if(m_bIsLooping)
            {
                m_qMutex.lock();
                //Down sampling in loop mode
//                std::cout<<"StcDataWorker::run() - m_matData.size(): "<<m_matData.size()<<std::endl;
//                std::cout<<"StcDataWorker::run() - m_iCurrentSample: "<<m_iCurrentSample<<std::endl;

                if(t_vecAverage.rows() != m_matData.rows())
                    t_vecAverage = m_matData.col(m_iCurrentSample%m_matData.cols());
                else
                    t_vecAverage += m_matData.col(m_iCurrentSample%m_matData.cols());
                m_qMutex.unlock();
            }
            else
            {
                m_qMutex.lock();
                //Down sampling in stream mode
                if(t_vecAverage.rows() != m_matData.rows())
                    t_vecAverage = m_matData.col(0);
                else
                    t_vecAverage += m_matData.col(0);

                m_matData = m_matData.block(0,1,m_matData.rows(),m_matData.cols()-1);
                m_qMutex.unlock();
            }

            m_qMutex.lock();
            m_iCurrentSample+=1;

            if((m_iCurrentSample/1)%m_iAverageSamples == 0)
            {
                t_vecAverage /= (double)m_iAverageSamples;

                emit stcSample(t_vecAverage);
                t_vecAverage = VectorXd::Zero(t_vecAverage.rows());
            }
            m_qMutex.unlock();
        }

        //qDebug()<<"StcDataWorker::run()"<<timer.elapsed()<<"msecs";
        QThread::msleep(m_iMSecIntervall);
    }
}


//*************************************************************************************************************

QList<MatrixX3f> StcDataWorker::transformDataToColor(const MatrixXd& data, const QString& sColormap)
{
    QList<MatrixX3f> matList;

    return matList;
}


//*************************************************************************************************************

void StcDataWorker::setAverage(qint32 samples)
{
    QMutexLocker locker(&m_qMutex);
    m_iAverageSamples = samples;
}


//*************************************************************************************************************

void StcDataWorker::setInterval(int usec)
{
    QMutexLocker locker(&m_qMutex);
    m_iMSecIntervall = usec;
}


//*************************************************************************************************************

void StcDataWorker::setLoop(bool looping)
{
    QMutexLocker locker(&m_qMutex);
    m_bIsLooping = looping;
}


//*************************************************************************************************************

void StcDataWorker::stop()
{
    m_qMutex.lock();
    m_bIsRunning = false;
    m_qMutex.unlock();

    QThread::wait();
}


//*************************************************************************************************************

void StcDataWorker::start()
{
    m_qMutex.lock();
    m_iCurrentSample = 0;
    m_qMutex.unlock();

    QThread::start();
}
