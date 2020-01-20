//=============================================================================================================
/**
 * @file     neuromag.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the Neuromag class.
 *
 */

#ifndef NEUROMAG_H
#define NEUROMAG_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag_global.h"

#include <scShared/Interfaces/ISensor.h>
#include <utils/generics/circularmatrixbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTimer>
#include <QTime>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

#define MAX_DATA_LEN    2000000000L
#define MAX_POS         2000000000L


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISPLIB {
    class ProjectSettingsView;
}

namespace FIFFLIB {
    class FiffStream;
    class FiffInfo;
}

namespace COMMUNICATIONLIB {
    class RtCmdClient;
}

namespace SCMEASLIB {
    class RealTimeMultiSampleArray;
}

namespace DISP3DLIB {
    class HpiView;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NEUROMAGPLUGIN
//=============================================================================================================

namespace NEUROMAGPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// NEUROMAGPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class NeuromagProducer;


//=============================================================================================================
/**
 * DECLARE CLASS Neuromag
 *
 * @brief The Neuromag class provides a connector to the mne_rt_server Neuromag plugin.
 */
class NEUROMAGSHARED_EXPORT Neuromag : public SCSHAREDLIB::ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "neuromag.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class NeuromagProducer;
    friend class NeuromagSetupWidget;

public:

    //=========================================================================================================
    /**
    * Constructs a Neuromag.
    */
    Neuromag();

    //=========================================================================================================
    /**
    * Destroys the Neuromag.
    */
    virtual ~Neuromag();

    //=========================================================================================================
    /**
    * Clears the rt server
    */
    void clear();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Initialise the Neuromag.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();    

    //=========================================================================================================
    /**
    * Shows the project dialog/window.
    */
    void showProjectDialog();

    //=========================================================================================================
    /**
    * Determines current file. And starts a new one.
    */
    void splitRecordingFile();

    //=========================================================================================================
    /**
    * Starts or stops a file recording depending on the current recording state.
    */
    void toggleRecordingFile();

    //=========================================================================================================
    /**
    * Set the recording time in seconds.
    *
    * @param[in] time   the new recording time.
    */
    void setRecordingTimerChanged(int timeMSecs);

    //=========================================================================================================
    /**
    * Set the recording time active flag.
    *
    * @param[in] state   whether the recording should be used or not.
    */
    void setRecordingTimerStateChanged(bool state);

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    bool readProjectors();

    //=========================================================================================================
    /**
    * Change connector
    *
    * @param[in] p_iNewConnectorId      new connector ID
    */
    void changeConnector(qint32 p_iNewConnectorId);

    //=========================================================================================================
    /**
    * Connects the cmd client.
    */
    void connectCmdClient();

    //=========================================================================================================
    /**
    * Disconnects the cmd client.
    */
    void disconnectCmdClient();

    //=========================================================================================================
    /**
    * Request FiffInfo using cmd client and producer (data client)
    */
    void requestInfo();

protected:
    virtual void run();

    //=========================================================================================================
    /**
    * Calibrate matrix.
    *
    * @param[out] data  the data matrix
    */
    Eigen::MatrixXd calibrate(const Eigen::MatrixXf& data);

    //=========================================================================================================
    /**
    * change recording button.
    */
    void changeRecordingButton();

    //=========================================================================================================
    /**
    * This function sends the current remaining recording time to the project window.
    */
    void onRecordingRemainingTimeChange();

    //=========================================================================================================
    /**
    * Initialises the output connector.
    */
    void initConnector();

    //=========================================================================================================
    /**
    * Set HPI fiff information.
    */
    void showHPIDialog();

    //=========================================================================================================
    /**
    * Sends the current data block to the HPI dialog.
    *
    * @param [in] matData   The new data block.
    */
    void updateHPI(const Eigen::MatrixXf &matData);

    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > m_pRTMSA_Neuromag;          /**< The RealTimeMultiSampleArray to provide the rt_server Channels.*/

