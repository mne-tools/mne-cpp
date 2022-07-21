//=============================================================================================================
/**
 * @file     rtsourcedatacontroller.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch. All rights reserved.
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
 * @brief    RtSourceDataController class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourcedatacontroller.h"
#include "rtsourceinterpolationmatworker.h"
#include "rtsourcedataworker.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceDataController::RtSourceDataController()
: m_iMSecInterval(17)
{
       //Stream data
       m_pRtSourceDataWorker = new RtSourceDataWorker();
       m_pRtSourceDataWorker->moveToThread(&m_rtSourceDataWorkerThread);

       connect(&m_rtSourceDataWorkerThread, &QThread::finished,
               m_pRtSourceDataWorker.data(), &QObject::deleteLater);

       connect(m_pRtSourceDataWorker.data(), &RtSourceDataWorker::newRtRawData,
               this, &RtSourceDataController::onNewRtRawData, Qt::BlockingQueuedConnection);

       connect(m_pRtSourceDataWorker.data(), &RtSourceDataWorker::newRtSmoothedData,
               this, &RtSourceDataController::onNewSmoothedRtRawData, Qt::BlockingQueuedConnection);

       connect(&m_timer, &QTimer::timeout,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::streamData);

       connect(this, &RtSourceDataController::rawDataChanged,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::addData, Qt::DirectConnection);

       connect(this, &RtSourceDataController::surfaceColorChanged,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::setSurfaceColor);

       connect(this, &RtSourceDataController::newInterpolationMatrixLeftAvailable,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::setInterpolationMatrixLeft, Qt::DirectConnection);

       connect(this, &RtSourceDataController::newInterpolationMatrixRightAvailable,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::setInterpolationMatrixRight, Qt::DirectConnection);

       connect(this, &RtSourceDataController::thresholdsChanged,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::setThresholds, Qt::DirectConnection);

       connect(this, &RtSourceDataController::sFreqChanged,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::setSFreq);

       connect(this, &RtSourceDataController::loopStateChanged,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::setLoopState, Qt::DirectConnection);

       connect(this, &RtSourceDataController::numberAveragesChanged,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::setNumberAverages, Qt::DirectConnection);

       connect(this, &RtSourceDataController::colormapTypeChanged,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::setColormapType, Qt::DirectConnection);

       connect(this, &RtSourceDataController::streamSmoothedDataChanged,
               m_pRtSourceDataWorker.data(), &RtSourceDataWorker::setStreamSmoothedData);

       m_rtSourceDataWorkerThread.start();

       //Calculate interpolation matrix left hemisphere
       m_pRtInterpolationLeftWorker = new RtSourceInterpolationMatWorker();
       m_pRtInterpolationLeftWorker->moveToThread(&m_rtInterpolationLeftWorkerThread);

       connect(this, &RtSourceDataController::interpolationFunctionChanged,
               m_pRtInterpolationLeftWorker.data(), &RtSourceInterpolationMatWorker::setInterpolationFunction);

       connect(&m_rtInterpolationLeftWorkerThread, &QThread::finished,
               m_pRtInterpolationLeftWorker.data(), &QObject::deleteLater);

       connect(this, &RtSourceDataController::cancelDistanceChanged,
               m_pRtInterpolationLeftWorker.data(), &RtSourceInterpolationMatWorker::setCancelDistance);

       connect(m_pRtInterpolationLeftWorker.data(), &RtSourceInterpolationMatWorker::newInterpolationMatrixCalculated,
               this, &RtSourceDataController::onNewInterpolationMatrixLeftCalculated);

       connect(this, &RtSourceDataController::interpolationInfoLeftChanged,
               m_pRtInterpolationLeftWorker.data(), &RtSourceInterpolationMatWorker::setInterpolationInfo);

       connect(this, &RtSourceDataController::annotationInfoLeftChanged,
               m_pRtInterpolationLeftWorker.data(), &RtSourceInterpolationMatWorker::setAnnotationInfo);

       connect(this, &RtSourceDataController::visualizationTypeChanged,
               m_pRtInterpolationLeftWorker.data(), &RtSourceInterpolationMatWorker::setVisualizationType, Qt::DirectConnection);

       m_rtInterpolationLeftWorkerThread.start();

       //Calculate interpolation matrix right hemisphere
       m_pRtInterpolationRightWorker = new RtSourceInterpolationMatWorker();
       m_pRtInterpolationRightWorker->moveToThread(&m_rtInterpolationRightWorkerThread);

       connect(this, &RtSourceDataController::interpolationFunctionChanged,
               m_pRtInterpolationRightWorker.data(), &RtSourceInterpolationMatWorker::setInterpolationFunction);

       connect(&m_rtInterpolationRightWorkerThread, &QThread::finished,
               m_pRtInterpolationRightWorker.data(), &QObject::deleteLater);

       connect(this, &RtSourceDataController::cancelDistanceChanged,
               m_pRtInterpolationRightWorker.data(), &RtSourceInterpolationMatWorker::setCancelDistance);

       connect(m_pRtInterpolationRightWorker.data(), &RtSourceInterpolationMatWorker::newInterpolationMatrixCalculated,
               this, &RtSourceDataController::onNewInterpolationMatrixRightCalculated);

       connect(this, &RtSourceDataController::interpolationInfoRightChanged,
               m_pRtInterpolationRightWorker.data(), &RtSourceInterpolationMatWorker::setInterpolationInfo);

       connect(this, &RtSourceDataController::annotationInfoRightChanged,
               m_pRtInterpolationRightWorker.data(), &RtSourceInterpolationMatWorker::setAnnotationInfo);

       connect(this, &RtSourceDataController::visualizationTypeChanged,
               m_pRtInterpolationRightWorker.data(), &RtSourceInterpolationMatWorker::setVisualizationType, Qt::DirectConnection);

       m_rtInterpolationRightWorkerThread.start();
}

//=============================================================================================================

RtSourceDataController::~RtSourceDataController()
{
    m_rtSourceDataWorkerThread.quit();
    m_rtSourceDataWorkerThread.wait();
    m_rtInterpolationLeftWorkerThread.quit();
    m_rtInterpolationLeftWorkerThread.wait();
    m_rtInterpolationRightWorkerThread.quit();
    m_rtInterpolationRightWorkerThread.wait();
}

//=============================================================================================================

void RtSourceDataController::setStreamingState(bool bStreamingState)
{
    if(bStreamingState) {
        m_timer.start(m_iMSecInterval);
    } else {
        m_timer.stop();
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationFunction(const QString &sInterpolationFunction)
{
    emit interpolationFunctionChanged(sInterpolationFunction);
}

//=============================================================================================================

void RtSourceDataController::setLoopState(bool bLoopState)
{
    emit loopStateChanged(bLoopState);
}

//=============================================================================================================

void RtSourceDataController::setCancelDistance(double dCancelDist)
{
    emit cancelDistanceChanged(dCancelDist);
}

//=============================================================================================================

void RtSourceDataController::setTimeInterval(int iMSec)
{
//    if(iMSec < 17) {
//        qDebug() << "RtSourceDataController::setTimeInterval - The minimum time interval is 17mSec.";
//        iMSec = 17;
//    }

    m_iMSecInterval = iMSec;
    m_timer.setInterval(m_iMSecInterval);
}

//=============================================================================================================

void RtSourceDataController::setInterpolationInfo(const MatrixX3f &matVerticesLeft,
                                                  const MatrixX3f &matVerticesRight,
                                                  const QVector<QVector<int> > &vecNeighborVerticesLeft,
                                                  const QVector<QVector<int> > &vecNeighborVerticesRight,
                                                  const VectorXi &vecVertNoLeftHemi,
                                                  const VectorXi &vecVertNoRightHemi)
{
    QVector<int> vecMappedSubsetLeft, vecMappedSubsetRight;

    for(int i = 0; i < vecVertNoLeftHemi.rows(); ++i) {
        vecMappedSubsetLeft.append(vecVertNoLeftHemi[i]);
    }

    for(int i = 0; i < vecVertNoRightHemi.rows(); ++i) {
        vecMappedSubsetRight.append(vecVertNoRightHemi[i]);
    }

    emit interpolationInfoLeftChanged(matVerticesLeft,
                                      vecNeighborVerticesLeft,
                                      vecMappedSubsetLeft);

    emit interpolationInfoRightChanged(matVerticesRight,
                                       vecNeighborVerticesRight,
                                       vecMappedSubsetRight);
}

//=============================================================================================================

void RtSourceDataController::setSurfaceColor(const MatrixX4f &matColorLeft,
                                             const MatrixX4f &matColorRight)
{
    emit surfaceColorChanged(matColorLeft,
                             matColorRight);
}

//=============================================================================================================

void RtSourceDataController::setAnnotationInfo(const VectorXi &vecLabelIdsLeftHemi,
                                               const VectorXi &vecLabelIdsRightHemi,
                                               const QList<Label> &lLabelsLeftHemi,
                                               const QList<Label> &lLabelsRightHemi,
                                               const VectorXi &vecVertNoLeft,
                                               const VectorXi &vecVertNoRight)
{
    emit annotationInfoLeftChanged(vecLabelIdsLeftHemi,
                                   lLabelsLeftHemi,
                                   vecVertNoLeft);

    emit annotationInfoRightChanged(vecLabelIdsRightHemi,
                                    lLabelsRightHemi,
                                    vecVertNoRight);
}

//=============================================================================================================

void RtSourceDataController::setThresholds(const QVector3D &vecThresholds)
{
    emit thresholdsChanged(vecThresholds);
}

//=============================================================================================================

void RtSourceDataController::setVisualizationType(int iVisType)
{
    emit visualizationTypeChanged(iVisType);
}

//=============================================================================================================

void RtSourceDataController::setColormapType(const QString &sColormapType)
{
    emit colormapTypeChanged(sColormapType);
}

//=============================================================================================================

void RtSourceDataController::setNumberAverages(int iNumAvr)
{
    emit numberAveragesChanged(iNumAvr);
}

//=============================================================================================================

void RtSourceDataController::setSFreq(double dSFreq)
{
    emit sFreqChanged(dSFreq);
}

//=============================================================================================================

void RtSourceDataController::setStreamSmoothedData(bool bStreamSmoothedData)
{
    emit streamSmoothedDataChanged(bStreamSmoothedData);
}

//=============================================================================================================

void RtSourceDataController::addData(const MatrixXd& data)
{
    emit rawDataChanged(data);
}

//=============================================================================================================

void RtSourceDataController::onNewRtRawData(const VectorXd &vecDataVectorLeftHemi,
                                            const VectorXd &vecDataVectorRightHemi)
{
    emit newRtRawDataAvailable(vecDataVectorLeftHemi,
                               vecDataVectorRightHemi);
}

//=============================================================================================================

void RtSourceDataController::onNewSmoothedRtRawData(const MatrixX4f &matColorMatrixLeftHemi,
                                                    const MatrixX4f &matColorMatrixRightHemi)
{
    emit newRtSmoothedDataAvailable(matColorMatrixLeftHemi,
                                    matColorMatrixRightHemi);
}

//=============================================================================================================

void RtSourceDataController::onNewInterpolationMatrixLeftCalculated(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixLeftHemi)
{
    emit newInterpolationMatrixLeftAvailable(pMatInterpolationMatrixLeftHemi);
}

//=============================================================================================================

void RtSourceDataController::onNewInterpolationMatrixRightCalculated(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixRightHemi)
{
    emit newInterpolationMatrixRightAvailable(pMatInterpolationMatrixRightHemi);
}

