//=============================================================================================================
/**
* @file     eegosports.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the EEGoSports class.
*
*/

#ifndef EEGOSPORTS_H
#define EEGOSPORTS_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <iostream>
#include <fstream>
#include <direct.h>

#include "eegosports_global.h"

#include <mne_x/Interfaces/ISensor.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/newrealtimemultisamplearray.h>

#include <utils/asaelc.h>

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

#include "FormFiles/eegosportssetupwidget.h"
#include "FormFiles/eegosportssetupprojectwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE EEGoSportsPlugin
//=============================================================================================================

namespace EEGoSportsPlugin
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

class EEGoSportsProducer;


//=============================================================================================================
/**
* EEGoSports...
*
* @brief The EEGoSports class provides a EEG connector. In order for this plugin to work properly the driver dll "RTINST.dll" must be installed in the system directory. This dll is automatically copied in the system directory during the driver installtion of the TMSi Refa device.
*/
class EEGOSPORTSSHARED_EXPORT EEGoSports : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "eegosports.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::ISensor)

    friend class EEGoSportsProducer;
    friend class EEGoSportsSetupWidget;
    friend class EEGoSportsImpedanceWidget;
    friend class EEGoSportsSetupProjectWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a EEGoSports.
    */
    EEGoSports();

    //=========================================================================================================
    /**
    * Destroys the EEGoSports.
    */
    virtual ~EEGoSports();

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
    * Sets up the fiff info with the current data chosen by the user.
    */
    void setUpFiffInfo();

    //=========================================================================================================
    /**
    * Starts the EEGoSports by starting the tmsi's thread.
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the EEGoSports by stopping the tmsi's thread.
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * Set/Add received samples to a QList.
    */
    void setSampleData(MatrixXf &matRawBuffer);

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

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
    PluginOutputData<NewRealTimeMultiSampleArray>::SPtr m_pRMTSA_EEGoSports;      /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<EEGoSportsSetupProjectWidget> m_pEEGoSportsSetupProjectWidget; /**< Widget for checking the impedances*/

    QString                             m_qStringResourcePath;              /**< The path to the EEG resource directory.*/

    int                                 m_iSamplingFreq;                    /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    int                                 m_iNumberOfChannels;                /**< The samples per block defined by the user via the GUI.*/
    int                                 m_iSamplesPerBlock;                 /**< The number of channels defined by the user via the GUI.*/

    int                                 m_iTriggerInterval;                 /**< The gap between the trigger signals which request the subject to do something (in ms).*/
    QTime                               m_qTimerTrigger;                    /**< Time stemp of the last trigger event (in ms).*/

    bool                                m_bUseChExponent;                   /**< Flag for using the channels exponent. Defined by the user via the GUI.*/
    bool                                m_bWriteToFile;                     /**< Flag for for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                m_bWriteDriverDebugToFile;          /**< Flag for for writing driver debug informstions to a file. Defined by the user via the GUI.*/
    bool                                m_bUseFiltering;                    /**< Flag for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                m_bIsRunning;                       /**< Whether EEGoSports is running.*/
    bool                                m_bBeepTrigger;                     /**< Flag for using a trigger input.*/
    bool                                m_bCheckImpedances;                 /**< Flag for checking the impedances of the EEG amplifier.*/

    ofstream                            m_outputFileStream;                 /**< fstream for writing the samples values to txt file.*/
    QString                             m_sOutputFilePath;                  /**< Holds the path for the sample output file. Defined by the user via the GUI.*/
    QString                             m_sElcFilePath;                     /**< Holds the path for the .elc file (electrode positions). Defined by the user via the GUI.*/
    QFile                               m_fileOut;                          /**< QFile for writing to fif file.*/
    FiffStream::SPtr                    m_pOutfid;                          /**< QFile for writing to fif file.*/
    QSharedPointer<FiffInfo>            m_pFiffInfo;                        /**< Fiff measurement info.*/
    MatrixXd                            m_cals;

    QSharedPointer<RawMatrixBuffer>     m_pRawMatrixBuffer_In;              /**< Holds incoming raw data.*/

    QSharedPointer<EEGoSportsProducer>  m_pEEGoSportsProducer;              /**< the EEGoSportsProducer.*/

    MatrixXf                            m_matOldMatrix;                     /**< Last received sample matrix by the tmsiproducer/tmsidriver class. Used for simple HP filtering.*/

    QMutex                              m_qMutex;                           /**< Holds the threads mutex.*/

    QAction*                            m_pActionSetupProject;              /**< shows setup project dialog */
    QAction*                            m_pActionStartRecording;            /**< starts to record data */

    QSharedPointer<QTimer>              m_pTimerRecordingChange;            /**< timer to control blinking of the recording icon */
    qint16                              m_iBlinkStatus;                     /**< flag for recording icon blinking */

    QList<MatrixXf>                     m_qListReceivedSamples;             /**< list with alle the received samples in form of differentley sized matrices. */

    QMutex                              m_mutex;

};

} // NAMESPACE

#endif // EEGOSPORTS_H
