//=============================================================================================================
/**
 * @file     tmsi.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     September, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the declaration of the TMSI class.
 *
 */

#ifndef TMSI_H
#define TMSI_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi_global.h"

#include "FormFiles/tmsisetupwidget.h"
#include "FormFiles/tmsimanualannotationwidget.h"
#include "FormFiles/tmsiimpedancewidget.h"
#include "FormFiles/tmsisetupprojectwidget.h"

#include <iostream>
#include <fstream>
#include <direct.h>

#include <scShared/Plugins/abstractsensor.h>
#include <utils/generics/circularbuffer.h>
#include <scMeas/realtimemultisamplearray.h>

#include <utils/layoutloader.h>

#include <fiff/fiff.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>
#include <QTime>
#include <QtConcurrent/QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>
#include <Eigen/Geometry>

//=============================================================================================================
// TMSIPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
    class FiffStream;
}

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
// TMSIPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class TMSIProducer;

//=============================================================================================================
/**
 * TMSI...
 *
 * @brief The TMSI class provides a EEG connector. In order for this plugin to work properly the driver dll "RTINST.dll" must be installed in the system directory. This dll is automatically copied in the system directory during the driver installtion of the TMSi Refa device.
 */
class TMSISHARED_EXPORT TMSI : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "tmsi.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

    friend class TMSIProducer;
    friend class TMSISetupWidget;
    friend class TMSIImpedanceWidget;
    friend class TMSISetupProjectWidget;

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
    virtual QSharedPointer<AbstractPlugin> clone() const;

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
     * Starts the TMSI by starting the tmsi's thread.
     */
    virtual bool start();

    //=========================================================================================================
    /**
     * Stops the TMSI by stopping the tmsi's thread.
     */
    virtual bool stop();

    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    virtual QString getBuildDateTime();

    void setKeyboardTriggerType(int type);

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
     * Opens a widget to check the impedance values
     */
    void showImpedanceDialog();

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
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > m_pRMTSA_TMSI;                  /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<TMSIManualAnnotationWidget>                                          m_pTmsiManualAnnotationWidget;  /**< Widget for manually annotation the trigger during session.*/
    QSharedPointer<TMSIImpedanceWidget>                                                 m_pTmsiImpedanceWidget;         /**< Widget for checking the impedances*/
    QSharedPointer<TMSISetupProjectWidget>                                              m_pTmsiSetupProjectWidget;      /**< Widget for checking the impedances*/

    QString                             m_qStringResourcePath;              /**< The path to the EEG resource directory.*/

    int                                 m_iSamplingFreq;                    /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    int                                 m_iNumberOfChannels;                /**< The number of channels defined by the user via the GUI.*/
    int                                 m_iSamplesPerBlock;                 /**< The samples per block defined by the user via the GUI.*/

    int                                 m_iTriggerInterval;                 /**< The gap between the trigger signals which request the subject to do something (in ms).*/
    QTime                               m_qTimerTrigger;                    /**< Time stemp of the last trigger event (in ms).*/

    bool                                m_bUseChExponent;                   /**< Flag for using the channels exponent. Defined by the user via the GUI.*/
    bool                                m_bUseUnitGain;                     /**< Flag for using the channels unit gain. Defined by the user via the GUI.*/
    bool                                m_bUseUnitOffset;                   /**< Flag for using the channels unit offset. Defined by the user via the GUI.*/
    bool                                m_bWriteDriverDebugToFile;          /**< Flag for for writing driver debug informstions to a file. Defined by the user via the GUI.*/
    bool                                m_bBeepTrigger;                     /**< Flag for using a trigger input.*/
    bool                                m_bUseCommonAverage;                /**< Flag for using common average.*/
    bool                                m_bUseKeyboardTrigger;              /**< Flag for using the keyboard as a trigger input.*/
    bool                                m_bCheckImpedances;                 /**< Flag for checking the impedances of the EEG amplifier.*/

    int                                 m_iTriggerType;                     /**< Holds the trigger type | 0 - no trigger activated, 254 - left, 253 - right, 252 - beep.*/

    QString                             m_sElcFilePath;                     /**< Holds the path for the .elc file (electrode positions). Defined by the user via the GUI.*/
    QSharedPointer<FIFFLIB::FiffInfo>   m_pFiffInfo;                        /**< Fiff measurement info.*/

    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_float>     m_pCircularBuffer;              /**< Holds incoming raw data.*/

    QSharedPointer<TMSIProducer>        m_pTMSIProducer;                    /**< the TMSIProducer.*/

    Eigen::MatrixXf                     m_matOldMatrix;                     /**< Last received sample matrix by the tmsiproducer/tmsidriver class. Used for simple HP filtering.*/

    QMutex                              m_qMutex;                           /**< Holds the threads mutex.*/

    QAction*                            m_pActionImpedance;                 /**< shows impedance widget. */
    QAction*                            m_pActionSetupProject;              /**< shows setup project dialog. */
};
} // NAMESPACE

#endif // TMSI_H
