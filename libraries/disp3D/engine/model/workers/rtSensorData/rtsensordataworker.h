//=============================================================================================================
/**
* @file     rtsensordataworker.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief     This worker calculates interpolated signals and provides SensorData items with the output
*
*/

#ifndef DISP3DLIB_RTSENSORDATAWORKER_H
#define DISP3DLIB_RTSENSORDATAWORKER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../../items/common/types.h"
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_evoked.h>
#include "../../../../helpers/geometryinfo/geometryinfo.h"
#include "../../../../helpers/interpolation/interpolation.h"
#include <mne/mne_bem_surface.h>
#include <disp/helpers/colormap.h>
#include <utils/ioutils.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QVector3D>
#include <QSharedPointer>
#include <QLinkedList>
#include <QTimer>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//=============================================================================================================

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Worker which schedules data with the right timing
*
* @brief Data scheduler
*/
class DISP3DSHARED_EXPORT RtSensorDataWorker : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtSensorDataWorker> SPtr;            /**< Shared pointer type for RtSensorDataWorker class. */
    typedef QSharedPointer<const RtSensorDataWorker> ConstSPtr; /**< Const shared pointer type for RtSensorDataWorker class. */

    //=========================================================================================================
    /**
    * The struct specifing visualization info.
    */
    struct VisualizationInfo {
        double                      dThresholdX;
        double                      dThresholdZ;

        MatrixX3f                   matOriginalVertColor;
        MatrixX3f                   matFinalVertColor;

        QRgb (*functionHandlerColorMap)(double v);
    } m_lVisualizationInfo;               /**< Container for the visualization info. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] parent      The parent of the QObject.
    */
    explicit RtSensorDataWorker(bool bStreamSmoothedData = true);

    //=========================================================================================================
    /**
    * Add data which is to be streamed.
    *
    * @param[in] data         The new data.
    */
    void addData(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
    * Clear this worker, empties the m_lData field that holds the current block of sensor activity
    */
    void clear();
    
    //=========================================================================================================
    /**
    * Set base surface color which the streamed data is plotted on.
    *
    * @param[in] matSurfaceVertColor      The vertex colors on which the streamed data should be plotted
    */
    void setSurfaceColor(const MatrixX3f &matSurfaceVertColor);

    //=========================================================================================================
    /**
    * Set the number of average to take after emitting the data to the listening threads.
    *
    * @param[in] iNumAvr                The new number of averages.
    */
    void setNumberAverages(int iNumAvr);

    //=========================================================================================================
    /**
    * Set the type of the colormap.
    *
    * @param[in] sColormapType          The new colormap type.
    */
    void setColormapType(const QString& sColormapType);

    //=========================================================================================================
    /**
    * Set the normalization value.
    *
    * @param[in] vecThresholds          The new threshold values used for normalizing the data.
    */
    void setThresholds(const QVector3D &vecThresholds);
    
    //=========================================================================================================
    /**
    * Set the loop functionality on or off.
    *
    * @param[in] bLoopState                The new looping state.
    */
    void setLoopState(bool bLoopState);

    //=========================================================================================================
    /**
    * Set the sampling frequency.
    *
    * @param[in] dSFreq                 The new sampling frequency.
    */
    void setSFreq(const double dSFreq);

    //=========================================================================================================
    /**
    * Main method of this worker: Checks whether it is time for the worker to output new data for visualization.
    * If so, it averages the specified amount of data samples and calculates the output.
    */
    void streamData();

    void setInterpolationMatrix(QSharedPointer<SparseMatrix<float>> matInterpolationOperator) {
        m_matInterpolationOperator = matInterpolationOperator;
    }

    //=========================================================================================================
    /**
     * @brief normalizeAndTransformToColor  This method normalizes final values for all vertices of the mesh and converts them to rgb using the specified color converter
     *
     * @param[in] vecData                       The final values for each vertex of the surface
     * @param[in,out] matFinalVertColor         The color matrix which the results are to be written to
     * @param[in] dThresholdX                   Lower threshold for normalizing
     * @param[in] dThreholdZ                    Upper threshold for normalizing
     * @param[in] functionHandlerColorMap       The pointer to the function which converts scalar values to rgb
     */
    void normalizeAndTransformToColor(const VectorXf& vecData, MatrixX3f& matFinalVertColor, double dThresholdX, double dThreholdZ, QRgb (*functionHandlerColorMap)(double v));

    //=========================================================================================================
    /**
     * @brief generateColorsFromSensorValues        Produces the final color matrix that is to be emitted
     *
     * @param[in] vecSensorValues                   A vector of sensor signals
     *
     * @return The final color values for the underlying mesh surface
     */
    Eigen::MatrixX3f generateColorsFromSensorValues(const Eigen::VectorXd& vecSensorValues);

    QLinkedList<Eigen::VectorXd>                        m_lDataQ;                            /**< List that holds the fiff matrix data <n_channels x n_samples>. */
    QLinkedList<Eigen::VectorXd>::const_iterator        m_itCurrentSample = 0;                  /**< Iterator to current sample which is/was streamed. */

    bool                                                m_bIsLooping;                       /**< Flag if this thread should repeat sending the same data over and over again. */
    bool                                                m_bStreamSmoothedData;              /**< Flag if this thread's streams the raw or already smoothed data. Latter are produced by multiplying the smoothing operator here in this thread. */

    int                                                 m_iAverageSamples;                  /**< Number of average to compute. */

    double                                              m_dSFreq;                           /**< The current sampling frequency. */

    QSharedPointer<SparseMatrix<float>> m_matInterpolationOperator;

    VectorXd vecAverage;
    uint iSampleCtr = 0;

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever this item should send new colors to its listeners.
    *
    * @param[in] colorMatrix     The samples data in form of rgb colors for the mesh.
    */
    void newRtRawData(const Eigen::VectorXd &vecDataVector);
    void newRtSmoothedData(const Eigen::MatrixX3f &matColorMatrix);
};





