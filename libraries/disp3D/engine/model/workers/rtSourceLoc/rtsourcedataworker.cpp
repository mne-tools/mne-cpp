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
#include <disp/plots/helpers/colormap.h>
#include "../../../../helpers/interpolation/interpolation.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>
#include <QDebug>
#include <QElapsedTimer>
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
, m_iCurrentSample(0)
, m_iSampleCtr(0)
{
    VisualizationInfo leftHemiInfo;
    VisualizationInfo rightHemiInfo;
    leftHemiInfo.functionHandlerColorMap = ColorMap::valueToHot;
    rightHemiInfo.functionHandlerColorMap = ColorMap::valueToHot;
    leftHemiInfo.pMatInterpolationMatrix = QSharedPointer<SparseMatrix<float> >(new SparseMatrix<float>());
    rightHemiInfo.pMatInterpolationMatrix = QSharedPointer<SparseMatrix<float> >(new SparseMatrix<float>());
    m_lHemiVisualizationInfo << leftHemiInfo << rightHemiInfo;
}


//*************************************************************************************************************

void RtSourceDataWorker::addData(const MatrixXd& data)
{
    if(data.rows() == 0) {
        qDebug() <<"RtSourceDataWorker::addData - Passed data is epmpty!";
        return;
    }

    //Transform from matrix to list for easier handling in non loop mode
    for(int i = 0; i<data.cols(); i++) {
        if(m_lDataQ.size() < m_dSFreq) {
            m_lDataQ.push_back(data.col(i));
        } else {
            qDebug() <<"RtSourceDataWorker::addData - worker is full ("<<m_lDataQ.size()<<")";
            break;
        }
    }
}


//*************************************************************************************************************

void RtSourceDataWorker::setNumberVertices(int iNumberVertsLeft,
                                           int iNumberVertsRight)
{
    m_lHemiVisualizationInfo[0].matOriginalVertColor.setZero(iNumberVertsLeft,3);
    m_lHemiVisualizationInfo[1].matOriginalVertColor.setZero(iNumberVertsRight,3);
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
    if(sColormapType == QStringLiteral("Hot Negative 1")) {
        m_lHemiVisualizationInfo[0].functionHandlerColorMap = ColorMap::valueToHotNegative1;
        m_lHemiVisualizationInfo[1].functionHandlerColorMap = ColorMap::valueToHotNegative1;
    } else if(sColormapType == QStringLiteral("Hot")) {
        m_lHemiVisualizationInfo[0].functionHandlerColorMap = ColorMap::valueToHot;
        m_lHemiVisualizationInfo[1].functionHandlerColorMap = ColorMap::valueToHot;
    } else if(sColormapType == QStringLiteral("Hot Negative 2")) {
        m_lHemiVisualizationInfo[0].functionHandlerColorMap = ColorMap::valueToHotNegative2;
        m_lHemiVisualizationInfo[1].functionHandlerColorMap = ColorMap::valueToHotNegative2;
    } else if(sColormapType == QStringLiteral("Jet")) {
        m_lHemiVisualizationInfo[0].functionHandlerColorMap = ColorMap::valueToJet;
        m_lHemiVisualizationInfo[1].functionHandlerColorMap = ColorMap::valueToJet;
    }
}


//*************************************************************************************************************

