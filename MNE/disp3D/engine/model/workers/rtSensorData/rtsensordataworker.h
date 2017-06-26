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

#ifndef RTSENSORDATAWORKER_H
#define RTSENSORDATAWORKER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../../items/common/types.h"
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_evoked.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QVector3D>
#include <QSharedPointer>


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


//*************************************************************************************************************
//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================
//=============================================================================================================

/**
* The struct specifing visualization info.
*/
struct VisualizationInfo {
    double                      dThresholdX;
    double                      dThresholdZ;

    MatrixX3f                   matOriginalVertColor;
    MatrixX3f                   matFinalVertColor;

    QRgb (*functionHandlerColorMap)(double v);
};

//=============================================================================================================
/**
 * The struct specifing all data that is used in the interpolation process
 */
struct InterpolationData {
    int                                     iSensorType;                      /**< Type of the sensor: FIFFV_EEG_CH or FIFFV_MEG_CH. */

    double                                  dCancelDistance;                  /**< Cancel distance for the interpolaion in meters. */
    
    QSharedPointer<SparseMatrix<double> >   pWeightMatrix;                    /**< Weight matrix that holds all coefficients for a signal interpolation. */
    QSharedPointer<MatrixXd>                pDistanceMatrix;                  /**< Distance matrix that holds distances from sensors positions to the near vertices in meters. */
    QSharedPointer<QVector<qint32>>         pVecMappedSubset;                 /**< Vector index position represents the id of the sensor and the qint in each cell is the vertex it is mapped to. */

    MNELIB::MNEBemSurface                   bemSurface;                       /**< Holds all vertex information that is needed (public member rr). */
    FIFFLIB::FiffInfo                       fiffInfo;                         /**< Contains all information about the sensors. */
    
    double (*interpolationFunction) (double);                                 /**< Function that computes interpolation coefficients using the distance values. */
};

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
class DISP3DNEWSHARED_EXPORT RtSensorDataWorker : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtSensorDataWorker> SPtr;            /**< Shared pointer type for RtSensorDataWorker class. */
    typedef QSharedPointer<const RtSensorDataWorker> ConstSPtr; /**< Const shared pointer type for RtSensorDataWorker class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] parent      The parent of the QObject.
    */
    explicit RtSensorDataWorker(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Default destructor.
    */
    ~RtSensorDataWorker();

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
     * Sets the members InterpolationData.bemSurface, InterpolationData.vecSensorPos and m_numSensors.
     * In the end calls calculateSurfaceData().
     * 
     * @param[in] bemSurface                The MNEBemSurface that holds the mesh information
     * @param[in] vecSensorPos              The QVector that holds the sensor positons in x, y and z coordinates.
     * @param[in] fiffEvoked                Holds all information about the sensors.
     * @param[in] iSensorType               Type of the sensor: FIFFV_EEG_CH or FIFFV_MEG_CH.
     */
    void calculateSurfaceData(const MNELIB::MNEBemSurface &bemSurface,
                              const QVector<Vector3f> &vecSensorPos,
                              const FIFFLIB::FiffInfo &fiffInfo,
                              int iSensorType);
   
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
    * Set the length in milli Seconds to wait inbetween data samples.
    *
    * @param[in] iMSec                  The new length in milli Seconds to wait inbetween data samples.
    */
    void setInterval(int iMSec);

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
    void setNormalization(const QVector3D &vecThresholds);
    
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
    * @param[in] bLooping                The new looping state.
    */
    void setLoop(bool bLooping);

    //=========================================================================================================
    /**
    * Sets the running flag to false and waits for the worker to stop.
    */
    void stop();

    //=========================================================================================================
    /**
    * Resets the index of the current sample and starts the worker.
    */
    void start();

protected:
    //=========================================================================================================
    /**
    * Main method of this worker: Checks whether it is time for the worker to output new data for visualization.
    * If so, it averages the specified amount of data samples and calculates the output.
    */
    virtual void run() override;

private:
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

    //=========================================================================================================
    /**
     * Prepares the necessary data for the later ongoing interpolation of signals.
     * Calculates a weight matrix which is based on surfaced constrained distances.
     */
    void calculateSurfaceData();

    //=========================================================================================================

    QMutex                                  m_qMutex;                           /**< The thread's mutex. */

    QList<Eigen::VectorXd>                  m_lData;                            /**< List that holds the fiff matrix data <n_channels x n_samples>. */

    bool                                    m_bIsRunning;                       /**< Flag if this thread is running. */
    bool                                    m_bIsLooping;                       /**< Flag if this thread should repeat sending the same data over and over again. */
    bool                                    m_bSurfaceDataIsInit;               /**< Flag if this thread's surface data was initialized. This flag is used to decide whether specific visualization types can be computed. */

    int                                     m_iNumSensors;                      /**< Number of sensors that this worker does expect when receiving rt data. */
    int                                     m_iAverageSamples;                  /**< Number of average to compute. */
    int                                     m_iCurrentSample;                   /**< Number of the current sample which is/was streamed. */
    int                                     m_iMSecIntervall;                   /**< Length in milli Seconds to wait inbetween data samples. */
    
    VisualizationInfo                       m_lVisualizationInfo;               /**< Container for the visualization info. */

    InterpolationData                       m_lInterpolationData;               /**< Container for the interpolation data. */
    

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever this item should send new colors to its listeners.
    *
    * @param[in] colorMatrix     The samples data in form of rgb colors for the mesh.
    */
    void newRtData(const Eigen::MatrixX3f &colorMatrix);
};

} // NAMESPACE

#endif //RTSENSORDATAWORKER_H
