//=============================================================================================================
/**
* @file     rtdataworker.cpp
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
* @brief    Implementation of the RtDataWorker class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtdataworker.h"


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

RtDataWorker::RtDataWorker(QObject* parent)
: QThread(parent)
, m_bIsRunning(false)
, m_bIsLooping(true)
, m_iAverageSamples(1)
, m_iCurrentSample(0)
, m_iMSecIntervall(1000)
, m_sColormap("Hot Negative 2")
, m_dNormalization(1.0)
, m_dNormalizationMax(10.0)
{
}


//*************************************************************************************************************

RtDataWorker::~RtDataWorker()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

void RtDataWorker::addData(const MatrixXd& data)
{
    QMutexLocker locker(&m_qMutex);
    if(data.size() == 0)
        return;

    m_matData = data;
}


//*************************************************************************************************************

void RtDataWorker::clear()
{
    QMutexLocker locker(&m_qMutex);
}


//*************************************************************************************************************

void RtDataWorker::setAverage(qint32 samples)
{
    QMutexLocker locker(&m_qMutex);
    m_iAverageSamples = samples;
}


//*************************************************************************************************************

void RtDataWorker::setInterval(const int& iMSec)
{
    QMutexLocker locker(&m_qMutex);
    m_iMSecIntervall = iMSec;
}


//*************************************************************************************************************

void RtDataWorker::setColormapType(const QString& sColormapType)
{
    QMutexLocker locker(&m_qMutex);
    m_sColormap = sColormapType;
}


//*************************************************************************************************************

void RtDataWorker::setNormalization(const double& dValue)
{
    QMutexLocker locker(&m_qMutex);
    m_dNormalization = (m_dNormalizationMax/100.0) * dValue;
}


//*************************************************************************************************************

void RtDataWorker::setLoop(bool looping)
{
    QMutexLocker locker(&m_qMutex);
    m_bIsLooping = looping;
}


//*************************************************************************************************************

void RtDataWorker::start()
{
    m_qMutex.lock();
    m_iCurrentSample = 0;
    m_qMutex.unlock();

    QThread::start();
}


//*************************************************************************************************************

void RtDataWorker::stop()
{
    m_qMutex.lock();
    m_bIsRunning = false;
    m_qMutex.unlock();

    QThread::wait();
}


//*************************************************************************************************************

void RtDataWorker::run()
{
    VectorXd t_vecAverage(0,0);

    m_bIsRunning = true;

    while(true)
    {
//        QTime timer;
//        timer.start();

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
            m_iCurrentSample++;

            if((m_iCurrentSample/1)%m_iAverageSamples == 0)
            {
                t_vecAverage /= (double)m_iAverageSamples;

                emit stcSample(transformDataToColor(t_vecAverage));
                t_vecAverage = VectorXd::Zero(t_vecAverage.rows());
            }
            m_qMutex.unlock();
        }

        //qDebug()<<"RtDataWorker::run()"<<timer.elapsed()<<"msecs";
        QThread::msleep(m_iMSecIntervall);
    }
}


//*************************************************************************************************************

QByteArray RtDataWorker::transformDataToColor(const VectorXd& data)
{
    //Note: This function needs to be implemented extremley efficient
    QByteArray arrayColor;
    int idxColor = 0;

    if(m_sColormap == "Hot Negative 1") {
        arrayColor.resize(data.rows() * 3 * (int)sizeof(float));
        float *rawArrayColors = reinterpret_cast<float *>(arrayColor.data());

        for(int r = 0; r<data.rows(); r++) {
            double dSample = data(r)/m_dNormalization;
            qint32 iVal = dSample > 255 ? 255 : dSample < 0 ? 0 : dSample;

            QRgb qRgb;
            qRgb = ColorMap::valueToHotNegative1((float)iVal/255.0);

            QColor colSample(qRgb);
            rawArrayColors[idxColor++] = colSample.redF();
            rawArrayColors[idxColor++] = colSample.greenF();
            rawArrayColors[idxColor++] = colSample.blueF();
        }

        return arrayColor;
    }

    if(m_sColormap == "Hot Negative 2") {
        arrayColor.resize(data.rows() * 3 * (int)sizeof(float));
        float *rawArrayColors = reinterpret_cast<float *>(arrayColor.data());

        for(int r = 0; r<data.rows(); r++) {
            double dSample = data(r)/m_dNormalization;
            qint32 iVal = dSample > 255 ? 255 : dSample < 0 ? 0 : dSample;

            QRgb qRgb;
            qRgb = ColorMap::valueToHotNegative2((float)iVal/255.0);

            QColor colSample(qRgb);
            rawArrayColors[idxColor++] = colSample.redF();
            rawArrayColors[idxColor++] = colSample.greenF();
            rawArrayColors[idxColor++] = colSample.blueF();
        }

        return arrayColor;
    }

    if(m_sColormap == "Hot") {
        arrayColor.resize(data.rows() * 3 * (int)sizeof(float));
        float *rawArrayColors = reinterpret_cast<float *>(arrayColor.data());

        for(int r = 0; r<data.rows(); r++) {
            double dSample = data(r)/m_dNormalization;
            qint32 iVal = dSample > 255 ? 255 : dSample < 0 ? 0 : dSample;

            QRgb qRgb;
            qRgb = ColorMap::valueToHot((float)iVal/255.0);

            QColor colSample(qRgb);
            rawArrayColors[idxColor++] = colSample.redF();
            rawArrayColors[idxColor++] = colSample.greenF();
            rawArrayColors[idxColor++] = colSample.blueF();
        }

        return arrayColor;
    }

    return arrayColor;
}
