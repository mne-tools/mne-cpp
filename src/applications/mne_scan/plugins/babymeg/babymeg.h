//=============================================================================================================
/**
 * @file     babymeg.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Gabriel B Motta, Limin Sun, Lorenz Esch. All rights reserved.
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
 * @brief    BabyMEG class declaration.
 *
 */

#ifndef BABYMEG_H
#define BABYMEG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>

#include <scShared/Plugins/abstractsensor.h>
#include <utils/generics/circularbuffer.h>

#include <thread>
#include <atomic>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMessageBox>
#include <QScrollBar>
#include <QTime>
#include <QPointer>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB {
    class RealTimeMultiSampleArray;
}

namespace DISPLIB {
    class ProjectSettingsView;
}

#define MAX_DATA_LEN    2000000000L
#define MAX_POS         2000000000L

//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{

//=============================================================================================================
// BABYMEGPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEGClient;
class BabyMEGInfo;
class BabyMEGSetupWidget;
class BabyMEGSQUIDControlDgl;

//=============================================================================================================
/**
 * The BabyMEG class provides a connection to the babyMEG system.
 *
 * @brief The BabyMEG class provides a connection to the babyMEG system.
 */
class BABYMEGSHARED_EXPORT BabyMEG : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "babymeg.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

    friend class BabyMEGSetupWidget;
    friend class BabyMEGSQUIDControlDgl;

public:
    //=========================================================================================================
    /**
     * Constructs a BabyMEG.
     */
    BabyMEG();

    //=========================================================================================================
    /**
     * Destroys the BabyMEG.
     */
    virtual ~BabyMEG();

    //=========================================================================================================
    /**
     * Clone the plugin
     *
     * @return Returns the cloned widget.
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;

    //=========================================================================================================
    /**
     * Initialise the BabyMEG.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    virtual void unload();

    //=========================================================================================================
    /**
     * Clears the babymeg
     */
    void clear();

    //=========================================================================================================
    /**
     * Start the thread.
     */
    virtual bool start();

    //=========================================================================================================
    /**
     * Stop the thread.
     */
    virtual bool stop();

    //=========================================================================================================
    /**
     * Get plugin type.
     *
     * @return the plugin type in form of a AbstractPlugin::PluginType.
     */
    virtual SCSHAREDLIB::AbstractPlugin::PluginType getType() const;

    //=========================================================================================================
    /**
     * Get plugin name.
     *
     * @return the plugin name as a QString.
     */
    virtual QString getName() const;

    //=========================================================================================================
    /**
     * Setups this widgets and returns a pointer to it.
     *
     * @return a QWidget pointer to the set up QWidget.
     */
    virtual QWidget* setupWidget();

    virtual QString getBuildInfo();

protected:
    virtual void run();

    //=========================================================================================================
    /**
     * Initialize the connector.
     */
    void initConnector();

    //=========================================================================================================
    /**
     * Sets the Fiff Info.
     *
     * @param[in] p_FiffInfo    the Fiff Info.
     */
    void setFiffInfo(const FIFFLIB::FiffInfo& p_FiffInfo);

    //=========================================================================================================
    /**
     * Sets the Fiff Info data.
     *
     * @param[in] DATA    the Fiff Info data.
     */
    void setFiffData(QByteArray DATA);

    //=========================================================================================================
    /**
     * Sets the CMD data.
     *
     * @param[in] DATA    the CMD data.
     */
    void setCMDData(QByteArray DATA);

    //=========================================================================================================
    /**
     * Sets the gain info.
     *
     * @param[in] GainInfo    a QStringList with all the gain information.
     */
    void setFiffGainInfo(QStringList GainInfo);

    //=========================================================================================================
    /**
     * Returns information from FLL hardware.
     *
     * @param[in] t_sFLLControlCommand  FLL command.
     */
    void comFLL(QString t_sFLLControlCommand);

    //=========================================================================================================
    /**
     * Update fiff information.
     */
    void updateFiffInfo();

    //=========================================================================================================
    /**
     * Shows the project squid control dialog.
     */
    void showSqdCtrlDialog();

    //=========================================================================================================
    /**
     * Combines all analog trigger signals to one single digital trigger line.
     *
     * @param[out] data  the data matrix.
     */
    void createDigTrig(Eigen::MatrixXf& data);

    //=========================================================================================================
    /**
     * Calibrate matrix.
     *
     * @param[out] data  the data matrix.
     */
    Eigen::MatrixXd calibrate(const Eigen::MatrixXf& data);

    //=========================================================================================================
    /**
     * Read projections from fiff file.
     *
     * @return true if successful, false otherwise.
     */
    bool readProjectors();

    //=========================================================================================================
    /**
     * Read compensators from fiff file.
     *
     * @return true if successful, false otherwise.
     */
    bool readCompensators();

    //=========================================================================================================
    /**
     * Read bad channels from fiff file.
     *
     * @return true if successful, false otherwise.
     */
    bool readBadChannels();

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr m_pRTMSABabyMEG;    /**< The RealTimeMultiSampleArray to provide the rt_server Channels.*/

    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_float>                    m_pCircularBuffer;  /**< Holds incoming raw data. */

    QSharedPointer<BabyMEGClient>                   m_pMyClient;                    /**< TCP/IP communication between Qt and Labview. */
    QSharedPointer<BabyMEGClient>                   m_pMyClientComm;                /**< TCP/IP communication between Qt and Labview - communication. */
    QSharedPointer<BabyMEGInfo>                     m_pInfo;                        /**< Set up the babyMEG info. */
    QSharedPointer<BabyMEGSQUIDControlDgl>          m_pSQUIDCtrlDlg;                /**< Nonmodal dialog for squid control. */

    QList<int>                              m_lTriggerChannelIndices;       /**< List of all trigger channel indices. */

    FIFFLIB::FiffInfo::SPtr                 m_pFiffInfo;                    /**< Fiff measurement info.*/

    qint32                                  m_iBufferSize;                  /**< The raw data buffer size.*/

    QString                                 m_sFiffProjections;             /**< Fiff projection information. */
    QString                                 m_sFiffCompensators;            /**< Fiff compensator information. */
    QString                                 m_sBadChannels;                 /**< Filename which contains a list of bad channels. */

    QMutex                                  m_mutex;                        /**< Mutex to guarantee thread safety.*/

    Eigen::RowVectorXd                      m_cals;                         /**< Calibration vector.*/
    Eigen::SparseMatrix<double>             m_sparseMatCals;                /**< Sparse calibration matrix.*/

    QPointer<QAction>                       m_pActionSqdCtrl;               /**< show squid control. */
    QPointer<QAction>                       m_pActionUpdateFiffInfo;        /**< Update Fiff Info action. */

    std::thread             m_OutputProcessingThread;
    std::atomic_bool        m_bProcessOutput;
signals:
    //=========================================================================================================
    /**
     * Emitted when command clients connection status changed.
     *
     * @param[in] p_bStatus  connection status.
     */
    void cmdConnectionChanged(bool p_bStatus);

    //=========================================================================================================
    /**
     * Emitted when fiffInfo is available.
     */
    void fiffInfoAvailable();

    //=========================================================================================================
    /**
     * Emitted when data is ready.
     *
     * @param[in] tmp    data to squid control.
     */
    void dataToSquidCtrlGUI(Eigen::MatrixXf tmp);

    //=========================================================================================================
    /**
     * Emitted when data received from tcp/ip socket.
     *
     * @param[in] DATA    data to squid control.
     */
    void sendCMDDataToSQUIDControl(QByteArray DATA);
};
} // NAMESPACE

#endif // BABYMEG_H
