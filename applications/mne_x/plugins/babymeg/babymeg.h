//=============================================================================================================
/**
* @file     babymeg.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the BabyMEG class.
*
*/

#ifndef BABYMEG_H
#define BABYMEG_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"
#include "babymegclient.h"

#include "FormFiles/babymegsquidcontroldgl.h"
#include "FormFiles/babymeghpidgl.h"


#include <mne_x/Interfaces/ISensor.h>
#include <generics/circularbuffer_old.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/newrealtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>
#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <rtClient/rtcmdclient.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>
#include <QTimer>

#define MAX_DATA_LEN    2000000000L
#define MAX_POS         2000000000L


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE BabyMEGPlugin
//=============================================================================================================

namespace BabyMEGPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace IOBuffer;
using namespace RTCLIENTLIB;
using namespace FIFFLIB;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEGProjectDialog;
class babymeghpidgl;


//=============================================================================================================
/**
* DECLARE CLASS BabyMEG
*
* @brief The BabyMEG class provides a RT server connection.
*/
class BABYMEGSHARED_EXPORT BabyMEG : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "babymeg.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::ISensor)

    friend class BabyMEGSetupWidget;
    friend class BabyMEGProjectDialog;
    friend class BabyMEGSQUIDControlDgl;
    friend class babymeghpidgl;

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
    * Clears the rt server
    */
    void clear();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

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
    *
    * @return the data path
    */
    QString getDataPath() const;

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

    void showProjectDialog();

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


    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    void setFiffInfo(FIFFLIB::FiffInfo);
    void setFiffData(QByteArray DATA);
    void setCMDData(QByteArray DATA);
    void setFiffGainInfo(QStringList);

    //=========================================================================================================
    /**
    * Returns information from FLL hardware
    *
    * @param[in] t_sFLLControlCommand  FLL command.
    */
    void comFLL(QString t_sFLLControlCommand);

    //=========================================================================================================
    /**
    * Update fiff information
    *
    */
    void UpdateFiffInfo();

    //=========================================================================================================
    /**
    * Set HPI fiff information
    *
    */
    void SetFiffInfoForHPI();

    void RecvHPIFiffInfo(FiffInfo info);

    void onRecordingElapsedTimeChange();

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

    //=========================================================================================================
    /**
    * Emitted when data is ready
    */
    void DataToSquidCtrlGUI(MatrixXf tmp);
    //=========================================================================================================
    /**
    * Emitted when data received from tcp/ip socket
    */
    void SendCMDDataToSQUIDControl(QByteArray DATA);


protected:
    virtual void run();

private:
    MatrixXd calibrate(const MatrixXf& data);

    bool readProjectors();

    bool readBadChannels();

    void changeRecordingButton();

    void recordingTimerChanged(const QTime & time);

    void recordingTimerStateChanged(bool state);

    QSharedPointer<QTimer> m_pBlinkingRecordButtonTimer;
    qint16 m_iBlinkStatus;

    //=========================================================================================================
    /**
    * Initialises the output connector.
    */
    void initConnector();

    PluginOutputData<NewRealTimeMultiSampleArray>::SPtr m_pRTMSABabyMEG;   /**< The NewRealTimeMultiSampleArray to provide the rt_server Channels.*/

    QMutex mutex;

    QSharedPointer<BabyMEGClient>           myClient;
    QSharedPointer<BabyMEGClient>           myClientComm;
    QSharedPointer<BabyMEGInfo>             pInfo;
    QSharedPointer<BabyMEGProjectDialog>    m_pBabyMEGProjectDialog;
    bool DataStartFlag;

    QSharedPointer<BabyMEGSQUIDControlDgl> SQUIDCtrlDlg; // added by Dr. Limin Sun for nonmodal dialog
    QSharedPointer<babymeghpidgl> HPIDlg; // HPI dialog information

    FiffInfo::SPtr  m_pFiffInfo;            /**< Fiff measurement info.*/
    qint32          m_iBufferSize;          /**< The raw data buffer size.*/

    bool            m_bWriteToFile;         /**< Flag for for writing the received samples to a file. Defined by the user via the GUI.*/
    bool            m_bUseRecordTimer;

    QString             m_sBabyMEGDataPath; /**< The data storage path.*/
    QString             m_sCurrentProject;  /**< The current project which is part of the filename to be recorded.*/
    QString             m_sCurrentSubject;  /**< The current subject which is part of the filename to be recorded.*/
    QString             m_sCurrentParadigm; /**< The current paradigm which is part of the filename to be recorded.*/
    QString             m_sRecordFile;      /**< Current record file. */
    qint32              m_iSplitCount;      /**< File split count */
    QFile               m_qFileOut;         /**< QFile for writing to fif file.*/
    FiffStream::SPtr    m_pOutfid;          /**< FiffStream to write to.*/

    QTime               m_recordTime;

    QSharedPointer<QTimer> m_pUpdateTimeInfo;

    QString                 m_sFiffHeader;  /**< Fiff header information */
    QString                 m_sBadChannels; /**< Filename which contains a list of bad channels */
    RowVectorXd             m_cals;
    SparseMatrix<double>    m_sparseMatCals;

    bool    m_bIsRunning;

    QSharedPointer<RawMatrixBuffer> m_pRawMatrixBuffer;  /**< Holds incoming raw data. */

    QSharedPointer<QTimer>          m_pRecordTimer;                 /**< timer to control recording time */
    QAction*                        m_pActionSetupProject;          /**< shows setup project dialog */
    QAction*                        m_pActionRecordFile;            /**< start recording action */
    QAction*                        m_pActionSqdCtrl;               /**< show squid control */
    QAction*                        m_pActionUpdateFiffInfo;        /**< Update Fiff Info action */
    QAction*                        m_pActionUpdateFiffInfoForHPI;  /**< Update HPI info into Fiff Info action */

public:
    double sfreq;

};


} // NAMESPACE

#endif // BABYMEG_H