    QSharedPointer<COMMUNICATIONLIB::RtCmdClient>       m_pRtCmdClient;                 /**< The command client.*/
    QSharedPointer<NeuromagProducer>                    m_pNeuromagProducer;            /**< Holds the NeuromagnProducer.*/
    QSharedPointer<IOBUFFER::RawMatrixBuffer>           m_pRawMatrixBuffer_In;          /**< Holds incoming raw data. */
    QSharedPointer<QTimer>                              m_pUpdateTimeInfoTimer;         /**< timer to control remaining time. */
    QSharedPointer<QTimer>                              m_pBlinkingRecordButtonTimer;   /**< timer to control blinking recording button. */
    QSharedPointer<QTimer>                              m_pRecordTimer;                 /**< timer to control recording time. */
    QSharedPointer<DISPLIB::ProjectSettingsView>        m_pProjectSettingsView;         /**< Window to setup the recording tiem and fiel name. */
    QSharedPointer<FIFFLIB::FiffStream>                 m_pOutfid;                      /**< FiffStream to write to.*/
    QSharedPointer<FIFFLIB::FiffInfo>                   m_pFiffInfo;                    /**< Fiff measurement info.*/
    QSharedPointer<DISP3DLIB::HpiView>                  m_pHPIWidget;                   /**< HPI widget. */

    QMutex                                  m_mutex;

    QString                                 m_sNeuromagClientAlias;         /**< The rt server client alias.*/
    QString                                 m_sNeuromagIP;                  /**< The IP Adress of mne_rt_server.*/
    QString                                 m_sFiffHeader;                  /**< Fiff header information */
    QString                                 m_sNeuromagDataPath;            /**< The data storage path.*/
    QString                                 m_sCurrentProject;              /**< The current project which is part of the filename to be recorded.*/
    QString                                 m_sCurrentSubject;              /**< The current subject which is part of the filename to be recorded.*/
    QString                                 m_sCurrentParadigm;             /**< The current paradigm which is part of the filename to be recorded.*/
    QString                                 m_sRecordFile;                  /**< Current record file. */

    bool                                    m_bCmdClientIsConnected;        /**< If the command client is connected.*/
    bool                                    m_bIsRunning;                   /**< Whether FiffSimulator is running.*/
    bool                                    m_bWriteToFile;                 /**< Flag for for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                    m_bUseRecordTimer;              /**< Flag whether to use data recording timer.*/

    qint16                                  m_iBlinkStatus;                 /**< The blink status of the recording button.*/
    qint32                                  m_iSplitCount;                  /**< File split count */
    qint32                                  m_iBufferSize;                  /**< The raw data buffer size.*/
    qint32                                  m_iActiveConnectorId;           /**< The active connector.*/
    int                                     m_iRecordingMSeconds;           /**< Recording length in mseconds.*/

    QMap<qint32, QString>                   m_qMapConnectors;               /**< Connector map.*/

    QTimer                                  m_cmdConnectionTimer;           /**< Timer for convinient command client connection. When timer times out a connection is tried to be established. */
    QTime                                   m_recordingStartedTime;         /**< The time when the recording started.*/

    QFile                                   m_qFileOut;                     /**< QFile for writing to fif file.*/

    Eigen::RowVectorXd                      m_cals;                         /**< Calibration vector.*/
    Eigen::SparseMatrix<double>             m_sparseMatCals;                /**< Sparse calibration matrix.*/

    QPointer<QAction>                       m_pActionSetupProject;          /**< shows setup project dialog */
    QPointer<QAction>                       m_pActionRecordFile;            /**< start recording action */
    QPointer<QAction>                       m_pActionComputeHPI;            /**< The Action to show the HPI view */

signals:
    //=========================================================================================================
    /**
    * Emitted when command clients connection status changed
    *
    * @param[in] p_bStatus  connection status
    */
    void cmdConnectionChanged(bool p_bStatus);

    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();

};

} // NAMESPACE

#endif // NEUROMAG_H
