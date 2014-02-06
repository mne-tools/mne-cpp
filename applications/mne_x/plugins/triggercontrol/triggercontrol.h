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

#include <mne_x/Interfaces/IAlgorithm.h>
#include <generics/circularbuffer.h>
#include <xMeas/newrealtimesamplearray.h>
#include <xMeas/newrealtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTime>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TriggerControlPlugin
//=============================================================================================================

namespace TriggerControlPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace XMEASLIB;
using namespace IOBuffer;


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
* @brief The TriggerControl ....
*/
class TRIGGERCONTROLSHARED_EXPORT TriggerControl : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "triggercontrol.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::IAlgorithm)

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
    void init();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    void updateSingleChannel(XMEASLIB::NewMeasurement::SPtr pMeasurement);

    void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);
//
    void byteReceived();

signals:
    void sendByte(int value);


protected:
    virtual void run();
    void sendByteTo(int);

private:
    PluginOutputData<NewRealTimeSampleArray>::SPtr  m_pTriggerOutput;   /**< The RealTimeSampleArray of the trigger output.*/
    PluginInputData<NewRealTimeMultiSampleArray>::SPtr  m_pRTMSAInput;  /**< The RealTimeMultiSampleArray input.*/
    PluginInputData<NewRealTimeSampleArray>::SPtr  m_pRTSAInput;

    QVector<int> m_vTimes;

    bool m_bBspBool;

    QSharedPointer<SerialPort> m_pSerialPort;

    qint32 m_iBaud;


    QMutex m_qMutex;

    QVector<VectorXd> m_pData;
    dBuffer::SPtr m_pDataSingleChannel;

    qint32 m_iNumChs;

    QTime m_qTime;

    bool m_bIsRunning;
    bool m_isReceived;


};

} // NAMESPACE

#endif // TRIGGERCONTROL_H
