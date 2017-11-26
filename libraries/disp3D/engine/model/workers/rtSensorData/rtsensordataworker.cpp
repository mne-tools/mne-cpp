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
#include <disp/helpers/colormap.h>
#include "../../../../helpers/interpolation/interpolation.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>
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
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorDataWorker::RtSensorDataWorker(bool bStreamSmoothedData)
: m_bIsLooping(true)
, m_iAverageSamples(1)
, m_dSFreq(1000.0)
, m_bStreamSmoothedData(bStreamSmoothedData)
, m_itCurrentSample(0)
, m_iSampleCtr(0)
{
    m_lVisualizationInfo.functionHandlerColorMap = ColorMap::valueToHot;
}


//*************************************************************************************************************

void RtSensorDataWorker::addData(const MatrixXd& data)
{
    if(data.rows() == 0) {
        return;
    }

    //Transform from matrix to list for easier handling in non loop mode
    for(int i = 0; i<data.cols(); i++) {
        if(m_lDataQ.size() < m_dSFreq) {
            m_lDataQ.push_back(data.col(i));
        } else {
            qDebug() <<"RtSensorDataWorker::addData - worker is full!";
            break;
        }
    }
}


//*************************************************************************************************************

void RtSensorDataWorker::setSurfaceColor(const MatrixX3f& matSurfaceVertColor)
{
    if(matSurfaceVertColor.size() == 0) {
        qDebug() << "RtSensorDataWorker::setSurfaceColor - Surface color data is empty. Returning ...";
        return;
    }

    m_lVisualizationInfo.matOriginalVertColor = matSurfaceVertColor;
}


//*************************************************************************************************************

void RtSensorDataWorker::setNumberAverages(int iNumAvr)
{
    m_iAverageSamples = iNumAvr;
}


//*************************************************************************************************************

void RtSensorDataWorker::setColormapType(const QString& sColormapType)
{
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

void RtSensorDataWorker::setThresholds(const QVector3D& vecThresholds)
{
    m_lVisualizationInfo.dThresholdX = vecThresholds.x();
    m_lVisualizationInfo.dThresholdZ = vecThresholds.z();
}


//*************************************************************************************************************

void RtSensorDataWorker::setLoopState(bool bLoopState)
{
    m_bIsLooping = bLoopState;
}


//*************************************************************************************************************

void RtSensorDataWorker::setSFreq(const double dSFreq)
{
    m_dSFreq = dSFreq;
}


//*************************************************************************************************************

void RtSensorDataWorker::setInterpolationMatrix(QSharedPointer<SparseMatrix<float>> matInterpolationOperator) {
    m_matInterpolationOperator = matInterpolationOperator;
}


//*************************************************************************************************************

void RtSensorDataWorker::streamData()
{
    if(m_lDataQ.size() > 0) {
        if(m_itCurrentSample == 0) {
            m_itCurrentSample = m_lDataQ.cbegin();
        }

        if(m_bIsLooping) {
            //Down sampling in loop mode
            if(m_vecAverage.rows() != m_lDataQ.front().rows()) {
                m_vecAverage = *m_itCurrentSample;
            } else {
                m_vecAverage += *m_itCurrentSample;
            }
        } else {
            //Down sampling in stream mode
            if(m_vecAverage.rows() != m_lDataQ.front().rows()) {
                m_vecAverage = m_lDataQ.front();
            } else {
                m_vecAverage += m_lDataQ.front();
            }

            m_lDataQ.pop_front();
        }

        m_itCurrentSample++;
        m_iSampleCtr++;

        //Set iterator back to the front if needed
        if(m_itCurrentSample == m_lDataQ.cend()) {
            m_itCurrentSample = m_lDataQ.cbegin();
        }

        if(m_iSampleCtr % m_iAverageSamples == 0) {
            //Perform the actual interpolation and send signal
            m_vecAverage /= (double)m_iAverageSamples;
            if(m_bStreamSmoothedData) {
                emit newRtSmoothedData(generateColorsFromSensorValues(m_vecAverage));
            } else {
                emit newRtRawData(m_vecAverage);
            }
            m_vecAverage.setZero(m_vecAverage.rows());

            //reset sample counter
            m_iSampleCtr = 0;
        }
        //qDebug()<<"RtSensorDataWorker::streamData - this->thread() "<< this->thread();
        //qDebug()<<"RtSensorDataWorker::streamData - m_lDataQ.size()"<<m_lDataQ.size();
    }
}


//*************************************************************************************************************

MatrixX3f RtSensorDataWorker::generateColorsFromSensorValues(const VectorXd& vecSensorValues)
{
    if(m_matInterpolationOperator.isNull()) {
        qDebug() << "RtSensorDataWorker::generateColorsFromSensorValues - Interpolation matrix was not defined. Returning ...";
        MatrixX3f matColor = m_lVisualizationInfo.matOriginalVertColor;
        return matColor;
    }

    // NOTE: This function is called for every new sample point and therefore must be kept highly efficient!
    if(vecSensorValues.rows() != m_matInterpolationOperator->cols()) {
        qDebug() << "RtSensorDataWorker::generateColorsFromSensorValues - Number of new vertex colors (" << vecSensorValues.rows() << ") do not match with previously set number of sensors (" << m_matInterpolationOperator->cols() << "). Returning...";
        MatrixX3f matColor = m_lVisualizationInfo.matOriginalVertColor;
        return matColor;
    }

    // interpolate sensor signals
    VectorXf vecIntrpltdVals = *Interpolation::interpolateSignal(m_matInterpolationOperator, vecSensorValues);

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
        fSample = std::fabs(vecData(r));

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
