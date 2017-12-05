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

void generateWeightsPerVertex(SmoothVertexInfo& input)
{
    QVector3D from = input.vVertPos;
    QVector3D to;

    double dist, valueWeight;
    double dWeightsSum = 0;

    for(int j = 0; j < input.lSourcePos.size(); ++j) {
        to = input.lSourcePos.at(j);

        if(to == from) {
            dist = exp(-25);
        } else {
            dist = from.distanceToPoint(to);
        }

        if(dist <= input.dThresholdDistance) {
            valueWeight = std::fabs(1.0/pow(dist,input.iDistPow));

            input.lTriplets.append(Eigen::Triplet<double>(input.iVertIdx, j, valueWeight));
            dWeightsSum += valueWeight;
        }
    }

    //Divide by the sum of all weights
    for(int j = 0; j < input.lTriplets.size(); ++j) {
        input.lTriplets[j] = Eigen::Triplet<double>(input.lTriplets.at(j).row(), input.lTriplets.at(j).col(), input.lTriplets.at(j).value()/dWeightsSum);
    }
}


//*************************************************************************************************************

void generateSmoothOperator(SmoothOperatorInfo& input)
{
    //Prepare data
    QList<SmoothVertexInfo> lInputData;
    SmoothVertexInfo vertInfo;
    QList<QVector3D> lSourcePos;
    QVector3D vertPos;

    //Create list with all source positions
    for(int j = 0; j < input.vecVertNo.rows(); ++j) {
        vertPos.setX(input.matVertPos(input.vecVertNo(j),0));
        vertPos.setY(input.matVertPos(input.vecVertNo(j),1));
        vertPos.setZ(input.matVertPos(input.vecVertNo(j),2));

        lSourcePos.append(vertPos);
    }

    for(int j = 0; j < input.matVertPos.rows(); ++j) {
        vertPos.setX(input.matVertPos(j,0));
        vertPos.setY(input.matVertPos(j,1));
        vertPos.setZ(input.matVertPos(j,2));

        vertInfo.vVertPos = vertPos;
        vertInfo.dThresholdDistance = input.dThresholdDistance;
        vertInfo.lSourcePos = lSourcePos;
        vertInfo.iDistPow = input.iDistPow;
        vertInfo.iVertIdx = j;

        lInputData << vertInfo;
    }

    //Do the vertex dist weight calculation for each vertex in a different thread
    QFuture<void> future = QtConcurrent::map(lInputData, generateWeightsPerVertex);
    future.waitForFinished();

    QList<Eigen::Triplet<double> > lFinalTriplets;
    for(int j = 0; j < lInputData.size(); ++j) {
        lFinalTriplets.append(lInputData.at(j).lTriplets);
    }

    input.sparseSmoothMatrix.setFromTriplets(lFinalTriplets.begin(), lFinalTriplets.end());
}


//*************************************************************************************************************

