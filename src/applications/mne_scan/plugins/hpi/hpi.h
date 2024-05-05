//=============================================================================================================
/**
 * @file     hpi.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Ruben Dörfel. All rights reserved.
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
 * @brief    Contains the declaration of the Hpi class.
 *
 */

#ifndef HPI_H
#define HPI_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpi_global.h"

#include <utils/generics/circularbuffer.h>
#include <scShared/Plugins/abstractalgorithm.h>

#include <fiff/fiff_dig_point.h>

#include <thread>
#include <atomic>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
    class FiffDigPoint;
    class FiffCoordTrans;
    class FiffDigitizerData;
}

namespace INVERSELIB {
    class HPIFit;
}

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
    class RealTimeHpiResult;
}

#define MAX_DATA_LEN    2000000000L

//=============================================================================================================
// DEFINE NAMESPACE HPIPLUGIN
//=============================================================================================================

namespace HPIPLUGIN
{

//=============================================================================================================
// HPIPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS Hpi
 *
 * @brief The Hpi class provides a tools to reduce noise of an incoming data stream. It then forwards the processed data to subsequent plugins.
 */
class HPISHARED_EXPORT Hpi : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "hpi.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

public:
    //=========================================================================================================
    /**
     * Constructs a Hpi.
     */
    Hpi();

    //=========================================================================================================
    /**
     * Destroys the Hpi.
     */
    ~Hpi();

    //=========================================================================================================
    /**
     * AbstractAlgorithm functions
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildInfo();

    //=========================================================================================================
    /**
     * Udates the plugin with new (incoming) data.
     *
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

private:
    //=========================================================================================================
    /**
     * Update the projectors for SSP and Comps.
     */
    void updateProjections();

    //=========================================================================================================
    /**
     * Call this function whenever the allowed error changed.
     *
     * @param[in] dAllowedMeanErrorDist    Allowed mean error in mm.
     */
    void onAllowedMeanErrorDistChanged(double dAllowedMeanErrorDist);

    //=========================================================================================================
    /**
     * Call this function whenever the allowed head movement threshold changed.
     *
     * @param[in] dAllowedMeanErrorDist    Allowed movement threshold.
     */
    void onAllowedMovementChanged(double dAllowedMovement);

    //=========================================================================================================
    /**
     * Call this function whenever the allowed head rotation threshold changed.
     *
     * @param[in] dAllowedMeanErrorDist    Allowed rotation in degree.
     */
    void onAllowedRotationChanged(double dAllowedRotation);

    //=========================================================================================================
    /**
     * Call this funciton whenever new digitzers were loaded.
     *
     * @param[in] lDigitzers    The new digitzers.
     * @param[in] sFilePath     The file path to the new digitzers.
     */
    void onDigitizersChanged(const QList<FIFFLIB::FiffDigPoint>& lDigitzers,
                             const QString& sFilePath);

    //=========================================================================================================
    /**
     * Call this funciton whenever when a single HPI fit based on the last data block was requested.
     */
    void onDoSingleHpiFit();

    //=========================================================================================================
    /**
     * Call this funciton whenever frequency ordering was requested.
     */
    void onDoFreqOrder();

    //=========================================================================================================
    /**
     * Call this funciton whenever the coil frequencies changed.
     *
     * @param[in] vCoilFreqs    The new coil frequencies.
     */
    void onCoilFrequenciesChanged(const QVector<int>& vCoilFreqs);

    //=========================================================================================================
    /**
     * Call this function whenever SSP checkbox changed.
     *
     * @param[in] bChecked    Whether the SSP check box is checked.
     */
    void onSspStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Call this function whenever compensator checkbox changed.
     *
     * @param[in] bChecked    Whether the compensator check box is checked.
     */
    void onCompStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Call this function whenever continous HPI checkbox changed.
     *
     * @param[in] bChecked    Whether the continous HPI check box is checked.
     */
    void onContHpiStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Call this function whenever the device to head transformation matrix changed.
     *
     * @param[in] devHeadTrans    The new device to head transformation matrix changed.
     */
    void onDevHeadTransAvailable(const FIFFLIB::FiffCoordTrans& devHeadTrans);

