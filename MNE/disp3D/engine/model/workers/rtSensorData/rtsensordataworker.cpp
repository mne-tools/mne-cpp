//=============================================================================================================
/**
* @file     rtsensordataworker.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2017
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
#include <interpolation/interpolation.h>
#include <geometryInfo/geometryinfo.h>
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QList>
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
, m_iMSecIntervall(17)
, m_bSurfaceDataIsInit(false)
, m_iNumSensors(0)
, m_dSFreq(1000.0)
{
    m_lVisualizationInfo = VisualizationInfo();
    m_lVisualizationInfo.functionHandlerColorMap = ColorMap::valueToHot;

    m_lInterpolationData = InterpolationData();
    //5cm cancel distance
    m_lInterpolationData.dCancelDistance = 0.05;
    m_lInterpolationData.interpolationFunction = Interpolation::cubic;
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
    if(data.rows() == 0) {
        return;
    }

    qDebug() <<"RtSensorDataWorker::addData - m_lData.size()"<<m_lData.size();

    //Transform from matrix to list for easier handling in non loop mode
    for(int i = 0; i<data.cols(); i++) {
        if(m_lData.size() < m_dSFreq) {
            m_lData.append(data.col(i));
        } else {
            qDebug() <<"RtSensorDataWorker::addData - worker is full!";
            break;
        }
    }
}


//*************************************************************************************************************

void RtSensorDataWorker::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_lData.clear();
}

//*************************************************************************************************************

void RtSensorDataWorker::calculateSurfaceData()
{
    //SCDC with cancel distance 
    m_lInterpolationData.pDistanceMatrix = GeometryInfo::scdc(m_lInterpolationData.bemSurface,
                                                              m_lInterpolationData.pVecMappedSubset,
                                                              m_lInterpolationData.dCancelDistance);

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(m_lInterpolationData.pDistanceMatrix,
                                    m_lInterpolationData.fiffInfo,
                                    m_lInterpolationData.iSensorType);

    //create weight matrix
    m_lInterpolationData.pWeightMatrix = Interpolation::createInterpolationMat(m_lInterpolationData.pVecMappedSubset,
                                                                               m_lInterpolationData.pDistanceMatrix,
                                                                               m_lInterpolationData.interpolationFunction,
                                                                               m_lInterpolationData.dCancelDistance,
                                                                               m_lInterpolationData.fiffInfo,
                                                                               m_lInterpolationData.iSensorType);
}

//*************************************************************************************************************

void RtSensorDataWorker::calculateSurfaceData(const MNEBemSurface &bemSurface,
                                              const QVector<Vector3f> &vecSensorPos,
                                              const FIFFLIB::FiffInfo &fiffInfo,
                                              int iSensorType)
{
    QMutexLocker locker(&m_qMutex);

    if(bemSurface.rr.rows() == 0) {
        qDebug() << "RtSensorDataWorker::calculateSurfaceData - Surface data is empty. Returning ...";
        return;
    }

    //set members
    m_iNumSensors = vecSensorPos.size();
    m_lInterpolationData.bemSurface = bemSurface;
    m_lInterpolationData.fiffInfo = fiffInfo;
    m_lInterpolationData.iSensorType = iSensorType;
    
    //sensor projecting: One time operation because surface and sensors can not change 
    m_lInterpolationData.pVecMappedSubset = GeometryInfo::projectSensors(m_lInterpolationData.bemSurface, vecSensorPos);
    
    calculateSurfaceData();

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

void RtSensorDataWorker::setCancelDistance(double dCancelDist)
{
    QMutexLocker locker(&m_qMutex);
    m_lInterpolationData.dCancelDistance = dCancelDist;

    if(m_bSurfaceDataIsInit == true){
        //recalculate because parameters changed
        calculateSurfaceData();
    }
}


//*************************************************************************************************************

void RtSensorDataWorker::setInterpolationFunction(const QString &sInterpolationFunction)
{
    QMutexLocker locker(&m_qMutex);
    if(sInterpolationFunction == "Linear") {
        m_lInterpolationData.interpolationFunction = Interpolation::linear;
    }
    else if(sInterpolationFunction == "Square") {
        m_lInterpolationData.interpolationFunction = Interpolation::square;
    }
    else if(sInterpolationFunction == "Cubic") {
        m_lInterpolationData.interpolationFunction = Interpolation::cubic;
    }
    else if(sInterpolationFunction == "Gaussian") {
        m_lInterpolationData.interpolationFunction = Interpolation::gaussian;
    }

    if(m_bSurfaceDataIsInit == true){
        //recalculate because parameters changed
        calculateSurfaceData();
    }
}


//*************************************************************************************************************

void RtSensorDataWorker::setLoop(bool bLooping)
{
    QMutexLocker locker(&m_qMutex);
    m_bIsLooping = bLooping;
}


//*************************************************************************************************************

void RtSensorDataWorker::setSFreq(const double dSFreq)
{
    QMutexLocker locker(&m_qMutex);

    m_dSFreq = dSFreq;
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
    QTime timer;

    while(true) {
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
                //Perform the actual interpolation and send signal
                t_vecAverage /= (double)m_iAverageSamples;
                emit newRtData(generateColorsFromSensorValues(t_vecAverage));
                t_vecAverage.setZero(t_vecAverage.rows());                

                //Sleep specified amount of time
                const int timerelap = timer.elapsed();
                const int iTimeLeft = m_iMSecIntervall - timerelap;

                //qDebug()<<"elapsed"<<timerelap<<"diff"<<iTimeLeft;
                if(iTimeLeft > 0) {
                    QThread::msleep(iTimeLeft);
                }
            }

            m_qMutex.unlock();
        }

        //qDebug()<<"m_lData.size()"<<m_lData.size();
    }
}


//*************************************************************************************************************

MatrixX3f RtSensorDataWorker::generateColorsFromSensorValues(const VectorXd& vecSensorValues)
{
    // NOTE: This function is called for every new sample point and therefore must be kept highly efficient!
    if(vecSensorValues.rows() != m_iNumSensors) {
        qDebug() << "RtSensorDataWorker::generateColorsFromSensorValues - Number of new vertex colors (" << vecSensorValues.rows() << ") do not match with previously set number of vertices (" << m_iNumSensors << "). Returning...";
        MatrixX3f matColor = m_lVisualizationInfo.matOriginalVertColor;
        return matColor;
    }

    if(!m_bSurfaceDataIsInit) {
        qDebug() << "RtSensorDataWorker::generateColorsFromSensorValues - Surface data was not initialized. Returning ...";
        MatrixX3f matColor = m_lVisualizationInfo.matOriginalVertColor;
        return matColor;
    }


    if(!m_lInterpolationData.pWeightMatrix) {
        qDebug() << "RtSensorDataWorker::generateColorsFromSensorValues - weight matrix is no initialized. Returning ...";
    }

    // interpolate sensor signals
    VectorXf vecIntrpltdVals = *Interpolation::interpolateSignal(m_lInterpolationData.pWeightMatrix, vecSensorValues);

    // Reset to original color as default
    m_lVisualizationInfo.matFinalVertColor = m_lVisualizationInfo.matOriginalVertColor;

    //Generate color data for vertices
    normalizeAndTransformToColor(vecIntrpltdVals,
                                    m_lVisualizationInfo.matFinalVertColor,
                                    m_lVisualizationInfo.dThresholdX,
                                    m_lVisualizationInfo.dThresholdZ,
                                    m_lVisualizationInfo.functionHandlerColorMap);

    return m_lVisualizationInfo.matFinalVertColor;
}

//*************************************************************************************************************

void RtSensorDataWorker::normalizeAndTransformToColor(const VectorXf& vecData,
                                                      MatrixX3f& matFinalVertColor,
                                                      double dThresholdX,
                                                      double dThreholdZ,
                                                      QRgb (*functionHandlerColorMap)(double v))
{
    //Note: This function needs to be implemented extremly efficient. That is why we have three if clauses.
    //      Otherwise we would have to check which color map to take for each vertex.

    if(vecData.rows() != matFinalVertColor.rows()) {
        qDebug() << "RtSensorDataWorker::transformDataToColor - Sizes of input data (" << vecData.rows() <<") do not match output data ("<< matFinalVertColor.rows() <<"). Returning ...";
        return;
    }

    float fSample;
    QRgb qRgb;
    const double dTresholdDiff = dThreholdZ - dThresholdX;

    for(int r = 0; r < vecData.rows(); ++r) {
        //Take the absolute values because the histogram threshold is also calcualted using the absolute values
        fSample = fabs(vecData(r));

        if(fSample >= dThresholdX) {
            //Check lower and upper thresholds and normalize to one
            if(fSample >= dThreholdZ) {
                fSample = 1.0f;
            } else {
                if(fSample != 0.0f && dTresholdDiff != 0.0 ) {
                    fSample = (fSample - dThresholdX) / (dTresholdDiff);
                } else {
                    fSample = 0.0f;
                }
            }

            qRgb = functionHandlerColorMap(fSample);

            matFinalVertColor(r,0) = (float)qRed(qRgb)/255.0f;
            matFinalVertColor(r,1) = (float)qGreen(qRgb)/255.0f;
            matFinalVertColor(r,2) = (float)qBlue(qRgb)/255.0f;
        }
    }
}

//*************************************************************************************************************
