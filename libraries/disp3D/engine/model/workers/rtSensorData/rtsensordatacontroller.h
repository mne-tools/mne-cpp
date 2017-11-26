//=============================================================================================================
/**
* @file     rtsensordatacontroller.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     RtSensorDataController class declaration.
*
*/

#ifndef DISP3DLIB_RTSENSORDATACONTROLLER_H
#define DISP3DLIB_RTSENSORDATACONTROLLER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTimer>
#include <QPointer>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEBemSurface;
}

namespace FIFFLIB {
    class FiffInfo;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class RtSensorDataWorker;
class RtInterpolationMatWorker;


//=============================================================================================================
/**
* This controller organizes data streaming and interpolation matrix calculations.
*
* @brief This controller organizes data streaming and interpolation matrix calculations.
*/
class DISP3DSHARED_EXPORT RtSensorDataController : public QObject
{
    Q_OBJECT

public:
    RtSensorDataController(bool bStreamSmoothedData = true);
    ~RtSensorDataController();

public:
    void setStreamingState(bool streamingState);

    void setInterpolationFunction(const QString &sInterpolationFunction);

    void setLoopState(bool bLoopState);

    void setCancelDistance(double dCancelDist);

    //=========================================================================================================
    /**
    * Set the length in MSec to wait inbetween data samples.
    *
    * @param[in] iMSec                  The new length in milli Seconds to wait inbetween data samples.
    */
    void setTimeInterval(int iMSec);

    void setInterpolationInfo(const MNELIB::MNEBemSurface &bemSurface,
                              const QVector<Eigen::Vector3f> &vecSensorPos,
                              const FIFFLIB::FiffInfo &fiffInfo,
                              int iSensorType);

    void setSurfaceColor(const Eigen::MatrixX3f& matSurfaceVertColor);

    void setThresholds(const QVector3D &vecThresholds);

    void setColormapType(const QString &sColormapType);

    void setNumberAverages(int iNumAvr);

    void setSFreq(const double dSFreq);

    void setBadChannels(const FIFFLIB::FiffInfo &info);

    void addData(const Eigen::MatrixXd& data);

protected:
    void onNewRtRawData(const Eigen::VectorXd &vecDataVector);

    void onNewSmoothedRtRawData(const Eigen::MatrixX3f &matColorMatrix);

    void onNewInterpolationMatrixCalculated(QSharedPointer<Eigen::SparseMatrix<float> > matInterpolationOperator);

    QTimer                                  m_timer;

    QThread                                 m_rtSensorDataWorkerThread;
    QThread                                 m_rtInterpolationWorkerThread;

    QPointer<RtSensorDataWorker>            m_pRtSensorDataWorker;
    QPointer<RtInterpolationMatWorker>      m_pRtInterpolationWorker;

    int                                     m_iMSecInterval;                    /**< Length in milli Seconds to wait inbetween data samples. */

signals:
    void interpolationMatrixChanged(QSharedPointer<Eigen::SparseMatrix<float>> matInterpolationOperator);
    void interpolationInfoChanged(const MNELIB::MNEBemSurface &bemSurface,
                              const QVector<Eigen::Vector3f> &vecSensorPos,
                              const FIFFLIB::FiffInfo &fiffInfo,
                              int iSensorType);
    void surfaceColorChanged(const Eigen::MatrixX3f& matSurfaceVertColor);
    void thresholdsChanged(const QVector3D &vecThresholds);
    void sFreqChanged(double dSFreq);
    void badChannelsChanged(const FIFFLIB::FiffInfo &info);
    void numberAveragesChanged(int iNumAvr);
    void loopStateChanged(bool bLoopState);
    void streamingStateChanged(bool streamingState);
    void interpolationFunctionChanged(const QString &sInterpolationFunction);
    void colormapTypeChanged(const QString &sColormapType);
    void cancelDistanceChanged(double dCancelDist);
    void newDataReceived(const Eigen::MatrixXd& data);
    void newRtRawDataAvailable(const Eigen::VectorXd &vecDataVector);
    void newRtSmoothedDataAvailable(const Eigen::MatrixX3f &matColorMatrix);
};

} // NAMESPACE

#endif //DISP3DLIB_RTSENSORDATACONTROLLER_H
