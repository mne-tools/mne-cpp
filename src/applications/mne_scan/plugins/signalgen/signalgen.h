//=============================================================================================================
/**
 * @file     signalgen.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the SignalGen class.
 *
 */

#ifndef SIGNALGEN_H
#define SIGNALGEN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "signalgen_global.h"

#include <scShared/Plugins/abstractsensor.h>
#include <communication/rtClient/rtcmdclient.h>
#include <utils/generics/circularbuffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>
#include <QTimer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB {
    class RealTimeMultiSampleArray;
}

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE SIGNALGENPLUGIN
//=============================================================================================================

namespace SIGNALGENPLUGIN
{

//=============================================================================================================
// SIGNALGENPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class SignalGenProducer;

//=============================================================================================================
/**
 * DECLARE CLASS SignalGen
 *
 * @brief The SignalGen class provides a stream of data.
 */
class SIGNALGENSHARED_EXPORT SignalGen : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "signalgen.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

    friend class SignalGenProducer;
    friend class SignalGenSetupWidget;

public:

    enum Mode{
        Zero,
        Sine,
        Noise
    };

    //=========================================================================================================
    /**
     * Constructs a SignalGen.
     */
    SignalGen();

    //=========================================================================================================
    /**
     * Destroys the SignalGen.
     */
    virtual ~SignalGen();

    //=========================================================================================================
    /**
     * Clears the rt server
     */
    void clear();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<AbstractPlugin> clone() const;

    //=========================================================================================================
    /**
     * Initialise the SignalGen.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    virtual void unload();

    virtual bool start();
    virtual bool stop();

    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QString getBuildInfo();

    //=========================================================================================================
    /**
     * Creates the setup widget.
     *
     * @return Returns the setup widget.
     */
    virtual QWidget* setupWidget();

protected:
    //=========================================================================================================
    /**
     * Initialises the output.
     */
    void initOutput();

    virtual void run();

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr m_pRTMSA_SignalGen;     /**< The RealTimeMultiSampleArray to provide the rt_server Channels.*/

    QSharedPointer<FIFFLIB::FiffInfo>                           m_pFiffInfo;                /**< Fiff measurement info.*/

    QMutex                  m_qMutex;                       /**< The mutex to ensure thread safety.*/

    int numChannels;
    int samplesPerBlock;

    float sample_freq;

    Mode    m_mode;

};
} // NAMESPACE

#endif // SIGNALGEN_H
