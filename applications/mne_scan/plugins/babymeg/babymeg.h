//=============================================================================================================
/**
* @file     babymeg.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh, Limin Sun, Lorenz Esch and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>

#include <scShared/Interfaces/ISensor.h>
#include <generics/circularmatrixbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>
#include <QTimer>
#include <QMutex>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <QQuaternion>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB {
    class NewRealTimeMultiSampleArray;
}

namespace SCDISPLIB {
    class HPIWidget;
}

#define MAX_DATA_LEN    2000000000L
#define MAX_POS         2000000000L


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEGProjectDialog;
class BabyMEGClient;
class BabyMEGInfo;
class BabyMEGSetupWidget;
class BabyMEGProjectDialog;
class BabyMEGSQUIDControlDgl;


//=============================================================================================================
/**
* The BabyMEG class provides a connection to the babyMEG system.
*
* @brief The BabyMEG class provides a connection to the babyMEG system.
*/
class BABYMEGSHARED_EXPORT BabyMEG : public SCSHAREDLIB::ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "babymeg.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class BabyMEGSetupWidget;
    friend class BabyMEGProjectDialog;
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
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;

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
    * @return the plugin type in form of a IPlugin::PluginType
    */
    virtual SCSHAREDLIB::IPlugin::PluginType getType() const;

    //=========================================================================================================
    /**
    * Get plugin name.
    *
    * @return the plugin name as a QString
    */
    virtual QString getName() const;

    //=========================================================================================================
    /**
    * Setups this widgets and returns a pointer to it.
    *
    * @return a QWidget pointer to the set up QWidget
    */
    virtual QWidget* setupWidget();

