//=============================================================================================================
/**
 * @file     rtsensordataworker.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensordataworker.h"
#include "../../../../helpers/interpolation/interpolation.h"
#include "../../items/common/abstractmeshtreeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>
#include <QDebug>
#include <QElapsedTimer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace DISPLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorDataWorker::RtSensorDataWorker()
: m_bIsLooping(true)
, m_iAverageSamples(1)
, m_dSFreq(1000.0)
, m_bStreamSmoothedData(true)
, m_iCurrentSample(0)
, m_pMatInterpolationMatrix(QSharedPointer<SparseMatrix<float> >(new SparseMatrix<float>()))
{
}

//=============================================================================================================

void RtSensorDataWorker::addData(const MatrixXd& data)
{
    if(data.rows() == 0) {
        qDebug() <<"RtSensorDataWorker::addData - Passed data is epmpty!";
        return;
    }

    //Transform from matrix to list for easier handling in non loop mode
    for(int i = 0; i<data.cols(); i++) {
        if(m_lDataQ.size() < m_dSFreq) {
            m_lDataQ.push_back(data.col(i));
        } else {
            qDebug() <<"RtSensorDataWorker::addData - worker is full ("<<m_lDataQ.size()<<")";
            break;
        }
    }

    m_lDataLoopQ = m_lDataQ;
}

//=============================================================================================================

void RtSensorDataWorker::setNumberVertices(int iNumberVerts)
{
//    m_lVisualizationInfo.matOriginalVertColor.resize(iNumberVerts,3);
//    m_lVisualizationInfo.matOriginalVertColor.setZero();
    m_lVisualizationInfo.matOriginalVertColor = AbstractMeshTreeItem::createVertColor(iNumberVerts);
}

//=============================================================================================================

void RtSensorDataWorker::setNumberAverages(int iNumAvr)
{
    m_iAverageSamples = iNumAvr;
}

//=============================================================================================================

void RtSensorDataWorker::setStreamSmoothedData(bool bStreamSmoothedData)
{
    m_bStreamSmoothedData = bStreamSmoothedData;
}

//=============================================================================================================

void RtSensorDataWorker::setColormapType(const QString& sColormapType)
{
    //Create function handler to corresponding color map function
    m_lVisualizationInfo.sColormapType = sColormapType;
}

//=============================================================================================================

void RtSensorDataWorker::setThresholds(const QVector3D& vecThresholds)
{
    m_lVisualizationInfo.dThresholdX = vecThresholds.x();
    m_lVisualizationInfo.dThresholdZ = vecThresholds.z();
}

//=============================================================================================================

void RtSensorDataWorker::setLoopState(bool bLoopState)
{
    m_bIsLooping = bLoopState;
}

//=============================================================================================================

void RtSensorDataWorker::setSFreq(const double dSFreq)
{
    m_dSFreq = dSFreq;
}

//=============================================================================================================

void RtSensorDataWorker::setInterpolationMatrix(QSharedPointer<SparseMatrix<float> > pMatInterpolationMatrix) {
    m_pMatInterpolationMatrix = pMatInterpolationMatrix;
}

//=============================================================================================================

void RtSensorDataWorker::streamData()
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    if(m_iAverageSamples != 0 && !m_lDataLoopQ.isEmpty()) {
        int iSampleCtr = 0;

        //Perform the actual interpolation and send signal
        while((iSampleCtr <= m_iAverageSamples)) {
            if(m_lDataQ.isEmpty()) {
                if(m_bIsLooping && !m_lDataLoopQ.isEmpty()) {
                    if(m_vecAverage.rows() != m_lDataLoopQ.front().rows()) {
                        m_vecAverage = m_lDataLoopQ.front();
                        m_iCurrentSample++;
                        iSampleCtr++;
                    } else if (m_iCurrentSample < m_lDataLoopQ.size()){
                        m_vecAverage += m_lDataLoopQ.at(m_iCurrentSample);
                        m_iCurrentSample++;
                        iSampleCtr++;
                    }

                    //Set iterator back to the front if needed
                    if(m_iCurrentSample >= m_lDataLoopQ.size()) {
                        m_iCurrentSample = 0;
                        break;
                    }
                } else {
                    return;
                }
            } else {
                if(m_vecAverage.rows() != m_lDataQ.front().rows()) {
                    m_vecAverage = m_lDataQ.takeFirst();
                    m_iCurrentSample++;
                    iSampleCtr++;
                } else {
                    m_vecAverage += m_lDataQ.takeFirst();
                    m_iCurrentSample++;
                    iSampleCtr++;
                }

                //Set iterator back to the front if needed
                if(m_iCurrentSample >= m_lDataQ.size()) {
                    m_iCurrentSample = 0;
                    break;
                }
            }
        }

        m_vecAverage /= (double)m_iAverageSamples;
        if(m_bStreamSmoothedData) {
            emit newRtSmoothedData(generateColorsFromSensorValues(m_vecAverage));
        } else {
            emit newRtRawData(m_vecAverage);
        }
        m_vecAverage.setZero(m_vecAverage.rows());
    }

    //    iTime = timer.elapsed();
    //    qWarning() << "RtSensorDataWorker::streamData iTime" << iTime;
    //    timer.restart();

    //qDebug()<<"RtSensorDataWorker::streamData - this->thread() "<< this->thread();
    //qDebug()<<"RtSensorDataWorker::streamData - m_lDataQ.size()"<<m_lDataQ.size();
}

//=============================================================================================================

MatrixX4f RtSensorDataWorker::generateColorsFromSensorValues(const VectorXd& vecSensorValues)
{
    if(vecSensorValues.rows() != m_pMatInterpolationMatrix->cols()) {
        qDebug() << "RtSensorDataWorker::generateColorsFromSensorValues - Number of new vertex colors (" << vecSensorValues.rows() << ") do not match with previously set number of sensors (" << m_pMatInterpolationMatrix->cols() << "). Returning...";
        MatrixX4f matColor = m_lVisualizationInfo.matOriginalVertColor;
        return matColor;
    }

    // interpolate sensor signals
    VectorXf vecIntrpltdVals = Interpolation::interpolateSignal(*m_pMatInterpolationMatrix, vecSensorValues.cast<float>());

    // Reset to original color as default
    m_lVisualizationInfo.matFinalVertColor = m_lVisualizationInfo.matOriginalVertColor;

    //Generate color data for vertices
    normalizeAndTransformToColor(vecIntrpltdVals,
                                 m_lVisualizationInfo.matFinalVertColor,
                                 m_lVisualizationInfo.dThresholdX,
                                 m_lVisualizationInfo.dThresholdZ,
                                 m_lVisualizationInfo.functionHandlerColorMap,
                                 m_lVisualizationInfo.sColormapType);

    return m_lVisualizationInfo.matFinalVertColor;
}

//=============================================================================================================

void RtSensorDataWorker::normalizeAndTransformToColor(const VectorXf& vecData,
                                                      MatrixX4f& matFinalVertColor,
                                                      double dThresholdX,
                                                      double dThreholdZ,
                                                      QRgb (*functionHandlerColorMap)(double v, const QString& sColorMap),
                                                      const QString& sColorMap)
{
    //Note: This function needs to be implemented extremly efficient.
    if(vecData.rows() != matFinalVertColor.rows()) {
        qDebug() << "RtSensorDataWorker::normalizeAndTransformToColor - Sizes of input data (" << vecData.rows() <<") do not match output data ("<< matFinalVertColor.rows() <<"). Returning ...";
        return;
    }

    float fSample;
    QRgb qRgb;
    const double dTresholdDiff = dThreholdZ - dThresholdX;

    for(int r = 0; r < vecData.rows(); ++r) {
        //Take the absolute values because the histogram threshold is also calcualted using the absolute values
        fSample = std::fabs(vecData(r));

        if(fSample >= dThresholdX) {
            matFinalVertColor(r,3) = 1.0f;

            //Check lower and upper thresholds and normalize to one
            if(fSample >= dThreholdZ) {
                if(vecData(r) < 0) {
                    fSample = 0.0;
                } else {
                    fSample = 1.0;
                }
                //fSample = 1.0f;
            } else {
                if(fSample != 0.0f && dTresholdDiff != 0.0 ) {
                    if(vecData(r) < 0) {
                        fSample = 0.5 - (fSample - dThresholdX) / (dTresholdDiff * 2);
                    } else {
                        fSample = 0.5 + (fSample - dThresholdX) / (dTresholdDiff * 2);
                    }
                } else {
                    fSample = 0.0f;
                }
            }

            qRgb = functionHandlerColorMap(fSample, sColorMap);

            matFinalVertColor(r,0) = (float)qRed(qRgb)/255.0f;
            matFinalVertColor(r,1) = (float)qGreen(qRgb)/255.0f;
            matFinalVertColor(r,2) = (float)qBlue(qRgb)/255.0f;
        } else {
            //matFinalVertColor(r,3) = 0.0f;
        }
    }
}

//=============================================================================================================
