//=============================================================================================================
/**
 * @file     rtsensordatacontroller.cpp
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
 * @brief    RtSensorDataController class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensordatacontroller.h"
#include "rtsensorinterpolationmatworker.h"
#include "rtsensordataworker.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorDataController::RtSensorDataController()
: m_iMSecInterval(17)
{
       //Stream data
       m_pRtSensorDataWorker = new RtSensorDataWorker();
       m_pRtSensorDataWorker->moveToThread(&m_rtSensorDataWorkerThread);

       connect(&m_rtSensorDataWorkerThread, &QThread::finished,
               m_pRtSensorDataWorker.data(), &QObject::deleteLater);

       connect(m_pRtSensorDataWorker.data(), &RtSensorDataWorker::newRtRawData,
               this, &RtSensorDataController::onNewRtRawData);

       connect(m_pRtSensorDataWorker.data(), &RtSensorDataWorker::newRtSmoothedData,
               this, &RtSensorDataController::onNewSmoothedRtRawData);

       connect(&m_timer, &QTimer::timeout,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::streamData);

       connect(this, &RtSensorDataController::rawDataChanged,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::addData);

       connect(this, &RtSensorDataController::numberVerticesChanged,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::setNumberVertices);

       connect(this, &RtSensorDataController::newInterpolationMatrixAvailable,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::setInterpolationMatrix, Qt::DirectConnection);

       connect(this, &RtSensorDataController::thresholdsChanged,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::setThresholds, Qt::DirectConnection);

       connect(this, &RtSensorDataController::sFreqChanged,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::setSFreq);

       connect(this, &RtSensorDataController::loopStateChanged,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::setLoopState);

       connect(this, &RtSensorDataController::numberAveragesChanged,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::setNumberAverages, Qt::DirectConnection);

       connect(this, &RtSensorDataController::colormapTypeChanged,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::setColormapType, Qt::DirectConnection);

       connect(this, &RtSensorDataController::streamSmoothedDataChanged,
               m_pRtSensorDataWorker.data(), &RtSensorDataWorker::setStreamSmoothedData);

       m_rtSensorDataWorkerThread.start();

       //Calculate interpolation matrix
       m_pRtInterpolationWorker = new RtSensorInterpolationMatWorker();
       m_pRtInterpolationWorker->moveToThread(&m_rtInterpolationWorkerThread);

       connect(this, &RtSensorDataController::interpolationFunctionChanged,
               m_pRtInterpolationWorker.data(), &RtSensorInterpolationMatWorker::setInterpolationFunction);

       connect(&m_rtInterpolationWorkerThread, &QThread::finished,
               m_pRtInterpolationWorker.data(), &QObject::deleteLater);

       connect(this, &RtSensorDataController::cancelDistanceChanged,
               m_pRtInterpolationWorker.data(), &RtSensorInterpolationMatWorker::setCancelDistance);

       connect(m_pRtInterpolationWorker.data(), &RtSensorInterpolationMatWorker::newInterpolationMatrixCalculated,
               this, &RtSensorDataController::onNewInterpolationMatrixCalculated);

       connect(this, &RtSensorDataController::interpolationInfoChanged,
               m_pRtInterpolationWorker.data(), &RtSensorInterpolationMatWorker::setInterpolationInfo);

       connect(this, &RtSensorDataController::badChannelsChanged,
               m_pRtInterpolationWorker.data(), &RtSensorInterpolationMatWorker::setBadChannels);

       m_rtInterpolationWorkerThread.start();
}

//=============================================================================================================

RtSensorDataController::~RtSensorDataController()
{
    m_rtSensorDataWorkerThread.quit();
    m_rtSensorDataWorkerThread.wait();
    m_rtInterpolationWorkerThread.quit();
    m_rtInterpolationWorkerThread.wait();
}

//=============================================================================================================

void RtSensorDataController::setStreamingState(bool bStreamingState)
{
    if(bStreamingState) {
        m_timer.start(m_iMSecInterval);
    } else {
        m_timer.stop();
    }
}

//=============================================================================================================

void RtSensorDataController::setInterpolationFunction(const QString &sInterpolationFunction)
{
    emit interpolationFunctionChanged(sInterpolationFunction);
}

//=============================================================================================================

void RtSensorDataController::setLoopState(bool bLoopState)
{
    emit loopStateChanged(bLoopState);
}

//=============================================================================================================

void RtSensorDataController::setCancelDistance(double dCancelDist)
{
    emit cancelDistanceChanged(dCancelDist);
}

//=============================================================================================================

void RtSensorDataController::setTimeInterval(int iMSec)
{
//    if(iMSec < 17) {
//        qDebug() << "RtSensorDataController::setTimeInterval - The minimum time interval is 17mSec.";
//        iMSec = 17;
//    }

    m_iMSecInterval = iMSec;
    m_timer.setInterval(m_iMSecInterval);
}

//=============================================================================================================

void RtSensorDataController::setInterpolationInfo(const Eigen::MatrixX3f &matVertices,
                                                  const QVector<QVector<int> > &vecNeighborVertices,
                                                  const QVector<Vector3f> &vecSensorPos,
                                                  const FiffInfo &fiffInfo,
                                                  int iSensorType)
{
    emit numberVerticesChanged(matVertices.rows());

    emit interpolationInfoChanged(matVertices,
                                  vecNeighborVertices,
                                  vecSensorPos,
                                  fiffInfo,
                                  iSensorType);
}

//=============================================================================================================

void RtSensorDataController::setThresholds(const QVector3D &vecThresholds)
{
    emit thresholdsChanged(vecThresholds);
}

//=============================================================================================================

void RtSensorDataController::setColormapType(const QString &sColormapType)
{
    emit colormapTypeChanged(sColormapType);
}

//=============================================================================================================

void RtSensorDataController::setNumberAverages(int iNumAvr)
{
    emit numberAveragesChanged(iNumAvr);
}

//=============================================================================================================

void RtSensorDataController::setSFreq(double dSFreq)
{
    emit sFreqChanged(dSFreq);
}

//=============================================================================================================

void RtSensorDataController::setBadChannels(const FiffInfo &info)
{
    emit badChannelsChanged(info);
}

//=============================================================================================================

void RtSensorDataController::setStreamSmoothedData(bool bStreamSmoothedData)
{
    emit streamSmoothedDataChanged(bStreamSmoothedData);
}

//=============================================================================================================

void RtSensorDataController::addData(const MatrixXd& data)
{
    emit rawDataChanged(data);
}

//=============================================================================================================

void RtSensorDataController::onNewRtRawData(const VectorXd &vecDataVector)
{
    emit newRtRawDataAvailable(vecDataVector);
}

//=============================================================================================================

void RtSensorDataController::onNewSmoothedRtRawData(const MatrixX4f &matColorMatrix)
{
    emit newRtSmoothedDataAvailable(matColorMatrix);
}

//=============================================================================================================

void RtSensorDataController::onNewInterpolationMatrixCalculated(QSharedPointer<SparseMatrix<float> > pMatInterpolationMatrix)
{
    emit newInterpolationMatrixAvailable(pMatInterpolationMatrix);
}