class DISP3DSHARED_EXPORT RtInterpolationMatWorker : public QObject
{
    Q_OBJECT

public:
    //=============================================================================================================
    /**
     * The struct specifing all data that is used in the interpolation process
     */
    struct InterpolationData {
        int                                     iSensorType;                      /**< Type of the sensor: FIFFV_EEG_CH or FIFFV_MEG_CH. */
        double                                  dCancelDistance;                  /**< Cancel distance for the interpolaion in meters. */

        QSharedPointer<SparseMatrix<float> >    pWeightMatrix;                    /**< Weight matrix that holds all coefficients for a signal interpolation. */
        QSharedPointer<MatrixXd>                pDistanceMatrix;                  /**< Distance matrix that holds distances from sensors positions to the near vertices in meters. */
        QSharedPointer<QVector<qint32>>         pVecMappedSubset;                 /**< Vector index position represents the id of the sensor and the qint in each cell is the vertex it is mapped to. */

        MNELIB::MNEBemSurface                   bemSurface;                       /**< Holds all vertex information that is needed (public member rr). */
        FIFFLIB::FiffInfo                       fiffInfo;                         /**< Contains all information about the sensors. */

        double (*interpolationFunction) (double);                                 /**< Function that computes interpolation coefficients using the distance values. */
    } m_lInterpolationData; /**< Container for the interpolation data. */

    RtInterpolationMatWorker() {
        //5cm cancel distance and cubic function as default
        m_lInterpolationData.dCancelDistance = 0.20;
        m_lInterpolationData.interpolationFunction = DISP3DLIB::Interpolation::cubic;
    }

    bool                m_bInterpolationInfoIsInit = false;         /**< Flag if this thread's interpoaltion data was initialized. This flag is used to decide whether specific visualization types can be computed. */

public:
    //=========================================================================================================
    /**
     * This function sets the function that is used in the interpolation process.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] sInterpolationFunction     Function that computes interpolation coefficients using the distance values.
     */
    void setInterpolationFunction(const QString &sInterpolationFunction)
    {
        if(sInterpolationFunction == "Linear") {
            m_lInterpolationData.interpolationFunction = Interpolation::linear;
        }
        else if(sInterpolationFunction == "Square") {
            m_lInterpolationData.interpolationFunction = Interpolation::square;
        }
        else if(sInterpolationFunction == "Cubic") {
            m_lInterpolationData.interpolationFunction = Interpolation::cubic;
        }
        else if(sInterpolationFunction == "Gaussian") {
            m_lInterpolationData.interpolationFunction = Interpolation::gaussian;
        }

        if(m_bInterpolationInfoIsInit == true){
            //recalculate weight matrix parameters changed
            m_lInterpolationData.pWeightMatrix = Interpolation::createInterpolationMat(m_lInterpolationData.pVecMappedSubset,
                                                                                       m_lInterpolationData.pDistanceMatrix,
                                                                                       m_lInterpolationData.interpolationFunction,
                                                                                       m_lInterpolationData.dCancelDistance,
                                                                                       m_lInterpolationData.fiffInfo,
                                                                                       m_lInterpolationData.iSensorType);

            emit newInterpolationMatrixCalculated(m_lInterpolationData.pWeightMatrix);
        }
    }

