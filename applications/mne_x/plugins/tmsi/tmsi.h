//=============================================================================================================
/**
* @file     tmsi.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     September, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the declaration of the TMSI class.
*
*/

#ifndef TMSI_H
#define TMSI_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <iostream>
#include <fstream>

#include "tmsi_global.h"

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

#include "FormFiles/tmsisetupwidget.h"
#include "FormFiles/tmsimanualannotationwidget.h"
#include "FormFiles/tmsiimpedancewidget.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace TMSIPlugin
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

class TMSIProducer;


//=============================================================================================================
/**
* TMSI...
*
* @brief The TMSI class provides a EEG connector. In order for this plugin to work properly the driver dll "RTINST.dll" must be installed in the system directory. This dll is automatically copied in the system directory during the driver installtion of the TMSi Refa device.
*/
class TMSISHARED_EXPORT TMSI : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "tmsi.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::ISensor)

    friend class TMSIProducer;
    friend class TMSISetupWidget;
    friend class TMSIImpedanceWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a TMSI.
    */
    TMSI();

    //=========================================================================================================
    /**
    * Destroys the TMSI.
    */
    virtual ~TMSI();

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
    * Sets up the fiff info with the current data chosen by the user.
    */
    void setUpFiffInfo();

    //=========================================================================================================
    /**
    * Starts the TMSI by starting the tmsi's thread.
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the TMSI by stopping the tmsi's thread.
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * Opens a dialog to check the impedance values
    */
    void showImpedanceDialog();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    void setKeyboardTriggerType(int type);

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    PluginOutputData<NewRealTimeMultiSampleArray>::SPtr m_pRMTSA_TMSI;      /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<TMSIManualAnnotationWidget> m_tmsiManualAnnotationWidget;/**< Widget for manually annotation the trigger during session.*/

    QString                             m_qStringResourcePath;              /**< The path to the EEG resource directory.*/

    int                                 m_iSamplingFreq;                    /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    int                                 m_iNumberOfChannels;                /**< The samples per block defined by the user via the GUI.*/
    int                                 m_iSamplesPerBlock;                 /**< The number of channels defined by the user via the GUI.*/

    int                                 m_iTriggerInterval;                 /**< The gap between the trigger signals which request the subject to do something (in ms).*/
    QTime                               m_qTimerTrigger;                    /**< Time stemp of the last trigger event (in ms).*/

    bool                                m_bUseChExponent;                   /**< Flag for using the channels exponent. Defined by the user via the GUI.*/
    bool                                m_bUseUnitGain;                     /**< Flag for using the channels unit gain. Defined by the user via the GUI.*/
    bool                                m_bUseUnitOffset;                   /**< Flag for using the channels unit offset. Defined by the user via the GUI.*/
    bool                                m_bWriteToFile;                     /**< Flag for for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                m_bWriteDriverDebugToFile;          /**< Flag for for writing driver debug informstions to a file. Defined by the user via the GUI.*/
    bool                                m_bUseFiltering;                    /**< Flag for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                m_bIsRunning;                       /**< Whether TMSI is running.*/
    bool                                m_bUseFFT;                          /**< Flag for using FFT. Defined by the user via the GUI.*/
    bool                                m_bBeepTrigger;                     /**< Flag for using a trigger input.*/
    bool                                m_bUseCommonAverage;                /**< Flag for using common average.*/
    bool                                m_bUseKeyboardTrigger;              /**< Flag for using the keyboard as a trigger input.*/
    bool                                m_bCheckImpedances;                 /**< Flag for checking the impedances of the EEG amplifier.*/

    int                                 m_iTriggerType;                     /**< Holds the trigger type | 0 - no trigger activated, 254 - left, 253 - right, 252 - beep.*/

    ofstream                            m_outputFileStream;                 /**< fstream for writing the samples values to txt file.*/
    QString                             m_sOutputFilePath;                  /**< Holds the path for the sample output file. Defined by the user via the GUI.*/
    QString                             m_sElcFilePath;                     /**< Holds the path for the .elc file (electrode positions). Defined by the user via the GUI.*/
    QFile                               m_fileOut;                          /**< QFile for writing to fif file.*/
    FiffStream::SPtr                    m_pOutfid;                          /**< QFile for writing to fif file.*/
    QSharedPointer<FiffInfo>            m_pFiffInfo;                        /**< Fiff measurement info.*/
    MatrixXd                            m_cals;

    QSharedPointer<RawMatrixBuffer>     m_pRawMatrixBuffer_In;              /**< Holds incoming raw data.*/

    QSharedPointer<TMSIProducer>        m_pTMSIProducer;                    /**< the TMSIProducer.*/

    MatrixXf                            m_matOldMatrix;                     /**< Last received sample matrix by the tmsiproducer/tmsidriver class. Used for simple HP filtering.*/

    QMutex                              m_qMutex;                           /**< Holds the threads mutex.*/

    QAction*                            m_pActionImpedance;                 /**< shows setup project dialog */

    QSharedPointer<TMSIImpedanceWidget> m_pTmsiImpedanceWidget;             /**< Widget for checking the impedances*/
};

} // NAMESPACE

#endif // TMSI_H
