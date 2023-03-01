//=============================================================================================================
/**
 * @file     fieldline.h
 * @author   Juan GarciaPrieto <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 * @since    0.1.0
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Juan G Prieto, Gabriel B Motta. All rights reserved.
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
 * @brief    Contains the declaration of the Fieldline plugin class.
 *
 */

#ifndef FIELDLINE_H
#define FIELDLINE_H //============================================================================================================= INCLUDES =============================================================================================================

#include "fieldline/fieldline_global.h"
#include "fieldline/fieldline_acq_system_controller.h"
#include "fieldline/fieldline_plugin_gui.h"

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>

#include <scShared/Plugins/abstractsensor.h>
// #include <utils/generics/circularbuffer.h>
#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

// #include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATION
//=============================================================================================================

namespace SCMEASLIB {
class RealTimeMultiSampleArray;
}

namespace FIFFLIB {
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE FIELDLINEPLUGIN
//=============================================================================================================

namespace FIELDLINEPLUGIN {

//=============================================================================================================
// FIELDLINEPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

// class FieldlineProducer;
// class FieldlineSetup;

//=============================================================================================================
/**
 * The Fieldline class provides a EEG connector for receiving data from Fieldline box through its Python API.
 *
 * @brief The Fieldline class provides a EEG connector for receiving data from Fieldline API.
 */
class FIELDLINESHARED_EXPORT Fieldline : public SCSHAREDLIB::AbstractSensor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "fieldline.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

    // friend class FieldlineSetup;

 public:
    //=========================================================================================================
    /**
     * Constructs a Fieldline.
     */
    Fieldline();

    //=========================================================================================================
    /**
     * Destroys the Fieldline.
     */
    virtual ~Fieldline();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;

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
    // void setUpFiffInfo();

    //=========================================================================================================
    /**
     * Starts the Fieldline by starting the tmsi's thread.
     */
    virtual bool start();

    //=========================================================================================================
    /**
     * Stops the Fieldline by stopping the tmsi's thread.
     */
    virtual bool stop();

    virtual AbstractPlugin::PluginType getType() const;

    virtual QString getName() const;

    virtual QWidget* setupWidget();

    virtual QString getBuildInfo();

 protected:
    //=========================================================================================================
    /**
     * Call this function whenenver you received new data.
     *
     * @param[in] matData The new data.
     */
    // void onNewDataAvailable(const Eigen::MatrixXd &matData);

    //=========================================================================================================
    /**
     * the starting point for the thread. after calling start(), the newly created thread calls this function.
     * returning from this method will end the execution of the thread.
     * pure virtual method inherited by qthread.
     */
  virtual void run();
   

  std::unique_ptr<FieldlineAcqSystemController> acqSystem;
  std::unique_ptr<FieldlinePluginGUI> guiWidget;

  // int m_iSamplingFreq;  /**< The sampling frequency defined by the user via the gui (in hertz).*/
  // int m_iNumberChannels;  /**< The number of channels to be received.*/
  // int m_iSamplesPerBlock;  /**< The number of samples per block to be received.*/
  // QString m_qStringResourcePath;  /**< The path to the EEG resource directory.*/
  // QThread m_pProducerThread;  /**< The thread used to host the producer.*/
  // // QSharedPointer<FIELDLINEPLUGIN::FieldlineProducer>      m_pFieldlineProducer;  /**< The producer object.*/
  // QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>  m_pCircularBuffer;     /**< Holds incoming raw data. */
  //
  QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> >     m_pRTMSA_Fieldline;     /**< The RealTimeSampleArray to provide the EEG data.*/
  QSharedPointer<FIFFLIB::FiffInfo> m_pFiffInfo;  /**< Fiff measurement info.*/ };

}  // namespace FIELDLINEPLUGIN

#endif  // FIELDLINE_H
