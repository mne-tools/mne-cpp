//=============================================================================================================
/**
* @file     neuromag.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the Neuromag class.
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
#include <utils/generics/circularbuffer_old.h>
#include <utils/generics/circularmatrixbuffer.h>
#include <scMeas/realtimemultisamplearray.h>

#include <communication/rtClient/rtcmdclient.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>
#include <QTimer>


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
// DEFINE NAMESPACE NEUROMAGPLUGIN
//=============================================================================================================

namespace NEUROMAGPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class NeuromagProducer;
class NeuromagProjectDialog;


//=============================================================================================================
/**
* DECLARE CLASS Neuromag
*
* @brief The Neuromag class provides a RT server connection.
*/
class NEUROMAGSHARED_EXPORT Neuromag : public SCSHAREDLIB::ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "neuromag.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class NeuromagProducer;
    friend class NeuromagSetupWidget;
    friend class NeuromagProjectDialog;

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
    * Returns the Neuromag file path which is to be written to.
    *
    * @param[in] currentTime    insert current time stamp.
    *
    * @return the storage filepath
    */
    QString getFilePath(bool currentTime = false) const;

    //=========================================================================================================
    /**
    * Returns the path where the subjects folders are stored.
    *
    * @return the data path
    */
    QString getDataPath() const;

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
    * change recording button.
    */
    void changeRecordingButton();

    //=========================================================================================================
    /**
    * This function sends the current remaining recording time to the project window.
    */
    void onRecordingRemainingTimeChange();

private:
    //=========================================================================================================
    /**
    * Initialises the output connector.
    */
    void initConnector();

    bool readHeader();

//    float           m_fSamplingRate;                /**< The sampling rate.*/
//    int             m_iDownsamplingFactor;          /**< The down sampling factor.*/

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr m_pRTMSA_Neuromag;          /**< The NewRealTimeMultiSampleArray to provide the rt_server Channels.*/

    QMutex                                  m_mutex;
    QString                                 m_sNeuromagClientAlias;         /**< The rt server client alias.*/

    QSharedPointer<COMMUNICATIONLIB::RtCmdClient>m_pRtCmdClient;                 /**< The command client.*/
    bool                                    m_bCmdClientIsConnected;        /**< If the command client is connected.*/

    QString                                 m_sNeuromagIP;                  /**< The IP Adress of mne_rt_server.*/
    QString                                 m_sFiffHeader;                  /**< Fiff header information */

    QSharedPointer<NeuromagProducer>        m_pNeuromagProducer;            /**< Holds the NeuromagnProducer.*/

    QMap<qint32, QString>                   m_qMapConnectors;               /**< Connector map.*/
    qint32                                  m_iActiveConnectorId;           /**< The active connector.*/

    FIFFLIB::FiffInfo::SPtr                 m_pFiffInfo;                    /**< Fiff measurement info.*/
    qint32                                  m_iBufferSize;                  /**< The raw data buffer size.*/

    QTimer                                  m_cmdConnectionTimer;           /**< Timer for convinient command client connection. When timer times out a connection is tried to be established. */

    QSharedPointer<IOBUFFER::RawMatrixBuffer>   m_pRawMatrixBuffer_In;      /**< Holds incoming raw data. */

    bool                                    m_bIsRunning;                   /**< Whether FiffSimulator is running.*/

    QSharedPointer<QTimer>                  m_pUpdateTimeInfoTimer;         /**< timer to control remaining time. */
    QSharedPointer<QTimer>                  m_pBlinkingRecordButtonTimer;   /**< timer to control blinking recording button. */
    QSharedPointer<QTimer>                  m_pRecordTimer;                 /**< timer to control recording time. */
    QSharedPointer<NeuromagProjectDialog>   m_pNeuromagProjectDialog;       /**< Window to setup the recording tiem and fiel name. */
    FIFFLIB::FiffStream::SPtr               m_pOutfid;                      /**< FiffStream to write to.*/
    qint16                                  m_iBlinkStatus;                 /**< The blink status of the recording button.*/
    qint32                                  m_iSplitCount;                  /**< File split count */
    int                                     m_iRecordingMSeconds;           /**< Recording length in mseconds.*/
    bool                                    m_bWriteToFile;                 /**< Flag for for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                    m_bUseRecordTimer;              /**< Flag whether to use data recording timer.*/
    QString                                 m_sNeuromagDataPath;            /**< The data storage path.*/
    QString                                 m_sCurrentProject;              /**< The current project which is part of the filename to be recorded.*/
    QString                                 m_sCurrentSubject;              /**< The current subject which is part of the filename to be recorded.*/
    QString                                 m_sCurrentParadigm;             /**< The current paradigm which is part of the filename to be recorded.*/
    QString                                 m_sRecordFile;                  /**< Current record file. */
    QFile                                   m_qFileOut;                     /**< QFile for writing to fif file.*/
    QTime                                   m_recordingStartedTime;         /**< The time when the recording started.*/
    Eigen::RowVectorXd                      m_cals;                         /**< Calibration vector.*/
    Eigen::SparseMatrix<double>             m_sparseMatCals;                /**< Sparse calibration matrix.*/
    QAction*                                m_pActionSetupProject;          /**< shows setup project dialog */
    QAction*                                m_pActionRecordFile;            /**< start recording action */

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
