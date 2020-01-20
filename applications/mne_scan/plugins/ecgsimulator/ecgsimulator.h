//=============================================================================================================
/**
 * @file     ecgsimulator.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
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
 * @brief    Contains the declaration of the ECGSimulator class.
 *
 */

#ifndef ECGSIMULATOR_H
#define ECGSIMULATOR_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecgsimulator_global.h"

#include "ecgsimchannel.h"

#include <scShared/Interfaces/ISensor.h>
#include <utils/generics/circularbuffer.h>
#include <scMeas/realtimesamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ECGSIMULATORPLUGIN
//=============================================================================================================

namespace ECGSIMULATORPLUGIN
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

class ECGProducer;
//class ECGChannel;


//=============================================================================================================
/**
 * DECLARE CLASS ECGSimulator
 *
 * @brief The ECGSimulator class provides a ECG simulator.
 */
class ECGSIMULATORSHARED_EXPORT ECGSimulator : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "ecgsimulator.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class ECGProducer;
    friend class ECGSetupWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a ECGSimulator.
     */
    ECGSimulator();

    //=========================================================================================================
    /**
     * Destroys the ECGSimulator.
     */
    virtual ~ECGSimulator();

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
    PluginOutputData<RealTimeSampleArray>::SPtr m_pRTSA_ECG_I_new;   /**< The RealTimeSampleArray to provide the channel ECG I.*/
    PluginOutputData<RealTimeSampleArray>::SPtr m_pRTSA_ECG_II_new;  /**< The RealTimeSampleArray to provide the channel ECG II.*/
    PluginOutputData<RealTimeSampleArray>::SPtr m_pRTSA_ECG_III_new; /**< The RealTimeSampleArray to provide the channel ECG III.*/

    float           m_fSamplingRate;        /**< the sampling rate.*/
    int             m_iDownsamplingFactor;  /**< the down sampling factor.*/
    dBuffer::SPtr   m_pInBuffer_I;          /**< ECG I data which arrive from ECG producer.*/
    dBuffer::SPtr   m_pInBuffer_II;         /**< ECG II data which arrive from ECG producer.*/
    dBuffer::SPtr   m_pInBuffer_III;        /**< ECG III data which arrive from ECG producer.*/
    QSharedPointer<ECGProducer>     m_pECGProducer; /**< the ECGProducer.*/

    QString m_qStringResourcePath;          /**< the path to the ECG resource directory.*/

    ECGSimChannel::SPtr m_pECGChannel_ECG_I;    /**< the simulation channel for ECG I.*/
    ECGSimChannel::SPtr m_pECGChannel_ECG_II;   /**< the simulation channel for ECG II.*/
    ECGSimChannel::SPtr m_pECGChannel_ECG_III;  /**< the simulation channel for ECG III.*/

    bool m_bIsRunning;  /**< Whether main thread is running */

};

} // NAMESPACE

#endif // ECGSIMULATOR_H
