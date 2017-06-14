//=============================================================================================================
/**
* @file     rtsensordataworker.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief     RtSensorDataWorker class declaration.
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


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QVector3D>


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
    class FiffEvoked;
}

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
* The strucut specifing the smoothing visualization info.
*/
struct VisualizationInfo {
    VectorXd                    vSourceColorSamples;
    VectorXi                    vVertNo;
    QList<FSLIB::Label>         lLabels;
    QMap<qint32, qint32>        mapLabelIdSources;
    QVector<QVector<int> >      mapVertexNeighbors;
    SparseMatrix<double>        matWDistSmooth;
    double                      dThresholdX;
    double                      dThresholdZ;
    QRgb (*functionHandlerColorMap)(double v);
    MatrixX3f                   matOriginalVertColor;
    MatrixX3f                   matFinalVertColor;
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
    * Clear this worker.
    */
    void clear();

    //=========================================================================================================
    /**
     * @brief setSurfaceData
     * @param inSurface
     * @param evoked
     * @param sensorType
     */
    void calculateSurfaceData(const MNELIB::MNEBemSurface &inSurface, const FIFFLIB::FiffEvoked &evoked, const QString sensorType);

    //=========================================================================================================
    /**
    * Set surface color data which the streamed data is plotted on.
    *
    * @param[in] matSurfaceVertColorLeftHemi      The vertex colors for the left hemipshere surface where the data is to be plotted on.
    * @param[in] matSurfaceVertColorRightHemi     The vertex colors for the right hemipshere surface where the data is to be plotted on.
    */
    void setSurfaceColor(const MatrixX3f &matSurfaceVert);


    //=========================================================================================================
    /**
    * Set the number of average to take after emitting the data to the listening threads.
    *
    * @param[in] samples                The new number of averages.
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
    * @param[in] dValue                 The new threshold values used for normalizing the data.
    */
    void setNormalization(const QVector3D &vecThresholds);

    //=========================================================================================================
    /**
    * Set the loop functionality on or off.
    *
    * @param[in] looping                The new looping state.
    */
    void setLoop(bool looping);

    //=========================================================================================================
    /**
     * @brief generateColorsFromSensorValues Produces the final color matrix that is to be emitted
     * @param sensorValues A vector of sensor signals
     * @return The final color values
     */
    Eigen::MatrixX3f generateColorsFromSensorValues(const Eigen::VectorXd& sensorValues);

    //=========================================================================================================
    /**
    * QThread functions
    */
    void stop();
    void start();

protected:
    //=========================================================================================================
    /**
    * QThread functions
    */
    virtual void run() override;

private:
    //=========================================================================================================
    /**
    * Perfrom the needed visualization type computations.
    *
    * @param[in] vSourceColorSamples        The color data for the sources.
    *
    * @return                               Returns the final colors in for the left and right hemisphere.
    */
    Eigen::MatrixX3f performVisualizationTypeCalculation(const Eigen::VectorXd& vSourceColorSamples);


    QMutex                  m_qMutex;                           /**< The thread's mutex. */

    QList<Eigen::VectorXd>  m_lData;                            /**< List that holds the fiff matrix data <n_channels x n_samples>. */

    bool                    m_bIsRunning;                       /**< Flag if this thread is running. */
    bool                    m_bIsLooping;                       /**< Flag if this thread should repeat sending the same data over and over again. */
    bool                    m_bSurfaceDataIsInit;               /**< Flag if this thread's surface data was initialized. This flag is used to decide whether specific visualization types can be computed. */

    int                     m_numSensors;                       /**< Number of sensors that this worker does expect when receiving rt data. */
    int                     m_iAverageSamples;                  /**< Number of average to compute. */
    int                     m_iCurrentSample;                   /**< Number of the current sample which is/was streamed. */
    int                     m_iMSecIntervall;                   /**< Length in milli Seconds to wait inbetween data samples. */

    VisualizationInfo       m_lVisualizationInfo;

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever this item should send new colors to its listeners.
    *
    * @param[in] colorPair     The samples data in form of a QPair rgb colors for each (left, right) hemisphere.
    */
    void newRtData(const Eigen::MatrixX3f &colorMatrix);
};

} // NAMESPACE

#endif //RTSENSORDATAWORKER_H
