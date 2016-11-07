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
#include "../../3DObjects/common/types.h"

#include <disp/helpers/colormap.h>
#include <fs/label.h>
#include <fs/annotation.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include <QTime>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace DISPLIB;
using namespace FSLIB;


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
, m_iVisualizationType(Data3DTreeModelItemRoles::VertexBased)
, m_iMSecIntervall(50)
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

void RtSourceLocDataWorker::setSurfaceData(const QByteArray& arraySurfaceVertColorLeftHemi,
                                           const QByteArray& arraySurfaceVertColorRightHemi,
                                           const Eigen::VectorXi& vecVertNoLeftHemi,
                                           const Eigen::VectorXi& vecVertNoRightHemi)
{
    QMutexLocker locker(&m_qMutex);

    if(arraySurfaceVertColorLeftHemi.size() == 0 || vecVertNoLeftHemi.rows() == 0 || arraySurfaceVertColorRightHemi.size() == 0 || vecVertNoRightHemi.rows() == 0) {
        qDebug() << "RtSourceLocDataWorker::setSurfaceData - Surface data is empty. Returning ...";
        return;
    }

    m_arraySurfaceVertColorLeftHemi = arraySurfaceVertColorLeftHemi;
    m_vecVertNoLeftHemi = vecVertNoLeftHemi;

    m_arraySurfaceVertColorRightHemi = arraySurfaceVertColorRightHemi;
    m_vecVertNoRightHemi = vecVertNoRightHemi;

    m_bSurfaceDataIsInit = true;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setAnnotationData(const Eigen::VectorXi& vecLabelIdsLeftHemi,
                                              const Eigen::VectorXi& vecLabelIdsRightHemi,
                                              const QList<FSLIB::Label>& lLabelsLeftHemi,
                                              const QList<FSLIB::Label>& lLabelsRightHemi)
{
    QMutexLocker locker(&m_qMutex);

    if(vecLabelIdsLeftHemi.rows() == 0 || lLabelsLeftHemi.isEmpty() || vecLabelIdsRightHemi.rows() == 0 || lLabelsRightHemi.isEmpty()) {
        qDebug() << "RtSourceLocDataWorker::setAnnotationData - Annotation data is empty. Returning ...";
        return;
    }

    m_lLabelsLeftHemi = lLabelsLeftHemi;
    m_lLabelsRightHemi = lLabelsRightHemi;

    //Generate fast lookup map for each source and corresponding label
    for(qint32 i = 0; i < m_vecVertNoLeftHemi.rows(); ++i) {
        m_mapLabelIdSourcesLeftHemi.insert(m_vecVertNoLeftHemi(i), vecLabelIdsLeftHemi(m_vecVertNoLeftHemi(i)));
    }

    for(qint32 i = 0; i < m_vecVertNoRightHemi.rows(); ++i) {
        m_mapLabelIdSourcesRightHemi.insert(m_vecVertNoRightHemi(i), vecLabelIdsRightHemi(m_vecVertNoRightHemi(i)));
    }

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

void RtSourceLocDataWorker::setNormalization(const QVector3D& vecThresholds)
{
    QMutexLocker locker(&m_qMutex);
    m_vecThresholds = vecThresholds;

    //m_dNormalization = (m_dNormalizationMax/100.0) * dValue;
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
        QTime timer;
        timer.start();

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

        //qDebug() << "RtSourceLocDataWorker::run()" << timer.elapsed() << "msecs";
        QThread::msleep(m_iMSecIntervall);
    }
}


//*************************************************************************************************************

QPair<QByteArray, QByteArray> RtSourceLocDataWorker::performVisualizationTypeCalculation(const VectorXd& sourceColorSamples)
{
    QPair<QByteArray, QByteArray> colorPair;
    colorPair.first = m_arraySurfaceVertColorLeftHemi;
    colorPair.second = m_arraySurfaceVertColorRightHemi;

    //NOTE: This function is called for every new sample point and therefore must be kept highly efficient!
    if(sourceColorSamples.rows() != m_vecVertNoLeftHemi.rows() + m_vecVertNoRightHemi.rows()) {
        qDebug() << "RtSourceLocDataWorker::performVisualizationTypeCalculation - number of rows in sample (" << sourceColorSamples.rows() << ") do not not match with idx/no number of rows in vertex (" << m_vecVertNoLeftHemi.rows() + m_vecVertNoRightHemi.rows() << "). Returning...";
        return colorPair;
    }

    //Cut out left and right hemisphere from source data
    VectorXd sourceColorSamplesLeftHemi = sourceColorSamples.segment(0,m_vecVertNoLeftHemi.rows());
    VectorXd sourceColorSamplesRightHemi = sourceColorSamples.segment(m_vecVertNoLeftHemi.rows()+1,m_vecVertNoRightHemi.rows());

    //Generate color data for vertices
    switch(m_iVisualizationType) {
        case Data3DTreeModelItemRoles::VertexBased: {
            if(!m_bSurfaceDataIsInit) {
                qDebug() << "RtSourceLocDataWorker::performVisualizationTypeCalculation - Surface data was not initialized. Returning ...";
                return colorPair;
            }

            //Left hemisphere
            QByteArray arrayCurrentVertColorLeftHemi = m_arraySurfaceVertColorLeftHemi;
            float *rawArrayCurrentVertColorLeftHemi = reinterpret_cast<float *>(arrayCurrentVertColorLeftHemi.data());

            //Create final QByteArray with colors based on the current anatomical information
            for(int i = 0; i < m_vecVertNoLeftHemi.rows(); ++i) {
                VectorXd vecActivationLeftHemi(1);
                vecActivationLeftHemi(0) = sourceColorSamplesLeftHemi(i);

                if(vecActivationLeftHemi(0) >= m_vecThresholds.x()) {
                    QByteArray sourceColorSamplesColorLeftHemi = transformDataToColor(vecActivationLeftHemi);
                    const float *rawSourceColorSamplesColorLeftHemi = reinterpret_cast<const float *>(sourceColorSamplesColorLeftHemi.data());

                    rawArrayCurrentVertColorLeftHemi[m_vecVertNoLeftHemi(i)*3+0] = rawSourceColorSamplesColorLeftHemi[0];
                    rawArrayCurrentVertColorLeftHemi[m_vecVertNoLeftHemi(i)*3+1] = rawSourceColorSamplesColorLeftHemi[1];
                    rawArrayCurrentVertColorLeftHemi[m_vecVertNoLeftHemi(i)*3+2] = rawSourceColorSamplesColorLeftHemi[2];
                }
            }

            colorPair.first = arrayCurrentVertColorLeftHemi;

            //Right hemisphere
            QByteArray arrayCurrentVertColorRightHemi = m_arraySurfaceVertColorRightHemi;
            float *rawArrayCurrentVertColorRightHemi = reinterpret_cast<float *>(arrayCurrentVertColorRightHemi.data());

            //Create final QByteArray with colors based on the current anatomical information
            for(int i = 0; i < m_vecVertNoRightHemi.rows(); ++i) {
                VectorXd vecActivationRightHemi(1);
                vecActivationRightHemi(0) = sourceColorSamplesRightHemi(i);

                if(vecActivationRightHemi(0) >= m_vecThresholds.x()) {
                    QByteArray sourceColorSamplesColorRightHemi = transformDataToColor(vecActivationRightHemi);
                    const float *rawSourceColorSamplesColorRightHemi = reinterpret_cast<const float *>(sourceColorSamplesColorRightHemi.data());

                    rawArrayCurrentVertColorRightHemi[m_vecVertNoRightHemi(i)*3+0] = rawSourceColorSamplesColorRightHemi[0];
                    rawArrayCurrentVertColorRightHemi[m_vecVertNoRightHemi(i)*3+1] = rawSourceColorSamplesColorRightHemi[1];
                    rawArrayCurrentVertColorRightHemi[m_vecVertNoRightHemi(i)*3+2] = rawSourceColorSamplesColorRightHemi[2];
                }
            }

            colorPair.second = arrayCurrentVertColorRightHemi;

            return colorPair;
        }

        case Data3DTreeModelItemRoles::AnnotationBased: {
            if(!m_bAnnotationDataIsInit) {
                qDebug() << "RtSourceLocDataWorker::performVisualizationTypeCalculation - Annotation data was not initialized. Returning ...";

                return colorPair;
            }

            //Find maximum actiavtion for each label
            QMap<qint32, double> vecLabelActivationLeftHemi;

            for(int i = 0; i < m_vecVertNoLeftHemi.rows(); ++i) {
                //Find out label for source
                qint32 labelIdxLeftHemi = m_mapLabelIdSourcesLeftHemi[m_vecVertNoLeftHemi(i)];

                if(abs(sourceColorSamplesLeftHemi(i)) > abs(vecLabelActivationLeftHemi[labelIdxLeftHemi]))
                    vecLabelActivationLeftHemi.insert(labelIdxLeftHemi, sourceColorSamplesLeftHemi(i));
            }

            QMap<qint32, double> vecLabelActivationRightHemi;
            for(int i = 0; i < m_vecVertNoRightHemi.rows(); ++i) {
                //Find out label for source
                qint32 labelIdxRightHemi = m_mapLabelIdSourcesRightHemi[m_vecVertNoRightHemi(i)];

                if(abs(sourceColorSamplesRightHemi(i)) > abs(vecLabelActivationRightHemi[labelIdxRightHemi]))
                    vecLabelActivationRightHemi.insert(labelIdxRightHemi, sourceColorSamplesRightHemi(i));
            }

            //Color all labels respectivley to their activation
            //Left hemisphere
            QByteArray arrayCurrentVertColorLeftHemi;
            //arrayCurrentVertColor.resize(m_arraySurfaceVertColor.size());
            arrayCurrentVertColorLeftHemi = m_arraySurfaceVertColorLeftHemi;

            float *rawArrayCurrentVertColorLeftHemi = reinterpret_cast<float *>(arrayCurrentVertColorLeftHemi.data());

            for(int i = 0; i<m_lLabelsLeftHemi.size(); i++) {
                FSLIB::Label labelLeftHemi = m_lLabelsLeftHemi.at(i);

                //Transform label activations to rgb colors
                VectorXd vecActivationLeftHemi(1);
                vecActivationLeftHemi(0) = vecLabelActivationLeftHemi[labelLeftHemi.label_id];

                //Check if value is bigger than lower threshold. If not, don't plot activation
                if(vecActivationLeftHemi(0) >= m_vecThresholds.x()) {
                    QByteArray arrayLabelColorsLeftHemi = transformDataToColor(vecActivationLeftHemi);
                    float *rawArrayLabelColorsLeftHemi = reinterpret_cast<float *>(arrayLabelColorsLeftHemi.data());

                    for(int j = 0; j<labelLeftHemi.vertices.rows(); j++) {
                        rawArrayCurrentVertColorLeftHemi[labelLeftHemi.vertices(j)*3+0] = rawArrayLabelColorsLeftHemi[0];
                        rawArrayCurrentVertColorLeftHemi[labelLeftHemi.vertices(j)*3+1] = rawArrayLabelColorsLeftHemi[1];
                        rawArrayCurrentVertColorLeftHemi[labelLeftHemi.vertices(j)*3+2] = rawArrayLabelColorsLeftHemi[2];
                    }
                }
            }

            colorPair.first = arrayCurrentVertColorLeftHemi;

            //Right hemisphere
            QByteArray arrayCurrentVertColorRightHemi;
            //arrayCurrentVertColor.resize(m_arraySurfaceVertColor.size());
            arrayCurrentVertColorRightHemi = m_arraySurfaceVertColorRightHemi;

            float *rawArrayCurrentVertColorRightHemi = reinterpret_cast<float *>(arrayCurrentVertColorRightHemi.data());

            for(int i = 0; i<m_lLabelsRightHemi.size(); i++) {
                FSLIB::Label labelRightHemi = m_lLabelsRightHemi.at(i);

                //Transform label activations to rgb colors
                VectorXd vecActivationRightHemi(1);
                vecActivationRightHemi(0) = vecLabelActivationRightHemi[labelRightHemi.label_id];

                //Check if value is bigger than lower threshold. If not, don't plot activation
                if(vecActivationRightHemi(0) >= m_vecThresholds.x()) {
                    QByteArray arrayLabelColorsRightHemi = transformDataToColor(vecActivationRightHemi);
                    float *rawArrayLabelColorsRightHemi = reinterpret_cast<float *>(arrayLabelColorsRightHemi.data());

                    for(int j = 0; j<labelRightHemi.vertices.rows(); j++) {
                        rawArrayCurrentVertColorRightHemi[labelRightHemi.vertices(j)*3+0] = rawArrayLabelColorsRightHemi[0];
                        rawArrayCurrentVertColorRightHemi[labelRightHemi.vertices(j)*3+1] = rawArrayLabelColorsRightHemi[1];
                        rawArrayCurrentVertColorRightHemi[labelRightHemi.vertices(j)*3+2] = rawArrayLabelColorsRightHemi[2];
                    }
                }
            }

            colorPair.second = arrayCurrentVertColorRightHemi;

            return colorPair;
        }        

        case Data3DTreeModelItemRoles::SmoothingBased: {
            //TODO: Smooth here!
            break;
        }
    }

    return colorPair;
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

        for(int r = 0; r < data.rows(); ++r) {
            float dSample = data(r);

            //Check lower and upper thresholds and normalize to one
            if(dSample > m_vecThresholds.z()) {
                dSample = 1.0;
            } else if(dSample < m_vecThresholds.x()) {
                dSample = 0.0;
            } else {
                dSample = (dSample - m_vecThresholds.x()) / (m_vecThresholds.z() - m_vecThresholds.x());
            }

//            qDebug() << "dSample" << dSample;
//            qDebug() << "data(r)" << data(r);
//            qDebug() << "m_vecThresholds" << m_vecThresholds;

            QRgb qRgb;
            qRgb = ColorMap::valueToHotNegative1(dSample);

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

        for(int r = 0; r < data.rows(); ++r) {
            float dSample = data(r);

            //Check lower and upper thresholds and normalize to one
            if(dSample > m_vecThresholds.z()) {
                dSample = 1.0;
            } else if(dSample < m_vecThresholds.x()) {
                dSample = 0.0;
            } else {
                dSample = (dSample - m_vecThresholds.x()) / (m_vecThresholds.z() - m_vecThresholds.x());
            }

            QRgb qRgb;
            qRgb = ColorMap::valueToHotNegative2(dSample);

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

        for(int r = 0; r < data.rows(); ++r) {
            float dSample = data(r);

            //Check lower and upper thresholds and normalize to one
            if(dSample > m_vecThresholds.z()) {
                dSample = 1.0;
            } else if(dSample < m_vecThresholds.x()) {
                dSample = 0.0;
            } else {
                dSample = (dSample - m_vecThresholds.x()) / (m_vecThresholds.z() - m_vecThresholds.x());
            }

            QRgb qRgb;
            qRgb = ColorMap::valueToHot(dSample);

            QColor colSample(qRgb);
            rawArrayColors[idxColor++] = colSample.redF();
            rawArrayColors[idxColor++] = colSample.greenF();
            rawArrayColors[idxColor++] = colSample.blueF();
        }

        return arrayColor;
    }

    return arrayColor;
}