void transformDataToColor(const VectorXd& data, MatrixX3f& matFinalVertColor, double dTrehsoldX, double dTrehsoldZ, QRgb (*functionHandlerColorMap)(double v))
{
    //Note: This function needs to be implemented extremley efficient. That is why we have three if clauses.
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
        //Take the absolute values because the histogram threshold is also calcualted using the absolute values
        dSample = std::fabs(data(r));

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

void transformDataToColor(float fSample, QColor& finalVertColor, double dTrehsoldX, double dTrehsoldZ, QRgb (*functionHandlerColorMap)(double v))
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

void generateColorsPerVertex(VisualizationInfo& input)
{
    QColor color;

    //Fill final QByteArray with colors based on the current anatomical information
    for(int i = 0; i < input.vVertNo.rows(); ++i) {
        if(input.vSourceColorSamples(i) >= input.dThresholdX) {
            transformDataToColor(input.vSourceColorSamples(i), color, input.dThresholdX, input.dThresholdZ, input.functionHandlerColorMap);

            input.matFinalVertColor(input.vVertNo(i),0) = color.redF();
            input.matFinalVertColor(input.vVertNo(i),1) = color.greenF();
            input.matFinalVertColor(input.vVertNo(i),2) = color.blueF();
        }
    }
}


//*************************************************************************************************************

void generateColorsPerAnnotation(VisualizationInfo& input)
{
    //Find maximum actiavtion for each label
    QMap<qint32, double> vecLabelActivation;

    for(int i = 0; i < input.vSourceColorSamples.rows(); ++i) {
        //Find out label for source
        qint32 labelIdx = input.mapLabelIdSources[input.vVertNo(i)];

        if(std::fabs(input.vSourceColorSamples(i)) > std::fabs(vecLabelActivation[labelIdx]))
            vecLabelActivation.insert(labelIdx, input.vSourceColorSamples(i));
    }

    //Color all labels respectivley to their activation
    QColor color;

    for(int i = 0; i<input.lLabels.size(); i++) {
        FSLIB::Label label = input.lLabels.at(i);

        //Transform label activations to rgb colors
        //Check if value is bigger than lower threshold. If not, don't plot activation
        if(vecLabelActivation[label.label_id] >= input.dThresholdX) {
            transformDataToColor(std::fabs(vecLabelActivation[label.label_id]), color, input.dThresholdX, input.dThresholdZ, input.functionHandlerColorMap);

            for(int j = 0; j<label.vertices.rows(); j++) {
                input.matFinalVertColor(label.vertices(j),0) = color.redF();
                input.matFinalVertColor(label.vertices(j),1) = color.greenF();
                input.matFinalVertColor(label.vertices(j),2) = color.blueF();
            }
        }
    }
}


//*************************************************************************************************************

void generateSmoothedColors(VisualizationInfo& input)
{
    //QTime prodDataTimer;
    //prodDataTimer.start();

//    //Option 1 - Use Matti's version. Smoothes between different source "patches".
//    //Activity is spread evenly around every source and then smoothed to neighboring source patches.
//    //Init the variables
//    int n,k,p,it,nn,nv;
//    float sum;
//    int niter = 2;

//    int nSurfaceVerts = arrayFinalVertColor.size() / (sizeof(float) * 3);
//    int nvert = sourceColorSamples.rows();

//    QVector<bool> undef(nSurfaceVerts, true);
//    QVector<bool> prev_undef(nSurfaceVerts);
//    QVector<bool> isSource(nSurfaceVerts, false);
//    VectorXd prev_val(nSurfaceVerts);
//    VectorXd smooth_val(nSurfaceVerts);
//    smooth_val.setZero();

//    //Set all vertex activation of the actually chosen sources to their current activation
//    for (k = 0; k < nvert; k++) {
//        undef[vertno[k]] = false;
//        smooth_val[vertno[k]] = sourceColorSamples[k];
//        isSource[vertno[k]] = true;
//    }

//    //Smooth here
//    for (it = 0; it < niter; it++) {
//        prev_undef = undef;
//        prev_val = smooth_val;

//        for (k = 0; k < nSurfaceVerts; k++) {
//            sum = 0;
//            n   = 0;

//            if (!prev_undef[k]) {
//                //If vertex color was defined during last step
//                sum = smooth_val[k];
//                n = 1;
//            }

//            //Generate the color of the current vertex at pos k based on its neighbor information
//            nn = mapVertexNeighbors[k].size();

//            for (p = 0; p < nn; p++) {
//                nv = mapVertexNeighbors[k].at(p);

//                if (!prev_undef[nv]) {
//                    //If vertex color was defined during last step
//                    sum += prev_val[nv];
//                    n++;
//                }
//            }

//            if (n > 0) {
//                smooth_val[k] = sum/(float)n;
//                undef[k] = false;
//            }
//        }
//    }

    //Option 2 - Inverse weighted distance smoothing operator
    VectorXd smooth_val = input.matWDistSmooth * input.vSourceColorSamples;

    //Produce final color
    transformDataToColor(smooth_val, input.matFinalVertColor, input.dThresholdX, input.dThresholdZ, input.functionHandlerColorMap);

    //int iAllTimer = allTimer.elapsed();
    //qDebug() << "All time" << iAllTimer;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceLocDataWorker::RtSourceLocDataWorker(QObject* parent)
: QThread(parent)
, m_bIsRunning(false)
, m_bIsLooping(true)
, m_iAverageSamples(1)
, m_iVisualizationType(Data3DTreeModelItemRoles::VertexBased)
, m_iMSecIntervall(50)
, m_bSurfaceDataIsInit(false)
, m_bAnnotationDataIsInit(false)
, m_dSFreq(1000.0)
{
    m_lVisualizationInfo << VisualizationInfo() << VisualizationInfo();
    m_lVisualizationInfo[0].functionHandlerColorMap = ColorMap::valueToHot;
    m_lVisualizationInfo[1].functionHandlerColorMap = ColorMap::valueToHot;
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
        if(m_lDataQ.size() < m_dSFreq) {
            m_lDataQ.push_back(data.col(i));
        } else {
            qDebug() <<"RtSourceLocDataWorker::addData - worker is full!";
            break;
        }
    }
}


//*************************************************************************************************************

void RtSourceLocDataWorker::clear()
{
    QMutexLocker locker(&m_qMutex);
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setSurfaceData(const Eigen::VectorXi& vecVertNoLeftHemi,
                                           const Eigen::VectorXi& vecVertNoRightHemi,
                                           const QVector<QVector<int> > &mapVertexNeighborsLeftHemi,
                                           const QVector<QVector<int> > &mapVertexNeighborsRightHemi,
                                           const MatrixX3f &matVertPosLeftHemi,
                                           const MatrixX3f &matVertPosRightHemi)
{
    QMutexLocker locker(&m_qMutex);

    if(vecVertNoLeftHemi.rows() == 0 || vecVertNoRightHemi.rows() == 0) {
        qDebug() << "RtSourceLocDataWorker::setSurfaceData - Surface data is empty. Returning ...";
        return;
    }

    m_lVisualizationInfo[0].vVertNo = vecVertNoLeftHemi;
    m_lVisualizationInfo[1].vVertNo = vecVertNoRightHemi;

    m_lVisualizationInfo[0].mapVertexNeighbors = mapVertexNeighborsLeftHemi;
    m_lVisualizationInfo[1].mapVertexNeighbors = mapVertexNeighborsRightHemi;

    createSmoothingOperator(matVertPosLeftHemi, matVertPosRightHemi);

    m_bSurfaceDataIsInit = true;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setSurfaceColor(const MatrixX3f& matSurfaceVertColorLeftHemi,
                                            const MatrixX3f& matSurfaceVertColorRightHemi)
{
    QMutexLocker locker(&m_qMutex);

    if(matSurfaceVertColorLeftHemi.size() == 0 || matSurfaceVertColorRightHemi.size() == 0) {
        qDebug() << "RtSourceLocDataWorker::setSurfaceColor - Surface color data is empty. Returning ...";
        return;
    }

    m_lVisualizationInfo[0].matOriginalVertColor = matSurfaceVertColorLeftHemi;
    m_lVisualizationInfo[1].matOriginalVertColor = matSurfaceVertColorRightHemi;
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

    m_lVisualizationInfo[0].lLabels = lLabelsLeftHemi;
    m_lVisualizationInfo[1].lLabels = lLabelsRightHemi;

    //Generate fast lookup map for each source and corresponding label
    for(qint32 i = 0; i < m_lVisualizationInfo[0].vVertNo.rows(); ++i) {
        m_lVisualizationInfo[0].mapLabelIdSources.insert(m_lVisualizationInfo[0].vVertNo(i), vecLabelIdsLeftHemi(m_lVisualizationInfo[0].vVertNo(i)));
    }

    for(qint32 i = 0; i < m_lVisualizationInfo[1].vVertNo.rows(); ++i) {
        m_lVisualizationInfo[1].mapLabelIdSources.insert(m_lVisualizationInfo[1].vVertNo(i), vecLabelIdsRightHemi(m_lVisualizationInfo[1].vVertNo(i)));
    }

    m_bAnnotationDataIsInit = true;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setNumberAverages(int iNumAvr)
{
    QMutexLocker locker(&m_qMutex);
    m_iAverageSamples = iNumAvr;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setInterval(int iMSec)
{
    QMutexLocker locker(&m_qMutex);
    m_iMSecIntervall = iMSec;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setVisualizationType(int iVisType)
{
    QMutexLocker locker(&m_qMutex);
    m_iVisualizationType = iVisType;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setColormapType(const QString& sColormapType)
{
    QMutexLocker locker(&m_qMutex);

    //Create function handler to corresponding color map function
    if(sColormapType == "Hot Negative 1") {
        m_lVisualizationInfo[0].functionHandlerColorMap = ColorMap::valueToHotNegative1;
        m_lVisualizationInfo[1].functionHandlerColorMap = ColorMap::valueToHotNegative1;
    } else if(sColormapType == "Hot") {
        m_lVisualizationInfo[0].functionHandlerColorMap = ColorMap::valueToHot;
        m_lVisualizationInfo[1].functionHandlerColorMap = ColorMap::valueToHot;
    } else if(sColormapType == "Hot Negative 2") {
        m_lVisualizationInfo[0].functionHandlerColorMap = ColorMap::valueToHotNegative2;
        m_lVisualizationInfo[1].functionHandlerColorMap = ColorMap::valueToHotNegative2;
    } else if(sColormapType == "Jet") {
        m_lVisualizationInfo[0].functionHandlerColorMap = ColorMap::valueToJet;
        m_lVisualizationInfo[1].functionHandlerColorMap = ColorMap::valueToJet;
    }
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setNormalization(const QVector3D& vecThresholds)
{
    QMutexLocker locker(&m_qMutex);
    m_lVisualizationInfo[0].dThresholdX = vecThresholds.x();
    m_lVisualizationInfo[1].dThresholdX = vecThresholds.x();

    m_lVisualizationInfo[0].dThresholdZ = vecThresholds.z();
    m_lVisualizationInfo[1].dThresholdZ = vecThresholds.z();
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setSFreq(const double dSFreq)
{
    QMutexLocker locker(&m_qMutex);

    m_dSFreq = dSFreq;
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
    m_itCurrentSample = m_lDataQ.cbegin();
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
    VectorXd t_vecAverage;

    uint iSampleCtr = 0;
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
            if(m_lDataQ.size() > 0)
                doProcessing = true;
        }

        if(doProcessing) {
            if(m_bIsLooping) {
                m_qMutex.lock();

                //Down sampling in loop mode
                if(t_vecAverage.rows() != m_lDataQ.front().rows()) {
                    t_vecAverage = *m_itCurrentSample;
                } else {
                    t_vecAverage += *m_itCurrentSample;
                }

                m_qMutex.unlock();
            } else {
                m_qMutex.lock();

                //Down sampling in stream mode
                if(t_vecAverage.rows() != m_lDataQ.front().rows()) {
                    t_vecAverage = m_lDataQ.front();
                } else {
                    t_vecAverage += m_lDataQ.front();
                }

                m_lDataQ.pop_front();

                m_qMutex.unlock();
            }

            m_qMutex.lock();

            m_itCurrentSample++;
            iSampleCtr++;

            //Set iterator back to the front if needed
            if(m_itCurrentSample == m_lDataQ.cend())
            {
                m_itCurrentSample = m_lDataQ.cbegin();
            }

            if(iSampleCtr % m_iAverageSamples == 0) {
                //Perform the actual interpolation and send signal
                t_vecAverage /= (double)m_iAverageSamples;

                emit newRtData(performVisualizationTypeCalculation(t_vecAverage));
                t_vecAverage = VectorXd::Zero(t_vecAverage.rows());                

                //Sleep specified amount of time
                const int timerelap = timer.elapsed();
                const int iTimeLeft = m_iMSecIntervall - timerelap;

                //reset sample counter
                iSampleCtr = 0;

                //qDebug()<<"elapsed"<<timerelap<<"diff"<<iTimeLeft;
                if(iTimeLeft > 0) {
                    QThread::msleep(iTimeLeft);
                }
            }

            m_qMutex.unlock();
        }
    }
}


//*************************************************************************************************************

QPair<MatrixX3f, MatrixX3f> RtSourceLocDataWorker::performVisualizationTypeCalculation(const VectorXd& vSourceColorSamples)
{
    //NOTE: This function is called for every new sample point and therefore must be kept highly efficient!
//    QTime allTimer;
//    allTimer.start();

    if(vSourceColorSamples.rows() != m_lVisualizationInfo[0].vVertNo.rows() + m_lVisualizationInfo[1].vVertNo.rows()) {
        qDebug() << "RtSourceLocDataWorker::performVisualizationTypeCalculation - Number of new vertex colors (" << vSourceColorSamples.rows() << ") do not match with previously set number of vertices (" << m_lVisualizationInfo[0].vVertNo.rows() + m_lVisualizationInfo[1].vVertNo.rows() << "). Returning...";
        QPair<MatrixX3f, MatrixX3f> colorPair;
        colorPair.first =  m_lVisualizationInfo[0].matOriginalVertColor;
        colorPair.second = m_lVisualizationInfo[1].matOriginalVertColor;
        return colorPair;
    }

    if(!m_bSurfaceDataIsInit) {
        qDebug() << "RtSourceLocDataWorker::performVisualizationTypeCalculation - Surface data was not initialized. Returning ...";
        QPair<MatrixX3f, MatrixX3f> colorPair;
        colorPair.first =  m_lVisualizationInfo[0].matOriginalVertColor;
        colorPair.second = m_lVisualizationInfo[1].matOriginalVertColor;
        return colorPair;
    }

    if(!m_bAnnotationDataIsInit) {
        qDebug() << "RtSourceLocDataWorker::performVisualizationTypeCalculation - Annotation data was not initialized. Returning ...";
        QPair<MatrixX3f, MatrixX3f> colorPair;
        colorPair.first =  m_lVisualizationInfo[0].matOriginalVertColor;
        colorPair.second = m_lVisualizationInfo[1].matOriginalVertColor;
        return colorPair;
    }

    //Cut out left and right hemisphere from source data
    m_lVisualizationInfo[0].vSourceColorSamples = vSourceColorSamples.segment(0, m_lVisualizationInfo[0].vVertNo.rows());
    m_lVisualizationInfo[1].vSourceColorSamples = vSourceColorSamples.segment(m_lVisualizationInfo[0].vVertNo.rows(), m_lVisualizationInfo[1].vVertNo.rows());

    //Reset to original color as default
    m_lVisualizationInfo[0].matFinalVertColor = m_lVisualizationInfo[0].matOriginalVertColor;
    m_lVisualizationInfo[1].matFinalVertColor = m_lVisualizationInfo[1].matOriginalVertColor;

    //Generate color data for vertices
    switch(m_iVisualizationType) {
        case Data3DTreeModelItemRoles::VertexBased: {
            QFuture<void> future = QtConcurrent::map(m_lVisualizationInfo, generateColorsPerVertex);
            future.waitForFinished();

            break;
        }

        case Data3DTreeModelItemRoles::AnnotationBased: {
            QFuture<void> future = QtConcurrent::map(m_lVisualizationInfo, generateColorsPerAnnotation);
            future.waitForFinished();

            break;
        }

        case Data3DTreeModelItemRoles::SmoothingBased: {
            QFuture<void> future = QtConcurrent::map(m_lVisualizationInfo, generateSmoothedColors);
            future.waitForFinished();

            break;
        }
    }

//    int iAllTimer = allTimer.elapsed();
//    qDebug() << "All time" << iAllTimer;

    QPair<MatrixX3f, MatrixX3f> colorPair;
    colorPair.first =  m_lVisualizationInfo[0].matFinalVertColor;
    colorPair.second = m_lVisualizationInfo[1].matFinalVertColor;
    return colorPair;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::createSmoothingOperator(const MatrixX3f& matVertPosLeftHemi, const MatrixX3f& matVertPosRightHemi)
{
//    QTime timer;
//    timer.start();

    //Create smooth operator in multi thread
    QList<SmoothOperatorInfo> inputData;

    SmoothOperatorInfo leftHemi;
    m_lVisualizationInfo[0].matWDistSmooth.resize(matVertPosLeftHemi.rows(), m_lVisualizationInfo[0].vVertNo.rows());
    leftHemi.sparseSmoothMatrix = m_lVisualizationInfo[0].matWDistSmooth;
    leftHemi.vecVertNo = m_lVisualizationInfo[0].vVertNo;
    leftHemi.matVertPos = matVertPosLeftHemi;
    leftHemi.iDistPow = 3;
    leftHemi.dThresholdDistance = 0.032;
    inputData.append(leftHemi);

    SmoothOperatorInfo rightHemi;
    m_lVisualizationInfo[1].matWDistSmooth.resize(matVertPosRightHemi.rows(), m_lVisualizationInfo[1].vVertNo.rows());
    rightHemi.sparseSmoothMatrix = m_lVisualizationInfo[1].matWDistSmooth;
    rightHemi.vecVertNo = m_lVisualizationInfo[1].vVertNo;
    rightHemi.matVertPos = matVertPosRightHemi;
    rightHemi.iDistPow = 3;
    rightHemi.dThresholdDistance = 0.032;
    inputData.append(rightHemi);

    QFuture<void> future = QtConcurrent::map(inputData, generateSmoothOperator);
    future.waitForFinished();

    m_lVisualizationInfo[0].matWDistSmooth = inputData.at(0).sparseSmoothMatrix;
    m_lVisualizationInfo[1].matWDistSmooth = inputData.at(1).sparseSmoothMatrix;

//    qDebug() << "RtSourceLocDataWorker::setSmootingInfo - time needed for smooth operator creation:" << timer.elapsed();

//    qDebug() << "non zero left " << m_lVisualizationInfo[0].matWDistSmooth.nonZeros();
//    qDebug() << "non zero right " << m_lVisualizationInfo[1].matWDistSmooth.nonZeros();

//    MatrixXd a;
//    a = MatrixXd(m_sparseSmoothMatrixLeftHemi);
//    UTILSLIB::IOUtils::write_eigen_matrix(a, "m_sparseSmoothMatrixLeftHemi.txt");

//    MatrixXd b;
//    b = MatrixXd(m_sparseSmoothMatrixRightHemi);
//    UTILSLIB::IOUtils::write_eigen_matrix(b, "m_sparseSmoothMatrixRightHemi.txt");
}


