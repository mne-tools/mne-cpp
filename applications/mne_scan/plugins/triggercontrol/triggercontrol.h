//=============================================================================================================
/**
* @file     triggercontrol.h
* @author   Tim Kunze <tim.kunze@tu-ilmenau.de>
*           Luise Lang <luise.lang@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Tim Kunze, Luise Lang and Christoph Dinh. All rights reserved.
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
* @brief    Contains the declaration of the TriggerControl class.
*
*/

#ifndef TRIGGERCONTROL_H
#define TRIGGERCONTROL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "triggercontrol_global.h"

#include <scShared/Interfaces/IAlgorithm.h>
#include <utils/generics/circularbuffer.h>
#include <utils/generics/circularmatrixbuffer.h>
#include <scMeas/realtimesamplearray.h>
#include <scMeas/realtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTime>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TRIGGERCONTROLPLUGIN
//=============================================================================================================

namespace TRIGGERCONTROLPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class SettingsWidget;
class SerialPort;


//=============================================================================================================
/**
* DECLARE CLASS TriggerControl
*

* @brief The TriggerControl is a MNE Scan plugin which contains an intuitive terminal for manual
* configurations of output channels and an automated processing of connected signal channels.
*
*/
class TRIGGERCONTROLSHARED_EXPORT TriggerControl : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "triggercontrol.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

    friend class TriggerControlSetupWidget;
    friend class SettingsWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a TriggerControl.
    */
    TriggerControl();

    //=========================================================================================================
    /**
    * Destroys the TriggerControl.
    */
    ~TriggerControl();

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
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Starts the TriggerControl by starting the triggercontrol's thread.
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the TriggerControl by starting the triggercontrol's thread.
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * [...]
    */
    virtual IPlugin::PluginType getType() const;

    //=========================================================================================================
    /**
    * [...]
    */
    virtual QString getName() const;

    //=========================================================================================================
    /**
    * [...]
    */    
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * [...]
    */
    void updateSingleChannel(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * [...]
    */
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Initialise input and output connectors.
    */
    void byteReceived();

signals:
//    void sendByte(int value);
    void sendByte(int value, int channel);

protected:
    //=========================================================================================================
    /**
    * Runs the run method
    */
    virtual void run();

    //=========================================================================================================
    /**
    * Sets or Unsets the HardWired channel from the terminal function (see manual)
    */

    void sendByteTo(int value, int channel);

private:
    PluginOutputData<RealTimeSampleArray>::SPtr  m_pTriggerOutput;   /**< The RealTimeSampleArray of the trigger output.*/
    PluginInputData<RealTimeMultiSampleArray>::SPtr  m_pRTMSAInput;  /**< The RealTimeMultiSampleArray input.*/
    PluginInputData<RealTimeSampleArray>::SPtr  m_pRTSAInput;

    QVector<int> m_vTimes;

    bool m_bBspBool;

    QSharedPointer<SerialPort> m_pSerialPort;

    qint32 m_iBaud;

    QMutex m_qMutex;

    CircularMatrixBuffer<double>::SPtr m_pDataMatrixBuffer;   /**< Holds incoming rt server data.*/

    QVector<VectorXd> m_pData;
    dBuffer::SPtr m_pDataSingleChannel;

    qint32 m_iNumChs;

    QTime m_qTime;

    bool m_bIsRunning;
    bool m_isReceived;

    //alpha locked stuff

    double m_fs;
    double m_dt;
    double m_refFreq;
    double m_alphaFreq;

    VectorXd m_refSin;
    VectorXd m_vecCorr;

    double corr(VectorXd a, VectorXd b);
};

} // NAMESPACE

#endif // TRIGGERCONTROL_H
