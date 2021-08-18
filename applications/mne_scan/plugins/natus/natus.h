//=============================================================================================================
/**
 * @file     natus.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the Natus class.
 *
 */

#ifndef NATUS_H
#define NATUS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "natus_global.h"

#include <scShared/Plugins/abstractsensor.h>
#include <utils/generics/circularbuffer.h>
#include <utils/buildtime.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QDebug>

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
// DEFINE NAMESPACE NATUSPLUGIN
//=============================================================================================================

namespace NATUSPLUGIN
{

//=============================================================================================================
// NATUSPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class NatusProducer;
class NatusSetup;

//=============================================================================================================
/**
 * The Natus class provides a EEG connector for receiving data from Natus UDP SDK.
 *
 * @brief The Natus class provides a EEG connector for receiving data from Natus UDP SDK.
 */
class NATUSSHARED_EXPORT Natus : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "natus.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

    friend class NatusSetup;

public:
    //=========================================================================================================
    /**
     * Constructs a Natus.
     */
    Natus();

    //=========================================================================================================
    /**
     * Destroys the Natus.
     */
    virtual ~Natus();

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
    void setUpFiffInfo();

    //=========================================================================================================
    /**
     * Starts the Natus by starting the tmsi's thread.
     */
    virtual bool start();

    //=========================================================================================================
    /**
     * Stops the Natus by stopping the tmsi's thread.
     */
    virtual bool stop();

    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildDateTime();

protected:
    //=========================================================================================================
    /**
     * Call this function whenenver you received new data.
     *
     * @param[in] matData The new data.
     */
    void onNewDataAvailable(const Eigen::MatrixXd &matData);

    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread.
     */
    virtual void run();

    int                     m_iSamplingFreq;                /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    int                     m_iNumberChannels;              /**< The number of channels to be received.*/
    int                     m_iSamplesPerBlock;             /**< The number of samples per block to be received.*/

    QString                 m_qStringResourcePath;          /**< The path to the EEG resource directory.*/

    QThread                                                 m_pProducerThread;          /**< The thread used to host the producer.*/
    QSharedPointer<NATUSPLUGIN::NatusProducer>              m_pNatusProducer;           /**< The producer object.*/
    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>  m_pCircularBuffer;          /**< Holds incoming raw data. */

    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> >     m_pRMTSA_Natus;     /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<FIFFLIB::FiffInfo>                                                       m_pFiffInfo;        /**< Fiff measurement info.*/
};
} // NAMESPACE

#endif // NATUS_H