    //=========================================================================================================
    /**
     * This function sets the cancel distance used in distance calculations for the interpolation.
     * Distances higher than this are ignored, i.e. the respective coefficients are set to zero.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] dCancelDist           The new cancel distance value in meters.
     */
    void setCancelDistance(double dCancelDist)
    {
        m_lInterpolationData.dCancelDistance = dCancelDist;

        //recalculate everything because parameters changed
        calculateInterpolationOperator();
    }

    //=========================================================================================================
    /**
    * Sets the members InterpolationData.bemSurface, InterpolationData.vecSensorPos and m_numSensors.
    * In the end calls calculateSurfaceData().
    *
    * @param[in] bemSurface                The MNEBemSurface that holds the mesh information
    * @param[in] vecSensorPos              The QVector that holds the sensor positons in x, y and z coordinates.
    * @param[in] fiffEvoked                Holds all information about the sensors.
    * @param[in] iSensorType               Type of the sensor: FIFFV_EEG_CH or FIFFV_MEG_CH.
    *
    * @return Returns the created interpolation matrix.
    */
    void setInterpolationInfo(const MNELIB::MNEBemSurface &bemSurface,
                              const QVector<Vector3f> &vecSensorPos,
                              const FIFFLIB::FiffInfo &fiffInfo,
                              int iSensorType)
    {
        if(bemSurface.rr.rows() == 0) {
            qDebug() << "RtSensorDataWorker::calculateSurfaceData - Surface data is empty. Returning ...";
            return;
        }

        //set members
        m_lInterpolationData.bemSurface = bemSurface;
        m_lInterpolationData.fiffInfo = fiffInfo;
        m_lInterpolationData.iSensorType = iSensorType;

        //sensor projecting: One time operation because surface and sensors can not change
        m_lInterpolationData.pVecMappedSubset = GeometryInfo::projectSensors(m_lInterpolationData.bemSurface, vecSensorPos);

        m_bInterpolationInfoIsInit = true;

        calculateInterpolationOperator();
    }

    //=========================================================================================================
    /**
    * Sets bad channels and recalculate interpolation matrix.
    *
    * @param[in] info                 The fiff info including the new bad channels.
    */
    void setBadChannels(const FIFFLIB::FiffInfo& info)
    {
        if(!m_bInterpolationInfoIsInit) {
            qDebug() << "RtInterpolationMatWorker::updateBadChannels - Set interpolation info first.";
            return;
        }

        m_lInterpolationData.fiffInfo = info;

        //filtering of bad channels out of the distance table
        GeometryInfo::filterBadChannels(m_lInterpolationData.pDistanceMatrix,
                                        m_lInterpolationData.fiffInfo,
                                        m_lInterpolationData.iSensorType);

        //create weight matrix
        m_lInterpolationData.pWeightMatrix = Interpolation::createInterpolationMat(m_lInterpolationData.pVecMappedSubset,
                                                                                   m_lInterpolationData.pDistanceMatrix,
                                                                                   m_lInterpolationData.interpolationFunction,
                                                                                   m_lInterpolationData.dCancelDistance,
                                                                                   m_lInterpolationData.fiffInfo,
                                                                                   m_lInterpolationData.iSensorType);

        emit newInterpolationMatrixCalculated(m_lInterpolationData.pWeightMatrix);
    }

protected:    
    //=========================================================================================================
    /**
    * Calculate the interpolation operator based on the set interpolation info.
    */
    void calculateInterpolationOperator()
    {
        if(!m_bInterpolationInfoIsInit) {
            qDebug() << "RtInterpolationMatWorker::calculateInterpolationOperator - Set interpolation info first.";
            return;
        }

        //SCDC with cancel distance
        m_lInterpolationData.pDistanceMatrix = GeometryInfo::scdc(m_lInterpolationData.bemSurface,
                                                                  m_lInterpolationData.pVecMappedSubset,
                                                                  m_lInterpolationData.dCancelDistance);

        //filtering of bad channels out of the distance table
        GeometryInfo::filterBadChannels(m_lInterpolationData.pDistanceMatrix,
                                        m_lInterpolationData.fiffInfo,
                                        m_lInterpolationData.iSensorType);

        //create weight matrix
        m_lInterpolationData.pWeightMatrix = Interpolation::createInterpolationMat(m_lInterpolationData.pVecMappedSubset,
                                                                                   m_lInterpolationData.pDistanceMatrix,
                                                                                   m_lInterpolationData.interpolationFunction,
                                                                                   m_lInterpolationData.dCancelDistance,
                                                                                   m_lInterpolationData.fiffInfo,
                                                                                   m_lInterpolationData.iSensorType);

        emit newInterpolationMatrixCalculated(m_lInterpolationData.pWeightMatrix);
    }

signals:
    void newInterpolationMatrixCalculated(QSharedPointer<Eigen::SparseMatrix<float>> matInterpolationOperator);
};







