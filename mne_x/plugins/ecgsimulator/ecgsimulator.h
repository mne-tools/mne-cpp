//=============================================================================================================
/**
* @file		ecgsimulator.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the declaration of the ECGSimulator class.
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

#include "../../mne_x/mne_x/src/interfaces/ISensor.h"
#include "../../MNE/rtMeas/IOBuffer/circularbuffer.h"
#include "../../MNE/rtMeas/Measurement/realtimesamplearray.h"


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

    RealTimeSampleArray*    m_pRTSA_ECG_I;		/**< Holds the RealTimeSampleArray to provide the channel ECG I.*/
    RealTimeSampleArray*    m_pRTSA_ECG_II;		/**< Holds the RealTimeSampleArray to provide the channel ECG II.*/
    RealTimeSampleArray*    m_pRTSA_ECG_III;	/**< Holds the RealTimeSampleArray to provide the channel ECG III.*/

    float           m_fSamplingRate;			/**< Holds the sampling rate.*/
    int             m_iDownsamplingFactor;		/**< Holds the down sampling factor.*/
    ECGBuffer*      m_pInBuffer_I;				/**< Holds ECG I data which arrive from ECG producer.*/
    ECGBuffer*      m_pInBuffer_II;				/**< Holds ECG II data which arrive from ECG producer.*/
    ECGBuffer*      m_pInBuffer_III;			/**< Holds ECG III data which arrive from ECG producer.*/
    ECGProducer*    m_pECGProducer;				/**< Holds the ECGProducer.*/

    QString m_qStringResourcePath;	/**< Holds the path to the ECG resource directory.*/

    ECGSimChannel* m_pECGChannel_ECG_I;		/**< Holds the simulation channel for ECG I.*/
    ECGSimChannel* m_pECGChannel_ECG_II;	/**< Holds the simulation channel for ECG II.*/
    ECGSimChannel* m_pECGChannel_ECG_III;	/**< Holds the simulation channel for ECG III.*/
};

} // NAMESPACE

#endif // ECGSIMULATOR_H
