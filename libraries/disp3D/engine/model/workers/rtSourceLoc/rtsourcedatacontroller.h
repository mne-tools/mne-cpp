//=============================================================================================================
/**
 * @file     rtsourcedatacontroller.h
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
 * @brief     RtSourceDataController class declaration.
 *
 */

#ifndef DISP3DLIB_RTSOURCEDATACONTROLLER_H
#define DISP3DLIB_RTSOURCEDATACONTROLLER_H

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

namespace FSLIB {
    class Label;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class RtSourceDataWorker;
class RtSourceInterpolationMatWorker;

//=============================================================================================================
/**
 * This controller organizes data streaming and interpolation matrix calculations. It only uses Queued signals in order to be thread safe with the underlying workers.
 *
 * @brief This controller organizes data streaming and interpolation matrix calculations. It only uses Queued signals in order to be thread safe with the underlying workers.
 */
class DISP3DSHARED_EXPORT RtSourceDataController : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtSourceDataController> SPtr;            /**< Shared pointer type for RtSourceDataController class. */
    typedef QSharedPointer<const RtSourceDataController> ConstSPtr; /**< Const shared pointer type for RtSourceDataController class. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    RtSourceDataController();

    //=========================================================================================================
    /**
     * Default destructor.
     */
    ~RtSourceDataController();

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
     * Sets the interpoaltion inforamtion about the surfaces etc.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] matVerticesLeft                 The surface vertices in 3D space for the left hemisphere.
     * @param[in] matVerticesRight                The surface vertices in 3D space for the right hemisphere.
     * @param[in] vecNeighborVerticesLeft         The neighbor vertices for the left hemisphere.
     * @param[in] vecNeighborVerticesRight        The neighbor vertices for the right hemisphere.
     * @param[in] vecVertNoLeftHemi               The vertex indexes for the left hemipshere.
     * @param[in] vecVertNoRightHemi              The vertex indexes for the right hemipshere.
     */
    void setInterpolationInfo(const Eigen::MatrixX3f &matVerticesLeft,
                              const Eigen::MatrixX3f &matVerticesRight,
                              const QVector<QVector<int> > &vecNeighborVerticesLeft,
                              const QVector<QVector<int> > &vecNeighborVerticesRight,
                              const Eigen::VectorXi& vecVertNoLeftHemi,
                              const Eigen::VectorXi& vecVertNoRightHemi);

    //=========================================================================================================
    /**
     * Set the color of the vertices for the left and right hemisphere.
     *
     * @param[in] matColorLeft      The color of the vertices for the left hemisphere.
     * @param[in] matColorRight     The color of the vertices for the right hemisphere.
     */
    void setSurfaceColor(const Eigen::MatrixX4f &matColorLeft,
                         const Eigen::MatrixX4f &matColorRight);

    //=========================================================================================================
    /**
     * Set annotation info.
     *
     * @param[in] vecLabelIdsLeftHemi        The labels ids for each of the left hemipshere surface vertex idx.
     * @param[in] vecLabelIdsRightHemi       The labels ids for each of the right hemipshere surface vertex idx.
     * @param[in] lLabelsLeftHemi            The label information for the left hemisphere.
     * @param[in] lLabelsRightHemi           The label information for the right hemisphere.
     * @param[in] vecVertNoLeft              The vertNos for the left hemisphere.
     * @param[in] vecVertNoRight             The vertNos for the right hemisphere.
     */
    void setAnnotationInfo(const Eigen::VectorXi &vecLabelIdsLeftHemi,
                           const Eigen::VectorXi &vecLabelIdsRightHemi,
                           const QList<FSLIB::Label> &lLabelsLeftHemi,
                           const QList<FSLIB::Label> &lLabelsRightHemi,
                           const Eigen::VectorXi &vecVertNoLeft,
                           const Eigen::VectorXi &vecVertNoRight);

    //=========================================================================================================
    /**
     * Set the normalization value.
     *
     * @param[in] vecThresholds          The new threshold values used for normalizing the data.
     */
    void setThresholds(const QVector3D &vecThresholds);

    //=========================================================================================================
    /**
     * Set the visualization type.
     *
     * @param[in] iVisType               The new visualization type.
     */
    void setVisualizationType(int iVisType);

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
     * This function is usefull, e.g., when the interpolation is done on shader (GPU) level.
     *
     * @param[in] vecDataVectorLeftHemi          The new streamed raw data for the left hemispehre.
     * @param[in] vecDataVectorRightHemi         The new streamed raw data for the right hemispehre.
     */
    void onNewRtRawData(const Eigen::VectorXd &vecDataVectorLeftHemi,
                        const Eigen::VectorXd &vecDataVectorRightHemi);

    //=========================================================================================================
    /**
     * Call this function whenever new interpolated raw data is available to be dispatched.
     *
     * @param[in] matColorMatrixLeftHemi          The new streamed interpolated raw data in form of RGB colors per vertex for the left hemisphere.
     * @param[in] matColorMatrixRightHemi         The new streamed interpolated raw data in form of RGB colors per vertex for the right hemisphere.
     */
    void onNewSmoothedRtRawData(const Eigen::MatrixX4f &matColorMatrixLeftHemi,
                                const Eigen::MatrixX4f &matColorMatrixRightHemi);

    //=========================================================================================================
    /**
     * Call this function whenever a new interpolation matrix for the left hemisphere is available to be dispatched.
     *
     * @param[in] pMatInterpolationMatrixLeftHemi          The new interpolation matrix for the left hemisphere.
     */
    void onNewInterpolationMatrixLeftCalculated(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixLeftHemi);

    //=========================================================================================================
    /**
     * Call this function whenever a new interpolation matrix for the right hemisphere is available to be dispatched.
     *
     * @param[in] pMatInterpolationMatrixRightHemi         The new interpolation matrix for the right hemisphere.
     */
    void onNewInterpolationMatrixRightCalculated(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixRightHemi);

    QTimer                                      m_timer;                                /**< The timer to control the streaming speed. */

    QThread                                     m_rtSourceDataWorkerThread;             /**< The RtSourceDataWorker thread. */
    QThread                                     m_rtInterpolationLeftWorkerThread;      /**< The RtSourceInterpolationMatWorker thread for the left hemisphere. */
    QThread                                     m_rtInterpolationRightWorkerThread;     /**< The RtSourceInterpolationMatWorker thread for the right hemisphere. */

    QPointer<RtSourceDataWorker>                m_pRtSourceDataWorker;                  /**< The pointer to the RtSourceDataWorker, which is running in the m_rtSourceDataWorkerThread thread. */
    QPointer<RtSourceInterpolationMatWorker>    m_pRtInterpolationLeftWorker;           /**< The pointer to the RtSourceInterpolationMatWorker, which is running in the m_rtInterpolationLeftHemiWorkerThread thread for the left hemisphere. */
    QPointer<RtSourceInterpolationMatWorker>    m_pRtInterpolationRightWorker;          /**< The pointer to the RtSourceInterpolationMatWorker, which is running in the m_rtInterpolationRightHemiWorkerThread thread for the right hemisphere. */

    int                                         m_iMSecInterval;                        /**< Length in milli Seconds to wait inbetween data samples. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the interpolation info for the left hemisphere changed.
     *
     * @param[in] matVerticesLeft               The mesh information in form of vertices.
     * @param[in] vecNeighborVerticesLeft       The neighbor vertex information.
     * @param[in] vecMappedSubsetLeft           Vector index position represents the id of the sensor and the qint in each cell is the vertex it is mapped to.
     */
    void interpolationInfoLeftChanged(const Eigen::MatrixX3f &matVerticesLeft,
                                      const QVector<QVector<int> > &vecNeighborVerticesLeft,
                                      const QVector<int> &vecMappedSubsetLeft);

    //=========================================================================================================
    /**
     * Emit this signal whenever the interpolation info for the right hemisphere changed.
     *
     * @param[in] matVerticesRight               The mesh information in form of vertices.
     * @param[in] vecNeighborVerticesRight       The neighbor vertex information.
     * @param[in] vecMappedSubsetRight           Vector index position represents the id of the sensor and the qint in each cell is the vertex it is mapped to.
     */
    void interpolationInfoRightChanged(const Eigen::MatrixX3f &matVerticesRight,
                                       const QVector<QVector<int> > &vecNeighborVerticesRight,
                                       const QVector<int> &vecMappedSubsetRight);

    //=========================================================================================================
    /**
     * Emit this signal whenever the interpolation info for the left hemisphere changed.
     *
     * @param[in] vecLabelIdsLeftHemi        The labels ids for each of the left hemipshere surface vertex idx.
     * @param[in] lLabels                    The label information.
     * @param[in] vecVertNoLeft              The vertNos for the left hemisphere.
     */
    void annotationInfoLeftChanged(const Eigen::VectorXi &vecLabelIdsLeftHemi,
                                   const  QList<FSLIB::Label> &lLabels,
                                   const Eigen::VectorXi &vecVertNoLeft);

    //=========================================================================================================
    /**
     * Emit this signal whenever the interpolation info for the right hemisphere changed.
     *
     * @param[in] vecLabelIdsRightHemi       The labels ids for each of the right hemipshere surface vertex idx.
     * @param[in] lLabels                    The label information.
     * @param[in] vecVertNoRight             The vertNos for the right hemisphere.
     */
    void annotationInfoRightChanged(const Eigen::VectorXi &vecLabelIdsRightHemi,
                                    const  QList<FSLIB::Label> &lLabels,
                                    const Eigen::VectorXi &vecVertNoRight);

    //=========================================================================================================
    /**
     * Emit this signal whenever the color of the vertices for the left and right hemisphere changed.
     *
     * @param[in] matColorLeft      The color of the vertices for the left hemisphere.
     * @param[in] matColorRight     The color of the vertices for the right hemisphere.
     */
    void surfaceColorChanged(const Eigen::MatrixX4f &matColorLeft,
                             const Eigen::MatrixX4f &matColorRight);

    //=========================================================================================================
    /**
     * Emit this signal whenever the fiff info changed.
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
     * Emit this signal whenever the thresholds changed.
     *
     * @param[in] vecThresholds          The new threshold values used for normalizing the data.
     */
    void thresholdsChanged(const QVector3D &vecThresholds);

    //=========================================================================================================
    /**
     * Emit this signal whenever the visualization type changed.
     *
     * @param[in] iVisType          The new visualization type.
     */
    void visualizationTypeChanged(int iVisType);

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
     * Emit this signal whenever a new interpolation matrix for the left hemisphere is available.
     *
     * @param[in] pMatInterpolationMatrixLeftHemi          The new interpolation matrix for the left hemisphere.
     */
    void newInterpolationMatrixLeftAvailable(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixLeftHemi);

    //=========================================================================================================
    /**
     * Emit this signal whenever a new interpolation matrix for the right hemisphere is available.
     *
     * @param[in] pMatInterpolationMatrixRightHemi         The new interpolation matrix for the right hemisphere.
     */
    void newInterpolationMatrixRightAvailable(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixRightHemi);

    //=========================================================================================================
    /**
     * Emit this signal whenever a new raw data is streamed.
     * This function is usefull, e.g., when the interpolation is done on shader (GPU) level.
     *
     * @param[in] vecDataVectorLeftHemi          The new streamed raw data for the left hemispehre.
     * @param[in] vecDataVectorRightHemi         The new streamed raw data for the right hemispehre.
     */
    void newRtRawDataAvailable(const Eigen::VectorXd &vecDataVectorLeftHemi,
                               const Eigen::VectorXd &vecDataVectorRightHemi);

    //=========================================================================================================
    /**
     * Emit this signal whenever a new interpolated raw data is streamed.
     *
     * @param[in] matColorMatrixLeftHemi          The new streamed interpolated raw data in form of RGB colors per vertex for the left hemisphere.
     * @param[in] matColorMatrixRightHemi         The new streamed interpolated raw data in form of RGB colors per vertex for the right hemisphere.
     */
    void newRtSmoothedDataAvailable(const Eigen::MatrixX4f &matColorMatrixLeftHemi,
                                    const Eigen::MatrixX4f &matColorMatrixRightHemi);
};
} // NAMESPACE

#endif //DISP3DLIB_RTSOURCEDATACONTROLLER_H