class DISP3DSHARED_EXPORT RtSensorDataController : public QObject
{
    Q_OBJECT

public:
    RtSensorDataController(bool bStreamSmoothedData = true) {
        //Stream data
        m_pRtSensorDataWorker = new RtSensorDataWorker(bStreamSmoothedData);
        m_pRtSensorDataWorker->moveToThread(&m_rtSensorDataWorkerThread);

        connect(&m_rtSensorDataWorkerThread, &QThread::finished,
                m_pRtSensorDataWorker, &QObject::deleteLater);

        connect(m_pRtSensorDataWorker, &RtSensorDataWorker::newRtRawData,
                this, &RtSensorDataController::onNewRtRawData);

        connect(m_pRtSensorDataWorker, &RtSensorDataWorker::newRtSmoothedData,
                this, &RtSensorDataController::onNewSmoothedRtRawData);

        connect(&m_timer, &QTimer::timeout,
                m_pRtSensorDataWorker, &RtSensorDataWorker::streamData);

        connect(this, &RtSensorDataController::newDataReceived,
                m_pRtSensorDataWorker, &RtSensorDataWorker::addData);

        connect(this, &RtSensorDataController::interpolationMatrixChanged,
                m_pRtSensorDataWorker, &RtSensorDataWorker::setInterpolationMatrix);

        connect(this, &RtSensorDataController::surfaceColorChanged,
                m_pRtSensorDataWorker, &RtSensorDataWorker::setSurfaceColor);

        connect(this, &RtSensorDataController::thresholdsChanged,
                m_pRtSensorDataWorker, &RtSensorDataWorker::setThresholds);

        connect(this, &RtSensorDataController::sFreqChanged,
                m_pRtSensorDataWorker, &RtSensorDataWorker::setSFreq);

        connect(this, &RtSensorDataController::loopStateChanged,
                m_pRtSensorDataWorker, &RtSensorDataWorker::setLoopState);

        connect(this, &RtSensorDataController::numberAveragesChanged,
                m_pRtSensorDataWorker, &RtSensorDataWorker::setNumberAverages);

        connect(this, &RtSensorDataController::colormapTypeChanged,
                m_pRtSensorDataWorker, &RtSensorDataWorker::setColormapType);

        m_rtSensorDataWorkerThread.start();

        //Calculate interpolation matrix
        m_pRtInterpolationWorker = new RtInterpolationMatWorker();
        m_pRtInterpolationWorker->moveToThread(&m_rtInterpolationWorkerThread);

        connect(this, &RtSensorDataController::interpolationFunctionChanged,
                m_pRtInterpolationWorker, &RtInterpolationMatWorker::setInterpolationFunction);

        connect(&m_rtInterpolationWorkerThread, &QThread::finished,
                m_pRtInterpolationWorker, &QObject::deleteLater);

        connect(this, &RtSensorDataController::cancelDistanceChanged,
                m_pRtInterpolationWorker, &RtInterpolationMatWorker::setCancelDistance);

        connect(m_pRtInterpolationWorker, &RtInterpolationMatWorker::newInterpolationMatrixCalculated,
                this, &RtSensorDataController::onNewInterpolationMatrixCalculated);

        connect(this, &RtSensorDataController::interpolationInfoChanged,
                m_pRtInterpolationWorker, &RtInterpolationMatWorker::setInterpolationInfo);

        connect(this, &RtSensorDataController::badChannelsChanged,
                m_pRtInterpolationWorker, &RtInterpolationMatWorker::setBadChannels);

        m_rtInterpolationWorkerThread.start();
    }
    ~RtSensorDataController() {
        m_rtSensorDataWorkerThread.quit();
        m_rtSensorDataWorkerThread.wait();
        m_rtInterpolationWorkerThread.quit();
        m_rtInterpolationWorkerThread.wait();
    }

