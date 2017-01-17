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
, m_dNormalization(1.0)
, m_dNormalizationMax(10.0)
, m_bSurfaceDataIsInit(false)
, m_bAnnotationDataIsInit(false)
, m_functionHandlerColorMap(ColorMap::valueToHotNegative2)
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

    //Create function handler to corresponding color map function
    if(sColormapType == "Hot Negative 1") {
        m_functionHandlerColorMap = ColorMap::valueToHotNegative1;
    } else if(sColormapType == "Hot Negative 2") {
        m_functionHandlerColorMap = ColorMap::valueToHotNegative2;
    } else if(sColormapType == "Hot") {
        m_functionHandlerColorMap = ColorMap::valueToHot;
    }
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setNormalization(const QVector3D& vecThresholds)
{
    QMutexLocker locker(&m_qMutex);
    m_vecThresholds = vecThresholds;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::setLoop(bool looping)
{
    QMutexLocker locker(&m_qMutex);
    m_bIsLooping = looping;
}


//*************************************************************************************************************

struct SmoothOperatorInfo {
    VectorXi                vecVertNo;
    SparseMatrix<double>    sparseSmoothMatrix;
    MatrixX3f               matVertPos;
    int                     iDistPow;
    double                  dThresholdDistance;
};

struct VertexInfo {
    int                                 iVertIdx;
    QList<Eigen::Triplet<double> >      lTriplets;
    QList<QVector3D>                    lSourcePos;
    QVector3D                           vVertPos;
    int                                 iDistPow;
    double                              dThresholdDistance;
};

void generateWeightsPerVertex(VertexInfo& input)
{
    QVector3D from = input.vVertPos;
    QVector3D to;

    double dist, valueWeight;
    double dWeightsSum = 0;

    for(int j = 0; j < input.lSourcePos.size(); ++j) {
        to = input.lSourcePos.at(j);

        if(to == from) {
            dist = 0.0000000000000000000000000000000001;
        } else {
            dist = from.distanceToPoint(to);
        }

        if(dist <= input.dThresholdDistance) {
            valueWeight = abs(1/pow(dist,input.iDistPow));

            input.lTriplets.append(Eigen::Triplet<double>(input.iVertIdx, j, valueWeight));
            dWeightsSum += valueWeight;
        }
    }

    //Divide by the sum of all weights
    for(int j = 0; j < input.lTriplets.size(); ++j) {
        input.lTriplets[j] = Eigen::Triplet<double>(input.lTriplets.at(j).row(), input.lTriplets.at(j).col(), input.lTriplets.at(j).value()/dWeightsSum);
    }
}

void generateSmoothOperator(SmoothOperatorInfo& input)
{
    //Prepare data
    QList<VertexInfo> lInputData;
    VertexInfo vertInfo;
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

void RtSourceLocDataWorker::setSmootingInfo(const QMap<int, QVector<int> >& mapVertexNeighborsLeftHemi,
                                            const QMap<int, QVector<int> >& mapVertexNeighborsRightHemi,
                                            const MatrixX3f& matVertPosLeftHemi,
                                            const MatrixX3f& matVertPosRightHemi)
{
    QMutexLocker locker(&m_qMutex);

    QTime timer;
    timer.start();

    m_mapVertexNeighborsLeftHemi = mapVertexNeighborsLeftHemi;
    m_mapVertexNeighborsRightHemi = mapVertexNeighborsRightHemi;

    //Create smooth operator in multi thread
    QList<SmoothOperatorInfo> inputData;

    SmoothOperatorInfo leftHemi;
    m_sparseSmoothMatrixLeftHemi.resize(matVertPosLeftHemi.rows(), m_vecVertNoLeftHemi.rows());
    leftHemi.sparseSmoothMatrix = m_sparseSmoothMatrixLeftHemi;
    leftHemi.vecVertNo = m_vecVertNoLeftHemi;
    leftHemi.matVertPos = matVertPosLeftHemi;
    leftHemi.iDistPow = 5;
    leftHemi.dThresholdDistance = 0.02;
    inputData.append(leftHemi);

    SmoothOperatorInfo rightHemi;
    m_sparseSmoothMatrixRightHemi.resize(matVertPosRightHemi.rows(), m_vecVertNoRightHemi.rows());
    rightHemi.sparseSmoothMatrix = m_sparseSmoothMatrixRightHemi;
    rightHemi.vecVertNo = m_vecVertNoRightHemi;
    rightHemi.matVertPos = matVertPosRightHemi;
    rightHemi.iDistPow = 5;
    rightHemi.dThresholdDistance = 0.02;
    inputData.append(rightHemi);

    QFuture<void> future = QtConcurrent::map(inputData, generateSmoothOperator);
    future.waitForFinished();

    m_sparseSmoothMatrixLeftHemi = inputData.at(0).sparseSmoothMatrix;
    m_sparseSmoothMatrixRightHemi = inputData.at(1).sparseSmoothMatrix;

    qDebug() << "RtSourceLocDataWorker::setSmootingInfo - time:" << timer.elapsed();

    qDebug() << "non zero left " << m_sparseSmoothMatrixLeftHemi.nonZeros();
    qDebug() << "non zero right " << m_sparseSmoothMatrixRightHemi.nonZeros();

//    MatrixXd a;
//    a = MatrixXd(m_sparseSmoothMatrixLeftHemi);
//    UTILSLIB::IOUtils::write_eigen_matrix(a, "m_sparseSmoothMatrixLeftHemi.txt");

//    MatrixXd b;
//    b = MatrixXd(m_sparseSmoothMatrixRightHemi);
//    UTILSLIB::IOUtils::write_eigen_matrix(b, "m_sparseSmoothMatrixRightHemi.txt");
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
        qDebug() << "RtSourceLocDataWorker::performVisualizationTypeCalculation - Number of new vertex colors (" << sourceColorSamples.rows() << ") do not match with previously set number of vertices (" << m_vecVertNoLeftHemi.rows() + m_vecVertNoRightHemi.rows() << "). Returning...";
        return colorPair;
    }

    //Cut out left and right hemisphere from source data
    VectorXd sourceColorSamplesLeftHemi = sourceColorSamples.segment(0, m_vecVertNoLeftHemi.rows());
    VectorXd sourceColorSamplesRightHemi = sourceColorSamples.segment(m_vecVertNoLeftHemi.rows()+1, m_vecVertNoRightHemi.rows());

    //Generate color data for vertices
    switch(m_iVisualizationType) {
        case Data3DTreeModelItemRoles::VertexBased: {
            return generateColorsPerVertex(sourceColorSamplesLeftHemi, sourceColorSamplesRightHemi);
        }

        case Data3DTreeModelItemRoles::AnnotationBased: {
            return generateColorsPerAnnotation(sourceColorSamplesLeftHemi, sourceColorSamplesRightHemi);
        }        

        case Data3DTreeModelItemRoles::SmoothingBased: {
            colorPair.first = generateSmoothedColors(sourceColorSamplesLeftHemi,
                                                     m_vecVertNoLeftHemi,
                                                     m_arraySurfaceVertColorLeftHemi,
                                                     m_mapVertexNeighborsLeftHemi,
                                                     m_sparseSmoothMatrixLeftHemi);

            colorPair.second = generateSmoothedColors(sourceColorSamplesRightHemi,
                                                      m_vecVertNoRightHemi,
                                                      m_arraySurfaceVertColorRightHemi,
                                                      m_mapVertexNeighborsRightHemi,
                                                      m_sparseSmoothMatrixRightHemi);

            break;
        }
    }

    return colorPair;
}


//*************************************************************************************************************

QPair<QByteArray, QByteArray> RtSourceLocDataWorker::generateColorsPerVertex(const VectorXd& sourceColorSamplesLeftHemi, const VectorXd& sourceColorSamplesRightHemi)
{
    QPair<QByteArray, QByteArray> colorPair;
    colorPair.first = m_arraySurfaceVertColorLeftHemi;
    colorPair.second = m_arraySurfaceVertColorRightHemi;

    if(!m_bSurfaceDataIsInit) {
        qDebug() << "RtSourceLocDataWorker::generateColorsPerVertex - Surface data was not initialized. Returning ...";
        return colorPair;
    }

    //Left hemisphere
    QByteArray arrayCurrentVertColorLeftHemi = m_arraySurfaceVertColorLeftHemi;
    float *rawArrayCurrentVertColorLeftHemi = reinterpret_cast<float *>(arrayCurrentVertColorLeftHemi.data());
    QByteArray sourceColorSample;
    sourceColorSample.resize(3 * sizeof(float));

    //Create final QByteArray with colors based on the current anatomical information
    for(int i = 0; i < m_vecVertNoLeftHemi.rows(); ++i) {
        if(sourceColorSamplesLeftHemi(i) >= m_vecThresholds.x()) {
            transformDataToColor(sourceColorSamplesLeftHemi(i), sourceColorSample);
            const float *rawSourceColorSamplesColorLeftHemi = reinterpret_cast<const float *>(sourceColorSample.data());

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
        if(sourceColorSamplesRightHemi(i) >= m_vecThresholds.x()) {
            transformDataToColor(sourceColorSamplesRightHemi(i), sourceColorSample);
            const float *rawSourceColorSamplesColorRightHemi = reinterpret_cast<const float *>(sourceColorSample.data());

            rawArrayCurrentVertColorRightHemi[m_vecVertNoRightHemi(i)*3+0] = rawSourceColorSamplesColorRightHemi[0];
            rawArrayCurrentVertColorRightHemi[m_vecVertNoRightHemi(i)*3+1] = rawSourceColorSamplesColorRightHemi[1];
            rawArrayCurrentVertColorRightHemi[m_vecVertNoRightHemi(i)*3+2] = rawSourceColorSamplesColorRightHemi[2];
        }
    }

    colorPair.second = arrayCurrentVertColorRightHemi;

    return colorPair;
}


//*************************************************************************************************************

QPair<QByteArray, QByteArray> RtSourceLocDataWorker::generateColorsPerAnnotation(const VectorXd& sourceColorSamplesLeftHemi, const VectorXd& sourceColorSamplesRightHemi)
{
    QPair<QByteArray, QByteArray> colorPair;
    colorPair.first = m_arraySurfaceVertColorLeftHemi;
    colorPair.second = m_arraySurfaceVertColorRightHemi;

    if(!m_bAnnotationDataIsInit) {
        qDebug() << "RtSourceLocDataWorker::generateColorsPerAnnotation - Annotation data was not initialized. Returning ...";
        return colorPair;
    }

    //Find maximum actiavtion for each label
    QMap<qint32, double> vecLabelActivationLeftHemi;

    for(int i = 0; i < m_vecVertNoLeftHemi.rows(); ++i) {
        //Find out label for source
        qint32 labelIdxLeftHemi = m_mapLabelIdSourcesLeftHemi[m_vecVertNoLeftHemi(i)];

        if(fabs(sourceColorSamplesLeftHemi(i)) > fabs(vecLabelActivationLeftHemi[labelIdxLeftHemi]))
            vecLabelActivationLeftHemi.insert(labelIdxLeftHemi, sourceColorSamplesLeftHemi(i));
    }

    QMap<qint32, double> vecLabelActivationRightHemi;
    for(int i = 0; i < m_vecVertNoRightHemi.rows(); ++i) {
        //Find out label for source
        qint32 labelIdxRightHemi = m_mapLabelIdSourcesRightHemi[m_vecVertNoRightHemi(i)];

        if(fabs(sourceColorSamplesRightHemi(i)) > fabs(vecLabelActivationRightHemi[labelIdxRightHemi]))
            vecLabelActivationRightHemi.insert(labelIdxRightHemi, sourceColorSamplesRightHemi(i));
    }

    //Color all labels respectivley to their activation
    //Left hemisphere
    QByteArray arrayCurrentVertColorLeftHemi;
    arrayCurrentVertColorLeftHemi = m_arraySurfaceVertColorLeftHemi;
    QByteArray sourceColorSample;
    sourceColorSample.resize(3 * sizeof(float));

    float *rawArrayCurrentVertColorLeftHemi = reinterpret_cast<float *>(arrayCurrentVertColorLeftHemi.data());

    for(int i = 0; i<m_lLabelsLeftHemi.size(); i++) {
        FSLIB::Label labelLeftHemi = m_lLabelsLeftHemi.at(i);

        //Transform label activations to rgb colors
        //Check if value is bigger than lower threshold. If not, don't plot activation
        if(vecLabelActivationLeftHemi[labelLeftHemi.label_id] >= m_vecThresholds.x()) {
            transformDataToColor(vecLabelActivationLeftHemi[labelLeftHemi.label_id], sourceColorSample);
            float *rawArrayLabelColorsLeftHemi = reinterpret_cast<float *>(sourceColorSample.data());

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
        //Check if value is bigger than lower threshold. If not, don't plot activation
        if(vecLabelActivationRightHemi[labelRightHemi.label_id] >= m_vecThresholds.x()) {
            transformDataToColor(vecLabelActivationRightHemi[labelRightHemi.label_id], sourceColorSample);
            float *rawArrayLabelColorsRightHemi = reinterpret_cast<float *>(sourceColorSample.data());

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


//*************************************************************************************************************

QByteArray RtSourceLocDataWorker::generateSmoothedColors(const VectorXd& sourceColorSamples,
                                                         const VectorXi& vertno,
                                                         const QByteArray& arrayCurrentVertColor,
                                                         const QMap<int, QVector<int> >& mapVertexNeighbors,
                                                         const SparseMatrix<double>& matWDistSmooth)
{    
    //QTime prodDataTimer;
    //prodDataTimer.start();

//    //Option 1 - Use Matti's version. Smoothes between different source "patches".
//    //Activity is spread evenly around every source and then smoothed to neighboring source patches.
//    //Init the variables
//    int n,k,p,it,nn,nv;
//    float sum;
//    int niter = 2;

//    int nSurfaceVerts = arrayCurrentVertColor.size() / (sizeof(float) * 3);
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
    VectorXd smooth_val = matWDistSmooth * sourceColorSamples;

    //Produce final color
    QByteArray finalColors = arrayCurrentVertColor;
    transformDataToColor(smooth_val, finalColors);

    //int iAllTimer = allTimer.elapsed();
    //qDebug() << "All time" << iAllTimer;

    return finalColors;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::transformDataToColor(const VectorXd& data, QByteArray& arrayFinalVertColor)
{
    //Note: This function needs to be implemented extremley efficient. That is why we have three if clauses.
    //      Otherwise we would have to check which color map to take for each vertex.
    //QElapsedTimer timer;
    //timer.start();

    if(data.rows() != arrayFinalVertColor.size()/(3*sizeof(float))) {
        qDebug() << "RtSourceLocDataWorker::transformDataToColor - Sizes of input vectors do not match. Returning ...";
    }

    float *rawArrayColors = reinterpret_cast<float *>(arrayFinalVertColor.data());
    int idxColor = 0;
    float dSample;
    QRgb qRgb;

    for(int r = 0; r < data.rows(); ++r) {
        dSample = data(r);

        if(dSample >= m_vecThresholds.x()) {
            //Check lower and upper thresholds and normalize to one
            if(dSample >= m_vecThresholds.z()) {
                dSample = 1.0;
            } else if(dSample < m_vecThresholds.x()) {
                dSample = 0.0;
            } else {
                dSample = (dSample - m_vecThresholds.x()) / (m_vecThresholds.z() - m_vecThresholds.x());
            }

            qRgb = m_functionHandlerColorMap(dSample);

            rawArrayColors[idxColor++] = (float)qRed(qRgb)/255.0f;
            rawArrayColors[idxColor++] = (float)qGreen(qRgb)/255.0f;
            rawArrayColors[idxColor++] = (float)qBlue(qRgb)/255.0f;
        } else {
            idxColor += 3;
        }
    }

    //int elapsed = timer.elapsed();
    //qDebug()<<"RtSourceLocDataWorker::transformDataToColor - elapsed"<<elapsed;
}


//*************************************************************************************************************

void RtSourceLocDataWorker::transformDataToColor(float fSample, QByteArray& arrayFinalVertColor)
{
    //Note: This function needs to be implemented extremley efficient. That is why we have three if clauses.
    //      Otherwise we would have to check which color map to take for each vertex.
    if(arrayFinalVertColor.size()/(sizeof(float)) != 3) {
        qDebug() << "RtSourceLocDataWorker::transformDataToColor - Sizes of input QByteArray must be 3. Returning ...";
    }

    int idxColor = 0;

    float *rawArrayColors = reinterpret_cast<float *>(arrayFinalVertColor.data());

    //Check lower and upper thresholds and normalize to one
    if(fSample >= m_vecThresholds.z()) {
        fSample = 1.0;
    } else if(fSample < m_vecThresholds.x()) {
        fSample = 0.0;
    } else {
        fSample = (fSample - m_vecThresholds.x()) / (m_vecThresholds.z() - m_vecThresholds.x());
    }

    QRgb qRgb;
    qRgb = qRgb = (m_functionHandlerColorMap)(fSample);

    rawArrayColors[idxColor++] = (float)qRed(qRgb)/255.0f;
    rawArrayColors[idxColor++] = (float)qGreen(qRgb)/255.0f;
    rawArrayColors[idxColor++] = (float)qBlue(qRgb)/255.0f;
}
