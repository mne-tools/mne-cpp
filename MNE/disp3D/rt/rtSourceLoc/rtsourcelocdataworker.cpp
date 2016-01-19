//=============================================================================================================
/**
* @file     rtsourcelocdataworker.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     December, 2015
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
* @brief    Implementation of the RtSourceLocDataWorker class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourcelocdataworker.h"
#include "fs/label.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceLocDataWorker::RtSourceLocDataWorker(QObject* parent)
: QThread(parent)
, m_bIsRunning(false)
, m_bIsLooping(true)
, m_iAverageSamples(1)
, m_iCurrentSample(0)
, m_iVisualizationType(BrainRTDataVisualizationTypes::VertexBased)
, m_iMSecIntervall(1000)
, m_sColormap("Hot Negative 2")
, m_dNormalization(1.0)
, m_dNormalizationMax(10.0)
, m_bSurfaceDataIsInit(false)
, m_bAnnotationDataIsInit(false)
{
}


//*************************************************************************************************************

RtSourceLocDataWorker::~RtSourceLocDataWorker()
{
    if(this->isRunning()) {
        stop();
    }
}


//*************************************************************************************************************

void RtSourceLocDataWorker::addData(const MatrixXd& data)
{
    QMutexLocker locker(&m_qMutex);
    if(data.rows() == 0)
        return;

    //Transform from matrix to list for easier handling in non loop mode
    for(int i = 0; i<data.cols(); i++) {
        m_lData.append(data.col(i));
    }
}


//*************************************************************************************************************

void RtSourceLocDataWorker::clear()
{
    QMutexLocker locker(&m_qMutex);
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setSurfaceData(const QByteArray& arraySurfaceVertColor, const VectorXi& vecVertNo)
{
    QMutexLocker locker(&m_qMutex);

    if(arraySurfaceVertColor.size() == 0 || vecVertNo.rows() == 0) {
        qDebug()<<"RtSourceLocDataWorker::setSurfaceData - Surface data is empty. Returning ...";
        return;
    }

    m_arraySurfaceVertColor = arraySurfaceVertColor;
    m_vecVertNo = vecVertNo;

    m_bSurfaceDataIsInit = true;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setAnnotationData(const VectorXi& vecLabelIds, const QList<FSLIB::Label>& lLabels)
{
    QMutexLocker locker(&m_qMutex);

    if(vecLabelIds.rows() == 0 || lLabels.isEmpty()) {
        qDebug()<<"RtSourceLocDataWorker::setAnnotationData - Annotation data is empty. Returning ...";
        return;
    }

    m_lLabels = lLabels;

    //Generate fast lookup map for each source and corresponding label
    for(qint32 i = 0; i < m_vecVertNo.rows(); ++i)
        m_mapLabelIdSources.insert(m_vecVertNo(i), vecLabelIds(m_vecVertNo(i)));

    m_bAnnotationDataIsInit = true;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setNumberAverages(const int &iNumAvr)
{
    QMutexLocker locker(&m_qMutex);
    m_iAverageSamples = iNumAvr;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setInterval(const int& iMSec)
{
    QMutexLocker locker(&m_qMutex);
    m_iMSecIntervall = iMSec;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setVisualizationType(const int& iVisType)
{
    QMutexLocker locker(&m_qMutex);
    m_iVisualizationType = iVisType;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setColormapType(const QString& sColormapType)
{
    QMutexLocker locker(&m_qMutex);
    m_sColormap = sColormapType;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setNormalization(const double& dValue)
{
    QMutexLocker locker(&m_qMutex);
    m_dNormalization = (m_dNormalizationMax/100.0) * dValue;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setLoop(bool looping)
{
    QMutexLocker locker(&m_qMutex);
    m_bIsLooping = looping;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::start()
{
    m_qMutex.lock();
    m_iCurrentSample = 0;
    m_qMutex.unlock();

    QThread::start();
}


//*************************************************************************************************************

void RtSourceLocDataWorker::stop()
{
    m_qMutex.lock();
    m_bIsRunning = false;
    m_qMutex.unlock();

    QThread::wait();
}


//*************************************************************************************************************

void RtSourceLocDataWorker::run()
{
    VectorXd t_vecAverage(0,0);

    m_bIsRunning = true;

    while(true) {
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
            if(!m_lData.isEmpty() && m_lData.size() > 0)
                doProcessing = true;
        }

        if(doProcessing) {
            if(m_bIsLooping) {
                m_qMutex.lock();

                //Down sampling in loop mode
                if(t_vecAverage.rows() != m_lData[0].rows()) {
                    t_vecAverage = m_lData[m_iCurrentSample%m_lData.size()];
                } else {
                    t_vecAverage += m_lData[m_iCurrentSample%m_lData.size()];
                }

                m_qMutex.unlock();
            } else {
                m_qMutex.lock();

                //Down sampling in stream mode
                if(t_vecAverage.rows() != m_lData[0].rows()) {
                    t_vecAverage = m_lData.front();
                } else {
                    t_vecAverage += m_lData.front();
                }

                m_lData.pop_front();

                m_qMutex.unlock();
            }

            m_qMutex.lock();
            m_iCurrentSample++;

            if((m_iCurrentSample/1)%m_iAverageSamples == 0) {
                t_vecAverage /= (double)m_iAverageSamples;

                emit newRtData(performVisualizationTypeCalculation(t_vecAverage));
                t_vecAverage = VectorXd::Zero(t_vecAverage.rows());
            }

            m_qMutex.unlock();
        }

//        qDebug()<<"RtSourceLocDataWorker::run()"<<timer.elapsed()<<"msecs";
        QThread::msleep(m_iMSecIntervall);
    }
}


//*************************************************************************************************************

QByteArray RtSourceLocDataWorker::performVisualizationTypeCalculation(const VectorXd& sourceColorSamples)
{
    //NOTE: This function is called for every new sample point and therefore must be kept highly efficient!
    if(sourceColorSamples.rows() != m_vecVertNo.rows()) {
        qDebug()<<"RtSourceLocDataWorker::performVisualizationTypeCalculation - number of rows in sample ("<<sourceColorSamples.rows()<<") do not not match with idx/no number of rows in vertex ("<<m_vecVertNo.rows()<<"). Returning...";
        return QByteArray();
    }

    //Generate color data for vertices
    switch(m_iVisualizationType) {
        case BrainRTDataVisualizationTypes::VertexBased: {
            if(!m_bSurfaceDataIsInit) {
                qDebug()<<"RtSourceLocDataWorker::performVisualizationTypeCalculation - Surface data was not initialized. Returning ...";
                return m_arraySurfaceVertColor;
            }

            QByteArray arrayCurrentVertColor = m_arraySurfaceVertColor;

            //Create final QByteArray with colors based on the current anatomical information
            QByteArray sourceColorSamplesColor = transformDataToColor(sourceColorSamples);
            const float *rawSourceColorSamplesColor = reinterpret_cast<const float *>(sourceColorSamplesColor.data());
            int idxSourceColorSamples = 0;
            float *rawArrayCurrentVertColor = reinterpret_cast<float *>(arrayCurrentVertColor.data());

            for(int i = 0; i<m_vecVertNo.rows(); i++) {
                rawArrayCurrentVertColor[m_vecVertNo(i)*3+0] = rawSourceColorSamplesColor[idxSourceColorSamples++];
                rawArrayCurrentVertColor[m_vecVertNo(i)*3+1] = rawSourceColorSamplesColor[idxSourceColorSamples++];
                rawArrayCurrentVertColor[m_vecVertNo(i)*3+2] = rawSourceColorSamplesColor[idxSourceColorSamples++];
            }

            return arrayCurrentVertColor;
        }

        case BrainRTDataVisualizationTypes::AnnotationBased: {
            if(!m_bAnnotationDataIsInit) {
                qDebug()<<"RtSourceLocDataWorker::performVisualizationTypeCalculation - Annotation data was not initialized. Returning ...";
                return m_arraySurfaceVertColor;
            }

            //Find maximum actiavtion for each label
            QMap<qint32, double> vecLabelActivation;

            for(int i = 0; i<m_vecVertNo.rows(); i++) {
                //Find out label for source
                qint32 labelIdx = m_mapLabelIdSources[m_vecVertNo(i)];

                if(abs(sourceColorSamples(i)) > abs(vecLabelActivation[labelIdx]))
                    vecLabelActivation.insert(labelIdx, sourceColorSamples(i));
            }      

            //Color all labels respectivley to their activation
            QByteArray arrayCurrentVertColor;
            arrayCurrentVertColor.resize(m_arraySurfaceVertColor.size());

            float *rawArrayCurrentVertColor = reinterpret_cast<float *>(arrayCurrentVertColor.data());

            for(int i = 0; i<m_lLabels.size(); i++) {
                FSLIB::Label label = m_lLabels.at(i);

                //Transform label activations to rgb colors
                VectorXd vecActivation(1);
                vecActivation(0) = vecLabelActivation[label.label_id];

                QByteArray arrayLabelColors = transformDataToColor(vecActivation);
                float *rawArrayLabelColors = reinterpret_cast<float *>(arrayLabelColors.data());

                for(int j = 0; j<label.vertices.rows(); j++) {
                    rawArrayCurrentVertColor[label.vertices(j)*3+0] = rawArrayLabelColors[0];
                    rawArrayCurrentVertColor[label.vertices(j)*3+1] = rawArrayLabelColors[1];
                    rawArrayCurrentVertColor[label.vertices(j)*3+2] = rawArrayLabelColors[2];
                }
            }

            return arrayCurrentVertColor;
        }        

        case BrainRTDataVisualizationTypes::SmoothingBased: {
            //TODO: Smooth here!
            break;
        }
    }

    return m_arraySurfaceVertColor;
}


//*************************************************************************************************************

QByteArray RtSourceLocDataWorker::transformDataToColor(const VectorXd& data)
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