    QTimer                                  m_timer;
    QThread                                 m_rtSensorDataWorkerThread;
    QThread                                 m_rtInterpolationWorkerThread;
    QPointer<RtSensorDataWorker>            m_pRtSensorDataWorker;
    QPointer<RtInterpolationMatWorker>      m_pRtInterpolationWorker;
    int                                     m_iMSecInterval = 17;               /**< Length in milli Seconds to wait inbetween data samples. */

public:
    void onNewRtRawData(const Eigen::VectorXd &vecDataVector){
        emit newRtRawDataAvailable(vecDataVector);
    }
    void onNewSmoothedRtRawData(const Eigen::MatrixX3f &matColorMatrix){
        emit newRtSmoothedDataAvailable(matColorMatrix);
    }
    void onNewInterpolationMatrixCalculated(QSharedPointer<SparseMatrix<float> > matInterpolationOperator){
        emit interpolationMatrixChanged(matInterpolationOperator);
    }

    void setStreamingState(bool streamingState) {
        if(streamingState) {
            qDebug() << "RtSensorDataController::setStreamingState - start streaming";
            m_timer.start(m_iMSecInterval);
        } else {
            qDebug() << "RtSensorDataController::setStreamingState - stop streaming";
            m_timer.stop();
        }
    }

    void setInterpolationFunction(const QString &sInterpolationFunction) {
        emit interpolationFunctionChanged(sInterpolationFunction);
    }

    void setLoopState(bool bLoopState) {
        emit loopStateChanged(bLoopState);
    }

    void setCancelDistance(double dCancelDist) {
        emit cancelDistanceChanged(dCancelDist);
    }

    //=========================================================================================================
    /**
    * Set the length in MSec to wait inbetween data samples.
    *
    * @param[in] iMSec                  The new length in milli Seconds to wait inbetween data samples.
    */
    void setTimeInterval(int iMSec) {
        m_iMSecInterval = iMSec;
        m_timer.setInterval(m_iMSecInterval);
    }

    void setInterpolationInfo(const MNELIB::MNEBemSurface &bemSurface,
                              const QVector<Vector3f> &vecSensorPos,
                              const FIFFLIB::FiffInfo &fiffInfo,
                              int iSensorType) {
        emit interpolationInfoChanged(bemSurface,
                                  vecSensorPos,
                                  fiffInfo,
                                  iSensorType);
    }

    void setSurfaceColor(const MatrixX3f& matSurfaceVertColor) {
        emit surfaceColorChanged(matSurfaceVertColor);
    }

    void setThresholds(const QVector3D &vecThresholds) {
        emit thresholdsChanged(vecThresholds);
    }

    void setColormapType(const QString &sColormapType) {
        emit colormapTypeChanged(sColormapType);
    }

    void setNumberAverages(int iNumAvr) {
        emit numberAveragesChanged(iNumAvr);
    }

    void setSFreq(const double dSFreq) {
        emit sFreqChanged(dSFreq);
    }

    void setBadChannels(const FIFFLIB::FiffInfo &info) {
        emit badChannelsChanged(info);
    }

    void addData(const Eigen::MatrixXd& data) {
        emit newDataReceived(data);
    }

signals:
    //Signals to inform updated parameters
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

#endif //RTSENSORDATAWORKER_H