void RtSourceDataWorker::setThresholds(const QVector3D& vecThresholds)
{
    m_lHemiVisualizationInfo[0].dThresholdX = vecThresholds.x();
    m_lHemiVisualizationInfo[0].dThresholdZ = vecThresholds.z();
    m_lHemiVisualizationInfo[1].dThresholdX = vecThresholds.x();
    m_lHemiVisualizationInfo[1].dThresholdZ = vecThresholds.z();
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

void RtSourceDataWorker::setInterpolationMatrixLeft(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixLeft)
{
    m_lHemiVisualizationInfo[0].pMatInterpolationMatrix = pMatInterpolationMatrixLeft;
}


//*************************************************************************************************************

void RtSourceDataWorker::setInterpolationMatrixRight(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixRight)
{
    m_lHemiVisualizationInfo[1].pMatInterpolationMatrix = pMatInterpolationMatrixRight;

}


//*************************************************************************************************************

void RtSourceDataWorker::streamData()
{
    QElapsedTimer time;
    time.start();
    if(m_lDataQ.size() > 0) {
        if(m_bIsLooping) {
            //Down sampling in loop mode
            if(m_vecAverage.rows() != m_lDataQ.front().rows()) {
                m_vecAverage = m_lDataQ.front();
            } else if (m_iCurrentSample < m_lDataQ.size()){
                m_vecAverage += m_lDataQ.at(m_iCurrentSample);
            }
        } else {
            //Down sampling in stream mode
            if(m_vecAverage.rows() != m_lDataQ.front().rows()) {
                m_vecAverage = m_lDataQ.takeFirst();
            } else {
                m_vecAverage += m_lDataQ.takeFirst();
            }
        }

        m_iCurrentSample++;
        m_iSampleCtr++;

        //Set iterator back to the front if needed
        if(m_iCurrentSample == m_lDataQ.size()) {
            m_iCurrentSample = 0;
        }

        if(m_iSampleCtr % m_iAverageSamples == 0
                && m_iAverageSamples != 0
                && m_lHemiVisualizationInfo[0].pMatInterpolationMatrix->cols() != 0
                && m_lHemiVisualizationInfo[1].pMatInterpolationMatrix->cols() != 0) {
            //Perform the actual interpolation and send signal
            m_vecAverage /= (double)m_iAverageSamples;
            if(m_bStreamSmoothedData) {
                m_lHemiVisualizationInfo[0].vecSensorValues = m_vecAverage.segment(0, m_lHemiVisualizationInfo[0].pMatInterpolationMatrix->cols());
                m_lHemiVisualizationInfo[1].vecSensorValues = m_vecAverage.segment(m_lHemiVisualizationInfo[0].pMatInterpolationMatrix->cols(), m_lHemiVisualizationInfo[1].pMatInterpolationMatrix->cols());

                //Do calculations for both hemispheres in parallel
                QFuture<void> result = QtConcurrent::map(m_lHemiVisualizationInfo,
                                                         generateColorsFromSensorValues);
                result.waitForFinished();

                emit newRtSmoothedData(m_lHemiVisualizationInfo[0].matFinalVertColor,
                                       m_lHemiVisualizationInfo[1].matFinalVertColor);
            } else {
                emit newRtRawData(m_vecAverage.segment(0, m_lHemiVisualizationInfo[0].pMatInterpolationMatrix->cols()),
                                  m_vecAverage.segment(m_lHemiVisualizationInfo[0].pMatInterpolationMatrix->cols(), m_lHemiVisualizationInfo[1].pMatInterpolationMatrix->cols()));
            }
            m_vecAverage.setZero(m_vecAverage.rows());

            //reset sample counter
            m_iSampleCtr = 0;
        }

        //qDebug()<<"RtSourceDataWorker::streamData - this->thread() "<< this->thread();
    }
     qDebug()<<"RtSourceDataWorker::streamData - time.elapsed()" << time.elapsed();
}


//*************************************************************************************************************

void RtSourceDataWorker::generateColorsFromSensorValues(VisualizationInfo &visualizationInfoHemi)
{
    QElapsedTimer time;
     time.start();
    if(visualizationInfoHemi.vecSensorValues.rows() != visualizationInfoHemi.pMatInterpolationMatrix->cols()) {
        qDebug() << "RtSourceDataWorker::generateColorsFromSensorValues - Number of new vertex colors (" << visualizationInfoHemi.vecSensorValues.rows() << ") do not match with previously set number of sensors (" << visualizationInfoHemi.pMatInterpolationMatrix->cols() << "). Returning...";
        return;
    }

    // interpolate sensor signals
    VectorXf vecIntrpltdVals = Interpolation::interpolateSignal(visualizationInfoHemi.pMatInterpolationMatrix, visualizationInfoHemi.vecSensorValues);
    qDebug()<<"RtSourceDataWorker::generateColorsFromSensorValues - interpolateSignal" << time.elapsed();

    // Reset to original color as default
    visualizationInfoHemi.matFinalVertColor = visualizationInfoHemi.matOriginalVertColor;

    //Generate color data for vertices
    time.restart();
    normalizeAndTransformToColor(vecIntrpltdVals,
                                 visualizationInfoHemi.matFinalVertColor,
                                 visualizationInfoHemi.dThresholdX,
                                 visualizationInfoHemi.dThresholdZ,
                                 visualizationInfoHemi.functionHandlerColorMap);
    qDebug()<<"RtSourceDataWorker::generateColorsFromSensorValues - normalizeAndTransformToColor" << time.elapsed();
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

    QList<ColorComputationInfo> lData;
    ColorComputationInfo tempData;
    tempData.dThresholdX = dThresholdX;
    tempData.dThresholdZ = dThreholdZ;
    tempData.functionHandlerColorMap = functionHandlerColorMap;
    tempData.iFinalMatSize = vecData.rows();

    int iStepSize = vecData.rows()/QThread::idealThreadCount();
    int iResidual = vecData.rows()%QThread::idealThreadCount();

    for(int i = 0; i < QThread::idealThreadCount(); i++) {
        tempData.vecData = vecData.segment(i*iStepSize, iStepSize);

        lData << tempData;
    }

    tempData.vecData = vecData.segment(iStepSize*QThread::idealThreadCount(), iResidual);

    lData << tempData;

    QFuture<MatrixX3f> result = QtConcurrent::mappedReduced(lData,
                                                            computeColors,
                                                            reduceColors,
                                                            QtConcurrent::OrderedReduce);
    result.waitForFinished();

    matFinalVertColor = result.result();
}


//*************************************************************************************************************

MatrixX3f RtSourceDataWorker::computeColors(const ColorComputationInfo& inputData)
{
    qDebug() << "RtSourceDataWorker::computeColors";
    MatrixX3f matFinalVertColor;
    float fSample;
    QRgb qRgb;
    const double dTresholdDiff = inputData.dThresholdZ - inputData.dThresholdX;

    for(int r = 0; r < inputData.vecData.rows(); ++r) {
        //Take the absolute values because the histogram threshold is also calcualted using the absolute values
        fSample = std::fabs(inputData.vecData(r));

        if(fSample >= inputData.dThresholdX) {
            //Check lower and upper thresholds and normalize to one
            if(fSample >= inputData.dThresholdZ) {
                fSample = 1.0f;
            } else {
                if(fSample != 0.0f && dTresholdDiff != 0.0 ) {
                    fSample = (fSample - inputData.dThresholdX) / (dTresholdDiff);
                } else {
                    fSample = 0.0f;
                }
            }

            qRgb = inputData.functionHandlerColorMap(fSample);

            matFinalVertColor(r,0) = (float)qRed(qRgb)/255.0f;
            matFinalVertColor(r,1) = (float)qGreen(qRgb)/255.0f;
            matFinalVertColor(r,2) = (float)qBlue(qRgb)/255.0f;
        }
    }

    qDebug() << "matFinalVertColor.rows()" << matFinalVertColor.rows();
    return matFinalVertColor;
}


//*************************************************************************************************************

void RtSourceDataWorker::reduceColors(MatrixX3f& finalData, const MatrixX3f& inputData)
{
    int iStartRow = finalData.rows();
    finalData.conservativeResize(finalData.rows()+inputData.rows(), 3);

    finalData.block(iStartRow, 0, inputData.rows(), 3) = inputData;

    qDebug() << "finalData.rows()" << finalData.rows();
    qDebug() << "finalData.cols()" << finalData.cols();
}