protected:
    virtual void run();

    //=========================================================================================================
    /**
    * Initialize the connector.
    */
    void initConnector();

    //=========================================================================================================
    /**
    * Returns the babyMEG file path which is to be written to.
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

    //=========================================================================================================
    /**
    * Sends the current data block to the HPI dialog and performs a fit.
    *
    * @param [in] matData   The data block to which the HPI information is to be written.
    */
    void doContinousHPI(Eigen::MatrixXf& matData);

    //=========================================================================================================
    /**
    * Toggles teh continous HPI flag.
    *
    * @param [in] bDoContinousHPI   Whether to do continous HPI.
    */
    void onContinousHPIToggled(bool bDoContinousHPI);

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

    //=========================================================================================================
    /**
    * Shows the project dialog/window.
    */
    void showProjectDialog();

    //=========================================================================================================
    /**
    * Shows the project squid control dialog.
    */
    void showSqdCtrlDialog();

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
    * Combines all analog trigger signals to one single digital trigger line.
    *
    * @param[out] data  the data matrix
    */
    void createDigTrig(Eigen::MatrixXf& data);

    //=========================================================================================================
    /**
    * Calibrate matrix.
    *
    * @param[out] data  the data matrix
    */
    Eigen::MatrixXd calibrate(const Eigen::MatrixXf& data);

    //=========================================================================================================
    /**
    * Read projections from fiff file.
    *
    * @return true if successful, false otherwise
    */
    bool readProjectors();

    //=========================================================================================================
    /**
    * Read compensators from fiff file.
    *
    * @return true if successful, false otherwise
    */
    bool readCompensators();

    //=========================================================================================================
    /**
    * Read bad channels from fiff file.
    *
    * @return true if successful, false otherwise
    */
    bool readBadChannels();

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

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr m_pRTMSABabyMEG;    /**< The NewRealTimeMultiSampleArray to provide the rt_server Channels.*/

    QSharedPointer<IOBUFFER::RawMatrixBuffer>                                   m_pRawMatrixBuffer; /**< Holds incoming raw data. */

    QSharedPointer<BabyMEGClient>           m_pMyClient;                    /**< TCP/IP communication between Qt and Labview. */
    QSharedPointer<BabyMEGClient>           m_pMyClientComm;                /**< TCP/IP communication between Qt and Labview - communication. */
    QSharedPointer<BabyMEGInfo>             pInfo;                          /**< Set up the babyMEG info. */
    QSharedPointer<BabyMEGProjectDialog>    m_pBabyMEGProjectDialog;        /**< Window to setup the recording tiem and fiel name. */
    QSharedPointer<BabyMEGSQUIDControlDgl>  SQUIDCtrlDlg;                   /**< Nonmodal dialog for squid control. */
    QSharedPointer<SCDISPLIB::HPIWidget>    m_pHPIWidget;                   /**< HPI widget. */

    QSharedPointer<QTimer>                  m_pUpdateTimeInfoTimer;         /**< timer to control remaining time. */
    QSharedPointer<QTimer>                  m_pBlinkingRecordButtonTimer;   /**< timer to control blinking recording button. */
    QSharedPointer<QTimer>                  m_pRecordTimer;                 /**< timer to control recording time. */

    QList<int>                              m_lTriggerChannelIndices;       /**< List of all trigger channel indices. */

    FIFFLIB::FiffInfo::SPtr                 m_pFiffInfo;                    /**< Fiff measurement info.*/
    FIFFLIB::FiffStream::SPtr               m_pOutfid;                      /**< FiffStream to write to.*/

    qint16                                  m_iBlinkStatus;                 /**< The blink status of the recording button.*/
    qint32                                  m_iBufferSize;                  /**< The raw data buffer size.*/
    qint32                                  m_iSplitCount;                  /**< File split count */
    int                                     m_iRecordingMSeconds;           /**< Recording length in mseconds.*/
    bool                                    m_bWriteToFile;                 /**< Flag for for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                    m_bUseRecordTimer;              /**< Flag whether to use data recording timer.*/
    bool                                    m_bIsRunning;                   /**< If thread is running flag.*/
    bool                                    m_bDoContinousHPI;              /**< Whether to do continous HPI.*/
    QString                                 m_sBabyMEGDataPath;             /**< The data storage path.*/
    QString                                 m_sCurrentProject;              /**< The current project which is part of the filename to be recorded.*/
    QString                                 m_sCurrentSubject;              /**< The current subject which is part of the filename to be recorded.*/
    QString                                 m_sCurrentParadigm;             /**< The current paradigm which is part of the filename to be recorded.*/
    QString                                 m_sRecordFile;                  /**< Current record file. */
    QString                                 m_sFiffProjections;             /**< Fiff projection information */
    QString                                 m_sFiffCompensators;            /**< Fiff compensator information */
    QString                                 m_sBadChannels;                 /**< Filename which contains a list of bad channels */

    QFile                                   m_qFileOut;                     /**< QFile for writing to fif file.*/
    QMutex                                  m_mutex;                        /**< Mutex to guarantee thread safety.*/
    QTime                                   m_recordingStartedTime;         /**< The time when the recording started.*/

    Eigen::RowVectorXd                      m_cals;                         /**< Calibration vector.*/
    Eigen::SparseMatrix<double>             m_sparseMatCals;                /**< Sparse calibration matrix.*/

    QAction*                m_pActionSetupProject;          /**< shows setup project dialog */
    QAction*                m_pActionRecordFile;            /**< start recording action */
    QAction*                m_pActionSqdCtrl;               /**< show squid control */
    QAction*                m_pActionUpdateFiffInfo;        /**< Update Fiff Info action */
    QAction*                m_pActionComputeHPI;            /**< Update HPI info into Fiff Info action */

signals:
    //=========================================================================================================
    /**
    * Emitted when command clients connection status changed.
    *
    * @param[in] p_bStatus  connection status
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
    * @param[in] tmp    data to squid control
    */
    void DataToSquidCtrlGUI(Eigen::MatrixXf tmp);

    //=========================================================================================================
    /**
    * Emitted when data received from tcp/ip socket.
    *
    * @param[in] DATA    data to squid control
    */
    void SendCMDDataToSQUIDControl(QByteArray DATA);
};

} // NAMESPACE

#endif // BABYMEG_H