    //=========================================================================================================
    /**
     * Set fitting window size when doing continuous hpi.
     *
     * @param[in] winSize    window size in samples
     */
    void setFittingWindowSize(int winSize);

    //=========================================================================================================
    /**
     * Read Polhemus data from fif file.
     */
    QList<FIFFLIB::FiffDigPoint> readPolhemusDig(const QString& fileName);

    //=========================================================================================================
    /**
     * AbstractAlgorithm function
     */
    virtual void run();

    //=========================================================================================================
    /**
     * Manages iitilization of measurement metadata
     *
     * @param[in] pRTMSA    input real-time multi-sample arraymeasurement
     */
    void manageInitialization(QSharedPointer<SCMEASLIB::RealTimeMultiSampleArray> pRTMSA);

    //=========================================================================================================
    /**
     * Initializes fiff info based on input info.
     *
     * @param[in] info      input fiff info objcts
     */
    void initFiffInfo(QSharedPointer<FIFFLIB::FiffInfo> info);

    //=========================================================================================================
    /**
     * Inititlizaes fiffi digitizer information based on input fiffDig.
     *
     * @param[in] fiffDig   input fiff digitizer information
     */
    void initFiffDigitizers(QSharedPointer<FIFFLIB::FiffDigitizerData> fiffDig);

    //=========================================================================================================
    /**
     * Update viewer gui with current fiff digitizer metadata..
     */
    void updateDigitizerInfo();

    void resetState();

    QMutex                      m_mutex;                    /**< The threads mutex.*/

    QVector<int>                m_vCoilFreqs;               /**< Vector contains the HPI coil frequencies. */

    QString                     m_sFilePathDigitzers;       /**< The file path to the current digitzers. */

    qint16                      m_iNumberBadChannels;       /**< The number of bad channels.*/
    qint16                      m_iFittingWindowSize;       /**< The number of samples in each fitting window.*/

    double                      m_dAllowedMeanErrorDist;    /**< The allowed error distance in order for the last fit to be counted as a good fit.*/
    double                      m_dAllowedMovement;         /**< The allowed head movement regarding reference head position.*/
    double                      m_dAllowedRotation;         /**< The allowed head rotation regarding reference head position in degree.*/

    bool                        m_bDoFreqOrder;             /**< Order Frequencies.*/
    bool                        m_bDoSingleHpi;             /**< Do a single HPI fit.*/
    bool                        m_bDoContinousHpi;          /**< Do continous HPI fitting.*/
    bool                        m_bUseSSP;                  /**< Use SSP's.*/
    bool                        m_bUseComp;                 /**< Use Comps's.*/

    Eigen::MatrixXd             m_matData;                  /**< The last data block.*/
    Eigen::MatrixXd             m_matCompProjectors;        /**< Holds the matrix with the SSP and compensator projectors.*/

    QSharedPointer<FIFFLIB::FiffInfo>                                           m_pFiffInfo;            /**< Fiff measurement info.*/
    QSharedPointer<FIFFLIB::FiffDigitizerData>                                  m_pFiffDigitizerData;
    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>                      m_pCircularBuffer;      /**< Holds incoming raw data. */

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pHpiInput;            /**< The RealTimeMultiSampleArray of the Hpi input.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeHpiResult>::SPtr           m_pHpiOutput;           /**< The RealTimeHpiResult of the Hpi output.*/

    std::thread             m_OutputProcessingThread;
    std::atomic_bool        m_bProcessOutput;

signals:
    void errorsChanged(const QVector<double>& vErrors,
                       double dMeanErrorDist);
    void gofChanged(const Eigen::VectorXd& vGoF,
                    const double dMeanGoF);
    void movementResultsChanged(double dMovement,
                                double dRotation);
    void devHeadTransAvailable(const FIFFLIB::FiffCoordTrans& devHeadTrans);

    void newDigitizerList(QList<FIFFLIB::FiffDigPoint> pointList);
};
} // NAMESPACE

#endif // HPI_H
