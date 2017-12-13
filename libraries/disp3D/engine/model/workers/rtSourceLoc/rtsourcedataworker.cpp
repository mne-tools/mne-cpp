//=============================================================================================================
/**
* @file     rtsourcedataworker.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    RtSourceDataWorker class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourcedataworker.h"
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

RtSourceDataWorker::RtSourceDataWorker()
: m_bIsLooping(true)
, m_iAverageSamples(1)
, m_dSFreq(1000.0)
, m_bStreamSmoothedData(true)
, m_itCurrentSample(0)
, m_iSampleCtr(0)
{
    m_lVisualizationInfoLeft.functionHandlerColorMap = ColorMap::valueToHot;
    m_lVisualizationInfoRight.functionHandlerColorMap = ColorMap::valueToHot;
}


//*************************************************************************************************************

void RtSourceDataWorker::addData(const MatrixXd& data)
{
    if(data.rows() == 0) {
        return;
    }

    //Transform from matrix to list for easier handling in non loop mode
    for(int i = 0; i<data.cols(); i++) {
        if(m_lDataQ.size() < m_dSFreq) {
            m_lDataQ.push_back(data.col(i));
        } else {
            qDebug() <<"RtSourceDataWorker::addData - worker is full!";
            break;
        }
    }
}


//*************************************************************************************************************

void RtSourceDataWorker::setNumberVertices(int iNumberVertsLeft,
                                           int iNumberVertsRight)
{
    m_lVisualizationInfoLeft.matOriginalVertColor.resize(iNumberVertsLeft,3);
    m_lVisualizationInfoLeft.matOriginalVertColor.setZero();
    m_lVisualizationInfoRight.matOriginalVertColor.resize(iNumberVertsRight,3);
    m_lVisualizationInfoRight.matOriginalVertColor.setZero();
}


//*************************************************************************************************************

void RtSourceDataWorker::setNumberAverages(int iNumAvr)
{
    m_iAverageSamples = iNumAvr;
}


//*************************************************************************************************************

void RtSourceDataWorker::setStreamSmoothedData(bool bStreamSmoothedData)
{
    m_bStreamSmoothedData = bStreamSmoothedData;
}


//*************************************************************************************************************

void RtSourceDataWorker::setColormapType(const QString& sColormapType)
{
    //Create function handler to corresponding color map function
    if(sColormapType == "Hot Negative 1") {
        m_lVisualizationInfoLeft.functionHandlerColorMap = ColorMap::valueToHotNegative1;
        m_lVisualizationInfoRight.functionHandlerColorMap = ColorMap::valueToHotNegative1;
    } else if(sColormapType == "Hot") {
        m_lVisualizationInfoLeft.functionHandlerColorMap = ColorMap::valueToHot;
        m_lVisualizationInfoRight.functionHandlerColorMap = ColorMap::valueToHot;
    } else if(sColormapType == "Hot Negative 2") {
        m_lVisualizationInfoLeft.functionHandlerColorMap = ColorMap::valueToHotNegative2;
        m_lVisualizationInfoRight.functionHandlerColorMap = ColorMap::valueToHotNegative2;
    } else if(sColormapType == "Jet") {
        m_lVisualizationInfoLeft.functionHandlerColorMap = ColorMap::valueToJet;
        m_lVisualizationInfoRight.functionHandlerColorMap = ColorMap::valueToJet;
    }
}


//*************************************************************************************************************

void RtSourceDataWorker::setThresholds(const QVector3D& vecThresholds)
{
    m_lVisualizationInfoLeft.dThresholdX = vecThresholds.x();
    m_lVisualizationInfoLeft.dThresholdZ = vecThresholds.z();
    m_lVisualizationInfoRight.dThresholdX = vecThresholds.x();
    m_lVisualizationInfoRight.dThresholdZ = vecThresholds.z();
}


//*************************************************************************************************************

void RtSourceDataWorker::setLoopState(bool bLoopState)
{
    m_bIsLooping = bLoopState;
}


//*************************************************************************************************************

void RtSourceDataWorker::setSFreq(const double dSFreq)
{
    m_dSFreq = dSFreq;
}


//*************************************************************************************************************

void RtSourceDataWorker::setInterpolationMatrixLeft(const Eigen::SparseMatrix<float> &matInterpolationMatrixLeft)
{
    m_lVisualizationInfoLeft.matInterpolationMatrix = matInterpolationMatrixLeft;
}


//*************************************************************************************************************

void RtSourceDataWorker::setInterpolationMatrixRight(const Eigen::SparseMatrix<float> &matInterpolationMatrixRight)
{
    m_lVisualizationInfoRight.matInterpolationMatrix = matInterpolationMatrixRight;

}


//*************************************************************************************************************

void RtSourceDataWorker::streamData()
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
                emit newRtSmoothedData(generateColorsFromSensorValues(m_vecAverage.segment(0, m_lVisualizationInfoLeft.matInterpolationMatrix.cols()), m_lVisualizationInfoLeft),
                                       generateColorsFromSensorValues(m_vecAverage.segment(m_lVisualizationInfoLeft.matInterpolationMatrix.cols(), m_lVisualizationInfoRight.matInterpolationMatrix.cols()), m_lVisualizationInfoRight));
            } else {
                emit newRtRawData(m_vecAverage.segment(0, m_lVisualizationInfoLeft.matInterpolationMatrix.cols()),
                                  m_vecAverage.segment(m_lVisualizationInfoLeft.matInterpolationMatrix.cols(), m_lVisualizationInfoRight.matInterpolationMatrix.cols()));
            }
            m_vecAverage.setZero(m_vecAverage.rows());

            //reset sample counter
            m_iSampleCtr = 0;
        }

        //qDebug()<<"RtSourceDataWorker::streamData - this->thread() "<< this->thread();
    }
}


//*************************************************************************************************************

MatrixX3f RtSourceDataWorker::generateColorsFromSensorValues(const VectorXd &vecSensorValues,
                                                             VisualizationInfo &visualizationInfoHemi)
{
    if(vecSensorValues.rows() != visualizationInfoHemi.matInterpolationMatrix.cols()) {
        qDebug() << "RtSourceDataWorker::generateColorsFromSensorValues - Number of new vertex colors (" << vecSensorValues.rows() << ") do not match with previously set number of sensors (" << visualizationInfoHemi.matInterpolationMatrix.cols() << "). Returning...";
        MatrixX3f matColor = visualizationInfoHemi.matOriginalVertColor;
        return matColor;
    }

    // interpolate sensor signals
    VectorXf vecIntrpltdVals = Interpolation::interpolateSignal(visualizationInfoHemi.matInterpolationMatrix, vecSensorValues);

    // Reset to original color as default
    visualizationInfoHemi.matFinalVertColor = visualizationInfoHemi.matOriginalVertColor;

    //Generate color data for vertices
    normalizeAndTransformToColor(vecIntrpltdVals,
                                 visualizationInfoHemi.matFinalVertColor,
                                 visualizationInfoHemi.dThresholdX,
                                 visualizationInfoHemi.dThresholdZ,
                                 visualizationInfoHemi.functionHandlerColorMap);

    return visualizationInfoHemi.matFinalVertColor;
}


//*************************************************************************************************************

void RtSourceDataWorker::normalizeAndTransformToColor(const VectorXf& vecData,
                                                      MatrixX3f& matFinalVertColor,
                                                      double dThresholdX,
                                                      double dThreholdZ,
                                                      QRgb (*functionHandlerColorMap)(double v))
{
    //Note: This function needs to be implemented extremly efficient.
    if(vecData.rows() != matFinalVertColor.rows()) {
        qDebug() << "RtSourceDataWorker::normalizeAndTransformToColor - Sizes of input data (" << vecData.rows() <<") do not match output data ("<< matFinalVertColor.rows() <<"). Returning ...";
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
