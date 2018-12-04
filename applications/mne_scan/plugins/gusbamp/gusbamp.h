//=============================================================================================================
/**
* @file     gusbamp.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the GUSBAmp class.
*
*/

#ifndef GUSBAMP_H
#define GUSBAMP_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbamp_global.h"
#include <scShared/Interfaces/ISensor.h>
#include <utils/generics/circularmatrixbuffer.h>
#include <scMeas/newrealtimemultisamplearray.h>
#include <fiff/fiff.h>

#include "FormFiles/gusbampsetupwidget.h"
#include "FormFiles/gusbampsetupprojectwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE GUSBAMPPLUGIN
//=============================================================================================================

namespace GUSBAMPPLUGIN
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmpProducer;


//=============================================================================================================
/**
* GUSBAmp...
*
* @brief The GUSBAmp class provides an EEG connector for the gTec USBAmp device.
*/
class GUSBAMPSHARED_EXPORT GUSBAmp : public SCSHAREDLIB::ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "gusbamp.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class GUSBAmpProducer;
    friend class GUSBAmpSetupWidget;
    friend class GUSBAmpSetupProjectWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a GUSBAmp.
    */
    GUSBAmp();

    //=========================================================================================================
    /**
    * Destroys the GUSBAmp.
    */
    virtual ~GUSBAmp();

    //=========================================================================================================
    /**
    * building all setting for the FIFF-data-stream
    */
    void setUpFiffInfo();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Initialise input and output connectors.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();

    //=========================================================================================================
    /**
    * Starts the GUSBAmp by starting the GUSBAmp's thread.
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the GUSBAmp by stopping the GUSBAmp's thread.
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * Opens a dialog to setup the project to check the impedance values
    */
    void showSetupProjectDialog();

    //=========================================================================================================
    /**
    * Starts data recording
    */
    void showStartRecording();

    //=========================================================================================================
    /**
    * Implements blinking recording button
    */
    void changeRecordingButton();

    //=========================================================================================================
    /**
    * Checks if a dir exists
    */
    bool dirExists(const std::string& dirName_in);

    //=========================================================================================================
    /**
    * returns the type of the plug in
    */
    virtual SCSHAREDLIB::IPlugin::PluginType getType() const;

    //=========================================================================================================
    /**
    * returns the name of the plugin
    */
    virtual QString getName() const;

    //=========================================================================================================
    /**
    * setups the widget
    */
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * splits the recorded FIFF file
    */
    void splitRecordingFile();

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr m_pRTMSA_GUSBAmp;               /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<GUSBAmpSetupProjectWidget>                                   m_pGUSBampSetupProjectWidget;   /**< Widget for setup the project file*/

    QSharedPointer<IOBUFFER::RawMatrixBuffer>     m_pRawMatrixBuffer_In;    /**< Holds incoming raw data.*/

    QString                             m_qStringResourcePath;              /**< The path to the EEG resource directory.*/
    bool                                m_bIsRunning;                       /**< Whether GUSBAmp is running.*/
    QSharedPointer<GUSBAmpProducer>     m_pGUSBAmpProducer;                 /**< the GUSBAmpProducer.*/
    QSharedPointer<FIFFLIB::FiffInfo>   m_pFiffInfo;                        /**< Fiff measurement info.*/

    std::vector<QString>        m_vSerials;                 /**< vector of all Serials (the first one is the master) */
    int                         m_iSampleRate;              /**< the sample rate in Hz (see documentation of the g.USBamp API for details on this value and the NUMBER_OF_SCANS!)*/
    int                         m_iSamplesPerBlock;         /**< The samples per block defined by the user via the GUI. */
    UCHAR                       m_iNumberOfChannels;        /**< the channels that should be acquired from each device */
    std::vector<int>            m_viSizeOfSampleMatrix;     /**< vector including the size of the two dimensional sample Matrix */
    std::vector<int>            m_viChannelsToAcquire;      /**< vector of the calling numbers of the channels to be acquired */
    bool                        m_bWriteToFile;             /**< Flag for File writing*/
    FIFFLIB::FiffStream::SPtr   m_pOutfid;                  /**< QFile for writing to fif file.*/
    Eigen::RowVectorXd          m_cals;
    bool                        m_bSplitFile;               /**< Flag for splitting the recorded file.*/
    int                         m_iSplitFileSizeMs;         /**< Holds the size of the splitted files in ms.*/
    int                         m_iSplitCount;              /**< File split count */
    QString                     m_sOutputFilePath;          /**< Holds the path for the sample output file. Defined by the user via the GUI.*/
    QFile                       m_fileOut;                  /**< QFile for writing to fiff file.*/
    QSharedPointer<QTimer>      m_pTimerRecordingChange;    /**< timer to control blinking of the recording icon */
    qint16                      m_iBlinkStatus;             /**< flag for recording icon blinking */
    QAction*                    m_pActionStartRecording;    /**< starts to record data */
    QAction*                    m_pActionSetupProject;      /**< shows setup project dialog */

};

} // NAMESPACE

#endif // GUSBAMP_H
