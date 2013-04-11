//=============================================================================================================
/**
* @file     ecgsimulator.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

#include <mne_x/Interfaces/ISensor.h>
#include <generics/circularbuffer_old.h>
#include <xMeas/Measurement/realtimesamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ECGSimulatorModule
//=============================================================================================================

namespace ECGSimulatorModule
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace IOBuffer;


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
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "ecgsimulator.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::ISensor)

    friend class ECGProducer;
    friend class ECGSetupWidget;
    friend class ECGRunWidget;

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

    virtual bool start();
    virtual bool stop();

    virtual Type getType() const;
    virtual const char* getName() const;

    virtual QWidget* setupWidget();
	virtual QWidget* runWidget();

    //=========================================================================================================
    /**
    * Returns the ECGSimulator resource path.
    *
    * @return the ECGSimulator resource path.
    */
    QString getResourcePath(){return m_qStringResourcePath;};


protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialise the ECGSimulator.
    */
    void init();

    RealTimeSampleArray*    m_pRTSA_ECG_I;      /**< Holds the RealTimeSampleArray to provide the channel ECG I.*/
    RealTimeSampleArray*    m_pRTSA_ECG_II;     /**< Holds the RealTimeSampleArray to provide the channel ECG II.*/
    RealTimeSampleArray*    m_pRTSA_ECG_III;    /**< Holds the RealTimeSampleArray to provide the channel ECG III.*/

    float           m_fSamplingRate;            /**< Holds the sampling rate.*/
    int             m_iDownsamplingFactor;      /**< Holds the down sampling factor.*/
    ECGBuffer_old*      m_pInBuffer_I;          /**< Holds ECG I data which arrive from ECG producer.*/
    ECGBuffer_old*      m_pInBuffer_II;         /**< Holds ECG II data which arrive from ECG producer.*/
    ECGBuffer_old*      m_pInBuffer_III;        /**< Holds ECG III data which arrive from ECG producer.*/
    ECGProducer*    m_pECGProducer;             /**< Holds the ECGProducer.*/

    QString m_qStringResourcePath;          /**< Holds the path to the ECG resource directory.*/

    ECGSimChannel* m_pECGChannel_ECG_I;     /**< Holds the simulation channel for ECG I.*/
    ECGSimChannel* m_pECGChannel_ECG_II;    /**< Holds the simulation channel for ECG II.*/
    ECGSimChannel* m_pECGChannel_ECG_III;   /**< Holds the simulation channel for ECG III.*/
};

} // NAMESPACE

#endif // ECGSIMULATOR_H
