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
#include <iostream>
#include <fstream>
#include <direct.h>

#include "gusbamp_global.h"

#include <mne_x/Interfaces/ISensor.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/newrealtimemultisamplearray.h>

#include <utils/layoutloader.h>

#include <unsupported/Eigen/FFT>
#include <Eigen/Geometry>


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>
#include <QTime>
#include <QtConcurrent/QtConcurrent>

#include "FormFiles/gusbampsetupwidget.h"
#include "FormFiles/gusbampsetupprojectwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE GUSBAmpPlugin
//=============================================================================================================

namespace GUSBAmpPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace XMEASLIB;
using namespace IOBuffer;
using namespace FIFFLIB;
using namespace std;
using namespace UTILSLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmpProducer;


//=============================================================================================================
/**
* GUSBAmp...
*
* @brief The GUSBAmp class provides an EEG connector for the gTrec USBAmp device.
*/
class GUSBAMPSHARED_EXPORT GUSBAmp : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "gusbamp.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::ISensor)

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
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

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
    * Sets up the fiff info with the current data chosen by the user. Note: Only works for ANT Neuro Waveguard Duke caps.
    */
    void setUpFiffInfo();

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

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    void splitRecordingFile();

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

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

private:
    PluginOutputData<NewRealTimeMultiSampleArray>::SPtr m_pRMTSA_GUSBAmp;   /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<GUSBAmpSetupProjectWidget> m_pGUSBAmpSetupProjectWidget; /**< Widget for setting up the session*/

    QString                             m_qStringResourcePath;              /**< The path to the EEG resource directory.*/

    int                                 m_iSamplingFreq;                    /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    int                                 m_iNumberOfChannels;                /**< The samples per block defined by the user via the GUI.*/
    int                                 m_iSamplesPerBlock;                 /**< The number of channels defined by the user via the GUI.*/
    qint32                              m_iSplitCount;                      /**< File split count */

    int                                 m_iTriggerInterval;                 /**< The gap between the trigger signals which request the subject to do something (in ms).*/
    QTime                               m_qTimerTrigger;                    /**< Time stemp of the last trigger event (in ms).*/

    bool                                m_bUseChExponent;                   /**< Flag for using the channels exponent. Defined by the user via the GUI.*/
    bool                                m_bUseUnitGain;                     /**< Flag for using the channels unit gain. Defined by the user via the GUI.*/
    bool                                m_bUseUnitOffset;                   /**< Flag for using the channels unit offset. Defined by the user via the GUI.*/
    bool                                m_bWriteToFile;                     /**< Flag for for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                m_bWriteDriverDebugToFile;          /**< Flag for for writing driver debug informstions to a file. Defined by the user via the GUI.*/
    bool                                m_bIsRunning;                       /**< Whether GUSBAmp is running.*/
    bool                                m_bBeepTrigger;                     /**< Flag for using a trigger input.*/
    bool                                m_bSplitFile;                       /**< Flag for splitting the recorded file.*/

    int                                 m_iSplitFileSizeMs;                 /**< Holds the size of the splitted files in ms.*/
    int                                 m_iTriggerType;                     /**< Holds the trigger type | 0 - no trigger activated, 254 - left, 253 - right, 252 - beep.*/

    ofstream                            m_outputFileStream;                 /**< fstream for writing the samples values to txt file.*/
    QString                             m_sOutputFilePath;                  /**< Holds the path for the sample output file. Defined by the user via the GUI.*/
    QString                             m_sElcFilePath;                     /**< Holds the path for the .elc file (electrode positions). Defined by the user via the GUI.*/
    QFile                               m_fileOut;                          /**< QFile for writing to fif file.*/
    FiffStream::SPtr                    m_pOutfid;                          /**< QFile for writing to fif file.*/
    QSharedPointer<FiffInfo>            m_pFiffInfo;                        /**< Fiff measurement info.*/
    RowVectorXd                         m_cals;

    QSharedPointer<RawMatrixBuffer>     m_pRawMatrixBuffer_In;              /**< Holds incoming raw data.*/

    QSharedPointer<GUSBAmpProducer>     m_pGUSBAmpProducer;                 /**< the GUSBAmpProducer.*/

    QMutex                              m_qMutex;                           /**< Holds the threads mutex.*/

    QAction*                            m_pActionSetupProject;              /**< shows setup project dialog */
    QAction*                            m_pActionStartRecording;            /**< starts to record data */

    QSharedPointer<QTimer>              m_pTimerRecordingChange;            /**< timer to control blinking of the recording icon */
    qint16                              m_iBlinkStatus;                     /**< flag for recording icon blinking */

};

} // NAMESPACE

#endif // GUSBAMP_H
