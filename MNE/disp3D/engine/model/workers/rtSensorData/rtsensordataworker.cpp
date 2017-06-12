//=============================================================================================================
/**
* @file     rtsensordataworker.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    RtSensorDataWorker class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensordataworker.h"
#include "../../items/common/types.h"

#include <disp/helpers/colormap.h>
#include <utils/ioutils.h>
#include <fs/label.h>
#include <fs/annotation.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include <QTime>
#include <QDebug>
#include <QtConcurrent>


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
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================




//*************************************************************************************************************

void _transformDataToColor(const VectorXd& data, MatrixX3f& matFinalVertColor, double dTrehsoldX, double dTrehsoldZ, QRgb (*functionHandlerColorMap)(double v))
{
    //Note: This function needs to be implemented extremly efficient. That is why we have three if clauses.
    //      Otherwise we would have to check which color map to take for each vertex.
    //QElapsedTimer timer;
    //timer.start();

    if(data.rows() != matFinalVertColor.rows()) {
        qDebug() << "RtSourceLocDataWorker::transformDataToColor - Sizes of input data (" <<data.rows() <<") do not match output data ("<< matFinalVertColor.rows() <<"). Returning ...";
    }

    float dSample;
    QRgb qRgb;
    double dTresholdDiff = dTrehsoldZ - dTrehsoldX;

    for(int r = 0; r < data.rows(); ++r) {
        dSample = data(r);

        if(dSample >= dTrehsoldX) {
            //Check lower and upper thresholds and normalize to one
            if(dSample >= dTrehsoldZ) {
                dSample = 1.0;
            } else if(dSample < dTrehsoldX) {
                dSample = 0.0;
            } else {
                if(dTresholdDiff != 0.0) {
                    dSample = (dSample - dTrehsoldX) / (dTresholdDiff);
                } else {
                    dSample = 0.0f;
                }

            }

            qRgb = functionHandlerColorMap(dSample);

            matFinalVertColor(r,0) = (float)qRed(qRgb)/255.0f;
            matFinalVertColor(r,1) = (float)qGreen(qRgb)/255.0f;
            matFinalVertColor(r,2) = (float)qBlue(qRgb)/255.0f;
        }
    }

    //int elapsed = timer.elapsed();
    //qDebug()<<"RtSourceLocDataWorker::transformDataToColor - elapsed"<<elapsed;
}


//*************************************************************************************************************

void _transformDataToColor(float fSample, QColor& finalVertColor, double dTrehsoldX, double dTrehsoldZ, QRgb (*functionHandlerColorMap)(double v))
{
    //Note: This function needs to be implemented extremley efficient. That is why we have three if clauses.
    //      Otherwise we would have to check which color map to take for each vertex.
    double dTresholdDiff = dTrehsoldZ - dTrehsoldX;

    //Check lower and upper thresholds and normalize to one
    if(fSample >= dTrehsoldZ) {
        fSample = 1.0f;
    } else if(fSample < dTrehsoldX) {
        fSample = 0.0f;
    } else {
        if(dTresholdDiff != 0.0) {
            fSample = (fSample - dTrehsoldX) / dTresholdDiff;
        } else {
            fSample = 0.0f;
        }
    }

    QRgb qRgb;
    qRgb = (functionHandlerColorMap)(fSample);

    finalVertColor.setRedF((float)qRed(qRgb)/255.0f);
    finalVertColor.setGreenF((float)qGreen(qRgb)/255.0f);
    finalVertColor.setBlueF((float)qBlue(qRgb)/255.0f);
}


//*************************************************************************************************************


void _generateColorsPerVertex(VisualizationInfo& input)
{
    QColor color;

    //Fill final QByteArray with colors based on the current anatomical information
    for(int i = 0; i < input.vVertNo.rows(); ++i) {
        if(input.vSourceColorSamples(i) >= input.dThresholdX) {
            _transformDataToColor(input.vSourceColorSamples(i), color, input.dThresholdX, input.dThresholdZ, input.functionHandlerColorMap);

            input.matFinalVertColor(input.vVertNo(i),0) = color.redF();
            input.matFinalVertColor(input.vVertNo(i),1) = color.greenF();
            input.matFinalVertColor(input.vVertNo(i),2) = color.blueF();
        }
    }
}

//*************************************************************************************************************

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorDataWorker::RtSensorDataWorker(QObject* parent)
: QThread(parent)
, m_bIsRunning(false)
, m_bIsLooping(true)
, m_iAverageSamples(1)
, m_iCurrentSample(0)
, m_iMSecIntervall(50)
, m_bSurfaceDataIsInit(false)
{
    m_lVisualizationInfo = VisualizationInfo();
    m_lVisualizationInfo.functionHandlerColorMap = ColorMap::valueToHot;
}


//*************************************************************************************************************

RtSensorDataWorker::~RtSensorDataWorker()
{
    if(this->isRunning()) {
        stop();
    }
}


//*************************************************************************************************************

void RtSensorDataWorker::addData(const MatrixXd& data)
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

void RtSensorDataWorker::clear()
{
    QMutexLocker locker(&m_qMutex);
}


//*************************************************************************************************************

void RtSensorDataWorker::setSurfaceData(const Eigen::VectorXi& vecVertNo,
                                           const QVector<QVector<int> > &mapVertexNeighbors,
                                           const MatrixX3f &matVertPosLeftHemi)
{
    QMutexLocker locker(&m_qMutex);

    if(vecVertNo.rows() == 0) {
        qDebug() << "RtSourceLocDataWorker::setSurfaceData - Surface data is empty. Returning ...";
        return;
    }

    m_lVisualizationInfo.vVertNo = vecVertNo;

    m_lVisualizationInfo.mapVertexNeighbors = mapVertexNeighbors;

    // createSmoothingOperator(matVertPosLeftHemi, matVertPosRightHemi);

    m_bSurfaceDataIsInit = true;
}


//*************************************************************************************************************

void RtSensorDataWorker::setSurfaceColor(const MatrixX3f& matSurfaceVertColor)
{
    QMutexLocker locker(&m_qMutex);

    if(matSurfaceVertColor.size() == 0) {
        qDebug() << "RtSourceLocDataWorker::setSurfaceColor - Surface color data is empty. Returning ...";
        return;
    }

    m_lVisualizationInfo.matOriginalVertColor = matSurfaceVertColor;
}


//*************************************************************************************************************

void RtSensorDataWorker::setNumberAverages(int iNumAvr)
{
    QMutexLocker locker(&m_qMutex);
    m_iAverageSamples = iNumAvr;
}


//*************************************************************************************************************

void RtSensorDataWorker::setInterval(int iMSec)
{
    QMutexLocker locker(&m_qMutex);
    m_iMSecIntervall = iMSec;
}


//*************************************************************************************************************

void RtSensorDataWorker::setColormapType(const QString& sColormapType)
{
    QMutexLocker locker(&m_qMutex);

    //Create function handler to corresponding color map function
    if(sColormapType == "Hot Negative 1") {
        m_lVisualizationInfo.functionHandlerColorMap = ColorMap::valueToHotNegative1;
    } else if(sColormapType == "Hot") {
        m_lVisualizationInfo.functionHandlerColorMap = ColorMap::valueToHot;
    } else if(sColormapType == "Hot Negative 2") {
        m_lVisualizationInfo.functionHandlerColorMap = ColorMap::valueToHotNegative2;
    } else if(sColormapType == "Jet") {
        m_lVisualizationInfo.functionHandlerColorMap = ColorMap::valueToJet;
    }
}


//*************************************************************************************************************

void RtSensorDataWorker::setNormalization(const QVector3D& vecThresholds)
{
    QMutexLocker locker(&m_qMutex);
    m_lVisualizationInfo.dThresholdX = vecThresholds.x();

    m_lVisualizationInfo.dThresholdZ = vecThresholds.z();
}


//*************************************************************************************************************

void RtSensorDataWorker::setLoop(bool looping)
{
    QMutexLocker locker(&m_qMutex);
    m_bIsLooping = looping;
}


//*************************************************************************************************************

void RtSensorDataWorker::start()
{
    m_qMutex.lock();
    m_iCurrentSample = 0;
    m_qMutex.unlock();

    QThread::start();
}


//*************************************************************************************************************

void RtSensorDataWorker::stop()
{
    m_qMutex.lock();
    m_bIsRunning = false;
    m_qMutex.unlock();

    QThread::wait();
}


//*************************************************************************************************************

void RtSensorDataWorker::run()
{
    VectorXd t_vecAverage;

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

        //Sleep specified amount of time - also take into account processing time from before
        int iTimerElapsed = timer.elapsed();
        //qDebug() << "RtSourceLocDataWorker::run()" << timer.elapsed() << "msecs";

        if(m_iMSecIntervall-iTimerElapsed > 0) {
            QThread::msleep(m_iMSecIntervall-iTimerElapsed);
        }
    }
}


//*************************************************************************************************************

MatrixX3f RtSensorDataWorker::performVisualizationTypeCalculation(const VectorXd& vSourceColorSamples)
{
    //NOTE: This function is called for every new sample point and therefore must be kept highly efficient!
//    QTime allTimer;
//    allTimer.start();

    if(vSourceColorSamples.rows() != m_lVisualizationInfo.vVertNo.rows()) {
        qDebug() << "RtSourceLocDataWorker::performVisualizationTypeCalculation - Number of new vertex colors (" << vSourceColorSamples.rows() << ") do not match with previously set number of vertices (" << m_lVisualizationInfo.vVertNo.rows() << "). Returning...";
        MatrixX3f color = m_lVisualizationInfo.matOriginalVertColor;
        return color;
    }

    if(!m_bSurfaceDataIsInit) {
        qDebug() << "RtSourceLocDataWorker::performVisualizationTypeCalculation - Surface data was not initialized. Returning ...";
        MatrixX3f color = m_lVisualizationInfo.matOriginalVertColor;
        return color;
    }

    // copy source data
    m_lVisualizationInfo.vSourceColorSamples = vSourceColorSamples;

    //Reset to original color as default
    m_lVisualizationInfo.matFinalVertColor = m_lVisualizationInfo.matOriginalVertColor;

    //Generate color data for vertices
    QFuture<void> future = QtConcurrent::run(_generateColorsPerVertex, m_lVisualizationInfo);
    future.waitForFinished();

//    int iAllTimer = allTimer.elapsed();
//    qDebug() << "All time" << iAllTimer;

    MatrixX3f color;
    color = m_lVisualizationInfo.matFinalVertColor;
    return color;
}


//*************************************************************************************************************

