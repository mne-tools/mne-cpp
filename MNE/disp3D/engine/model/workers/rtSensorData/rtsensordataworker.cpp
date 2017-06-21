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
#include <interpolation/interpolation.h>
#include <geometryInfo/geometryinfo.h>
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_types.h>

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
using namespace MNELIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace GEOMETRYINFO;
using namespace INTERPOLATION;

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
, m_numSensors(0)
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
    Interpolation::clearInterpolateMatrix();
}


//*************************************************************************************************************

void RtSensorDataWorker::addData(const MatrixXd& data)
{
    QMutexLocker locker(&m_qMutex);
    if(data.rows() == 0) {
        return;
    }

    //Transform from matrix to list for easier handling in non loop mode
    for(int i = 0; i<data.cols(); i++) {
        m_lData.append(data.col(i));
    }
}


//*************************************************************************************************************

void RtSensorDataWorker::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_lData.clear();
}


//*************************************************************************************************************

void RtSensorDataWorker::calculateSurfaceData(const MNEBemSurface &inSurface, const QVector<Vector3f> &sensorPos, const QString sensorType)
{
    QMutexLocker locker(&m_qMutex);

    if(inSurface.rr.rows() == 0) {
        qDebug() << "RtSensorDataWorker::calculateSurfaceData - Surface data is empty. Returning ...";
        return;
    }

    m_numSensors = sensorPos.size();

    //sensor projecting
    QSharedPointer<QVector<qint32>> mappedSubSet = GeometryInfo::projectSensor(inSurface, sensorPos);

    //SCDC with cancel distance 0.03m
    QSharedPointer<MatrixXd> distanceMatrix = GeometryInfo::scdc(inSurface, *mappedSubSet, 0.03);
    //@todo missing filtering of bad channels, add after merge

    // linear weight matrix
    m_weightMatrix = Interpolation::createInterpolationMat(*mappedSubSet, distanceMatrix);

    m_bSurfaceDataIsInit = true;
}


//*************************************************************************************************************

void RtSensorDataWorker::setSurfaceColor(const MatrixX3f& matSurfaceVertColor)
{
    QMutexLocker locker(&m_qMutex);
    if(matSurfaceVertColor.size() == 0) {
        qDebug() << "RtSensorDataWorker::setSurfaceColor - Surface color data is empty. Returning ...";
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

            if((m_iCurrentSample/1) % m_iAverageSamples == 0) {
                t_vecAverage /= (double)m_iAverageSamples;
                emit newRtData(generateColorsFromSensorValues(t_vecAverage));
                t_vecAverage.setZero(t_vecAverage.rows());
            }

            m_qMutex.unlock();
        }

        //Sleep specified amount of time - also take into account processing time from before
        const int iTimeLeft = m_iMSecIntervall - timer.elapsed();

        if(iTimeLeft > 0) {
            QThread::msleep(iTimeLeft);
        }
    }
}


//*************************************************************************************************************

MatrixX3f RtSensorDataWorker::generateColorsFromSensorValues(const VectorXd& sensorValues)
{
    // NOTE: This function is called for every new sample point and therefore must be kept highly efficient!
    if(sensorValues.rows() != m_numSensors) {
        qDebug() << "RtSensorDataWorker::generateColorsFromSensorValues - Number of new vertex colors (" << sensorValues.rows() << ") do not match with previously set number of vertices (" << m_numSensors << "). Returning...";
        MatrixX3f color = m_lVisualizationInfo.matOriginalVertColor;
        return color;
    }

    if(!m_bSurfaceDataIsInit) {
        qDebug() << "RtSensorDataWorker::generateColorsFromSensorValues - Surface data was not initialized. Returning ...";
        MatrixX3f color = m_lVisualizationInfo.matOriginalVertColor;
        return color;
    }

    // interpolate sensor signals
    if(! m_weightMatrix) {
        qDebug() << "RtSensorDataWorker::generateColorsFromSensorValues - weight matrix is no initialized. Returning ...";
    }
    VectorXd intrpltdVals = (* m_weightMatrix) * sensorValues;

    // Reset to original color as default
    m_lVisualizationInfo.matFinalVertColor = m_lVisualizationInfo.matOriginalVertColor;

    //Generate color data for vertices
    normalizeAndTransformToColor(
                intrpltdVals,
                m_lVisualizationInfo.matFinalVertColor,
                m_lVisualizationInfo.dThresholdX,
                m_lVisualizationInfo.dThresholdZ,
                m_lVisualizationInfo.functionHandlerColorMap);

    return m_lVisualizationInfo.matFinalVertColor;
}

//*************************************************************************************************************

void RtSensorDataWorker::normalizeAndTransformToColor(const VectorXd& data, MatrixX3f& matFinalVertColor, double dThresholdX, double dThreholdZ, QRgb (*functionHandlerColorMap)(double v))
{
    //Note: This function needs to be implemented extremly efficient. That is why we have three if clauses.
    //      Otherwise we would have to check which color map to take for each vertex.

    if(data.rows() != matFinalVertColor.rows()) {
        qDebug() << "RtSensorDataWorker::transformDataToColor - Sizes of input data (" <<data.rows() <<") do not match output data ("<< matFinalVertColor.rows() <<"). Returning ...";
        return;
    }

    double dSample;
    QRgb qRgb;
    const double dTresholdDiff = dThreholdZ - dThresholdX;

    for(int r = 0; r < data.rows(); ++r) {
        dSample = data(r);

        if(dSample >= dThresholdX) {
            //Check lower and upper thresholds and normalize to one
            if(dSample >= dThreholdZ) {
                dSample = 1.0;
            }
            else {
                if(dSample != 0.0 && dTresholdDiff != 0.0 ) {
                    dSample = (dSample - dThresholdX) / (dTresholdDiff);
                } else {
                    dSample = 0.0;
                }
            }

            qRgb = functionHandlerColorMap(dSample);

            matFinalVertColor(r,0) = (float)qRed(qRgb)/255.0f;
            matFinalVertColor(r,1) = (float)qGreen(qRgb)/255.0f;
            matFinalVertColor(r,2) = (float)qBlue(qRgb)/255.0f;
        }
    }
}

//*************************************************************************************************************
