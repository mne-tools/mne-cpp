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

#include "tmsi_global.h"

#include "tmsichannel.h"

#include <mne_x/Interfaces/ISensor.h>
#include <generics/circularbuffer.h>
#include <xMeas/newrealtimesamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>


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


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TMSIProducer;
//class ECGChannel;


//=============================================================================================================
/**
* TMSI...
*
* @brief The TMSI class provides a EEG connector.
*/
class TMSISHARED_EXPORT TMSI : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "tmsi.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::ISensor)

    friend class TMSIProducer;
    friend class TMSISetupWidget;

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
    * Initialise the ECGSimulator.
    */
    void initChannels();


    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * Returns the ECGSimulator resource path.
    *
    * @return the ECGSimulator resource path.
    */
    QString getResourcePath() const {return m_qStringResourcePath;}

protected:
    virtual void run();

private:
    PluginOutputData<NewRealTimeSampleArray>::SPtr m_pRTSA_TMSI_I_new;   /**< The RealTimeSampleArray to provide the channel ECG I.*/
    PluginOutputData<NewRealTimeSampleArray>::SPtr m_pRTSA_TMSI_II_new;  /**< The RealTimeSampleArray to provide the channel ECG II.*/
    PluginOutputData<NewRealTimeSampleArray>::SPtr m_pRTSA_TMSI_III_new; /**< The RealTimeSampleArray to provide the channel ECG III.*/

    float           m_fSamplingRate;        /**< the sampling rate.*/
    int             m_iDownsamplingFactor;  /**< the down sampling factor.*/
    dBuffer::SPtr   m_pInBuffer_I;          /**< ECG I data which arrive from ECG producer.*/
    dBuffer::SPtr   m_pInBuffer_II;         /**< ECG II data which arrive from ECG producer.*/
    dBuffer::SPtr   m_pInBuffer_III;        /**< ECG III data which arrive from ECG producer.*/
    QSharedPointer<TMSIProducer>     m_pTMSIProducer; /**< the ECGProducer.*/

    QString m_qStringResourcePath;          /**< the path to the ECG resource directory.*/

    TMSIChannel::SPtr m_pTMSIChannel_TMSI_I;    /**< the simulation channel for ECG I.*/
    TMSIChannel::SPtr m_pTMSIChannel_TMSI_II;   /**< the simulation channel for ECG II.*/
    TMSIChannel::SPtr m_pTMSIChannel_TMSI_III;  /**< the simulation channel for ECG III.*/
};

} // NAMESPACE

#endif // ECGSIMULATOR_H
