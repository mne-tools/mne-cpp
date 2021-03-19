//=============================================================================================================
/**
 * @file     rtsensordatacontroller.h
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
 * @brief     RtSensorDataController class declaration.
 *
 */

#ifndef DISP3DLIB_RTSENSORDATACONTROLLER_H
#define DISP3DLIB_RTSENSORDATACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTimer>
#include <QPointer>
#include <QThread>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class RtSensorDataWorker;
class RtSensorInterpolationMatWorker;

//=============================================================================================================
/**
 * This controller organizes data streaming and interpolation matrix calculations. It only uses Queued signals in order to be thread safe with the underlying workers.
 *
 * @brief This controller organizes data streaming and interpolation matrix calculations. It only uses Queued signals in order to be thread safe with the underlying workers.
 */
class DISP3DSHARED_EXPORT RtSensorDataController : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtSensorDataController> SPtr;            /**< Shared pointer type for RtSensorDataController class. */
    typedef QSharedPointer<const RtSensorDataController> ConstSPtr; /**< Const shared pointer type for RtSensorDataController class. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    RtSensorDataController();

    //=========================================================================================================
    /**
     * Default destructor.
     */
    ~RtSensorDataController();

    //=========================================================================================================
    /**
     * Set the streaming state (start/stop streaming).
     *
     * @param[in] bStreamingState                The new straming state.
     */
    void setStreamingState(bool bStreamingState);

    //=========================================================================================================
    /**
     * This function sets the function that is used in the interpolation process.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] sInterpolationFunction     Function that computes interpolation coefficients using the distance values.
     */
    void setInterpolationFunction(const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
     * Set the loop functionality on or off.
     *
     * @param[in] bLoopState                The new looping state.
     */
    void setLoopState(bool bLoopState);

    //=========================================================================================================
    /**
     * This function sets the cancel distance used in distance calculations for the interpolation.
     * Distances higher than this are ignored, i.e. the respective coefficients are set to zero.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] dCancelDist           The new cancel distance value in meters.
     */
    void setCancelDistance(double dCancelDist);

    //=========================================================================================================
    /**
     * Set the time in MSec to wait inbetween data samples.
     *
     * @param[in] iMSec                  The new length in milli Seconds to wait inbetween data samples.
     */
    void setTimeInterval(int iMSec);

    //=========================================================================================================
    /**
     * Sets the members InterpolationData.bemSurface, InterpolationData.vecSensorPos and m_numSensors.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] matVertices               The vertex information.
     * @param[in] vecNeighborVertices       The neighbor vertex information.
     * @param[in] vecSensorPos              The QVector that holds the sensor positons in x, y and z coordinates.
     * @param[in] fiffEvoked                Holds all information about the sensors.
     * @param[in] iSensorType               Type of the sensor: FIFFV_EEG_CH or FIFFV_MEG_CH.
     *
     * @return Returns the created interpolation matrix.
     */
    void setInterpolationInfo(const Eigen::MatrixX3f &matVertices,
                              const QVector<QVector<int> > &vecNeighborVertices,
                              const QVector<Eigen::Vector3f> &vecSensorPos,
                              const FIFFLIB::FiffInfo &fiffInfo,
                              int iSensorType);

    //=========================================================================================================
    /**
     * Set the normalization value.
     *
     * @param[in] vecThresholds          The new threshold values used for normalizing the data.
     */
    void setThresholds(const QVector3D &vecThresholds);

    //=========================================================================================================
    /**
     * Set the type of the colormap.
     *
     * @param[in] sColormapType          The new colormap type.
     */
    void setColormapType(const QString &sColormapType);

    //=========================================================================================================
    /**
     * Set the number of averages.
     *
     * @param[in] iNumAvr                The new number of averages.
     */
    void setNumberAverages(int iNumAvr);

    //=========================================================================================================
    /**
     * Set the sampling frequency.
     *
     * @param[in] dSFreq                 The new sampling frequency.
     */
    void setSFreq(double dSFreq);

    //=========================================================================================================
    /**
     * Sets bad channels and recalculate interpolation matrix.
     *
     * @param[in] info                 The fiff info including the new bad channels.
     */
    void setBadChannels(const FIFFLIB::FiffInfo &info);

    //=========================================================================================================
    /**
     * Sets the state whether to stream smoothed or raw data
     *
     * @param[in] bStreamSmoothedData                 The new state.
     */
    void setStreamSmoothedData(bool bStreamSmoothedData);

    //=========================================================================================================
    /**
     * Add data which is to be streamed.
     *
     * @param[in] data         The new data.
     */
    void addData(const Eigen::MatrixXd& data);

protected:
    //=========================================================================================================
    /**
     * Call this function whenever new raw data is available to be dispatched.
     *
     * @param[in] vecDataVector         The new raw data.
     */
    void onNewRtRawData(const Eigen::VectorXd &vecDataVector);

    //=========================================================================================================
    /**
     * Call this function whenever new interpolated raw data is available to be dispatched.
     *
     * @param[in] matColorMatrix         The new interpolated data as RGB colors per vertex.
     */
    void onNewSmoothedRtRawData(const Eigen::MatrixX4f &matColorMatrix);

    //=========================================================================================================
    /**
     * Call this function whenever a new interpolation matrix is available to be dispatched.
     *
     * @param[in] pMatInterpolationMatrix         The new interpolation matrix data.
     */
    void onNewInterpolationMatrixCalculated(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrix);

    QTimer                                  m_timer;                            /**< The timer to control the streaming speed. */

    QThread                                 m_rtSensorDataWorkerThread;         /**< The RtSensorDataWorker thread. */
    QThread                                 m_rtInterpolationWorkerThread;      /**< The RtSensorInterpolationMatWorker thread. */

    QPointer<RtSensorDataWorker>            m_pRtSensorDataWorker;              /**< The pointer to the RtSensorDataWorker, which is running in the RtSensorDataWorker thread. */
    QPointer<RtSensorInterpolationMatWorker>      m_pRtInterpolationWorker;           /**< The pointer to the RtSensorInterpolationMatWorker, which is running in the RtSensorInterpolationMatWorker thread. */

    int                                     m_iMSecInterval;                    /**< Length in milli Seconds to wait inbetween data samples. */
signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the interpolation info changed.
     *
     * @param[in] matVertices               The vertex information.
     * @param[in] vecNeighborVertices       The neighbor vertex information.
     * @param[in] vecSensorPos              The QVector that holds the sensor positons in x, y and z coordinates.
     * @param[in] fiffEvoked                Holds all information about the sensors.
     * @param[in] iSensorType               Type of the sensor: FIFFV_EEG_CH or FIFFV_MEG_CH.
     */
    void interpolationInfoChanged(const Eigen::MatrixX3f &matVertices,
                                  const QVector<QVector<int> > &vecNeighborVertices,
                                  const QVector<Eigen::Vector3f> &vecSensorPos,
                                  const FIFFLIB::FiffInfo &fiffInfo,
                                  int iSensorType);

    //=========================================================================================================
    /**
     * Emit this signal whenever the number of vertices changed.
     *
     * @param[in] iNumberVerts                The new number of vertices.
     */
    void numberVerticesChanged(int iNumberVerts);

    //=========================================================================================================
    /**
     * Emit this signal whenever the bad channels changed.
     *
     * @param[in] info                 The fiff info including the new bad channels.
     */
    void badChannelsChanged(const FIFFLIB::FiffInfo &info);

    //=========================================================================================================
    /**
     * Emit this signal whenever the state to whether stream smoothed/interpolated or raw data changed.
     *
     * @param[in] info                 The fiff info including the new bad channels.
     */
    void streamSmoothedDataChanged(bool bStreamSmoothedData);

    //=========================================================================================================
    /**
     * Emit this signal whenever the interpolation function changed.
     *
     * @param[in] sInterpolationFunction     Function that computes interpolation coefficients using the distance values.
     */
    void interpolationFunctionChanged(const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
     * Emit this signal whenever the cancel distance changed.
     *
     * @param[in] dCancelDist           The new cancel distance value in meters.
     */
    void cancelDistanceChanged(double dCancelDist);

    //=========================================================================================================
    /**
     * Emit this signal whenever the surface color changed.
     *
     * @param[in] matSurfaceVertColor      The vertex colors on which the streamed data should be plotted.
     */
    void surfaceColorChanged(const Eigen::MatrixX3f& matSurfaceVertColor);

    //=========================================================================================================
    /**
     * Emit this signal whenever the thresholds changed.
     *
     * @param[in] vecThresholds          The new threshold values used for normalizing the data.
     */
    void thresholdsChanged(const QVector3D &vecThresholds);

    //=========================================================================================================
    /**
     * Emit this signal whenever the sampling frequency changed.
     *
     * @param[in] dSFreq                 The new sampling frequency.
     */
    void sFreqChanged(double dSFreq);

    //=========================================================================================================
    /**
     * Emit this signal whenever the number of averages changed.
     *
     * @param[in] iNumAvr                The new number of averages.
     */
    void numberAveragesChanged(int iNumAvr);

    //=========================================================================================================
    /**
     * Emit this signal whenever the loop state changed.
     *
     * @param[in] bLoopState                The new looping state.
     */
    void loopStateChanged(bool bLoopState);

    //=========================================================================================================
    /**
     * Emit this signal whenever the colormap changed.
     *
     * @param[in] sColormapType          The new colormap type.
     */
    void colormapTypeChanged(const QString &sColormapType);

    //=========================================================================================================
    /**
     * Emit this signal whenever new data to be streamed was added.
     *
     * @param[in] data          The new data.
     */
    void rawDataChanged(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
     * Emit this signal whenever a new interpolation matrix is available.
     *
     * @param[in] pMatInterpolationMatrix          The new interpolation matrix.
     */
    void newInterpolationMatrixAvailable(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrix);

    //=========================================================================================================
    /**
     * Emit this signal whenever a new raw data is streamed.
     *
     * @param[in] vecDataVector          The new streamed raw data.
     */
    void newRtRawDataAvailable(const Eigen::VectorXd &vecDataVector);

    //=========================================================================================================
    /**
     * Emit this signal whenever a new interpolated raw data is streamed.
     *
     * @param[in] matColorMatrix          The new streamed interpolated raw data in form of RGB colors per vertex.
     */
    void newRtSmoothedDataAvailable(const Eigen::MatrixX4f &matColorMatrix);
};
} // NAMESPACE

#endif //DISP3DLIB_RTSENSORDATACONTROLLER_H
