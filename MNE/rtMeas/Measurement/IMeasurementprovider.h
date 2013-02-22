//=============================================================================================================
/**
* @file		IMeasurementprovider.h
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
* @brief	Contains the declaration of the IMeasurementProvider interface.
*
*/

#ifndef MEASUREMENTPROVIDER_H
#define MEASUREMENTPROVIDER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"
#include "../Nomenclature/nomenclature.h"

#include "../../../mne_x/mne_x/src/interfaces/IModule.h" //ToDo: this is not allowed to be relative


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

//#include <QVector>
#include <QHash>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CSART
//=============================================================================================================

namespace CSART
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Numeric;
class RealTimeSampleArray;
class RealTimeMultiSampleArray;
class ProgressBar;
class Text;
//class Alert;


//*************************************************************************************************************
//=============================================================================================================
// DECLARE CLASS MeasurementManager
//=============================================================================================================

//Measuerement manager
//=========================================================================================================
/**
* DECLARE INTERFACE IMeasurementProvider
*
* @brief The IMeasurementProvider class provides an interface for a measurement provider. Todo check what's virtual and write this to inherits todo add measurement providers, if measurement provider with id exists already return pointer to MeasurementProvider
*/
class RTMEASSHARED_EXPORT IMeasurementProvider : public IModule
{
public:

    //=========================================================================================================
    /**
    * Destroys the IMeasurementProvider.
    */
    virtual ~IMeasurementProvider();








    //=========================================================================================================
    /**
    * Starts the IModule.
    * Pure virtual method.
    *
    * @return true if success, false otherwise
    */
    virtual bool start() = 0;// = 0 call is not longer possible - it has to be reimplemented in child;

    //=========================================================================================================
    /**
    * Stops the IModule.
    * Pure virtual method.
    *
    * @return true if success, false otherwise
    */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
    * Returns the module type.
    * Pure virtual method.
    *
    * @return type of the IModule
    */
    virtual Type getType() const = 0;

    //=========================================================================================================
    /**
    * Returns the module name.
    * Pure virtual method.
    *
    * @return the name of module.
    */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of the IModule.
    * Pure virtual method.
    *
    * @return the setup widget.
    */
    virtual QWidget* setupWidget() = 0; //setup()

    //=========================================================================================================
    /**
    * Returns the widget which is shown under configuration tab while running mode.
    * Pure virtual method.
    *
    * @return the run widget.
    */
    virtual QWidget* runWidget() = 0;






    //=========================================================================================================
    /**
    * Adds a Numeric measurement to provider.
    *
    * @param [in] id of Numeric measurement.
    * @return pointer to added Numeric measurement.
    */
    Numeric* addProviderNumeric(MSR_ID::Measurement_ID id);
    //=========================================================================================================
    /**
    * Adds a RealTimeSampleArray measurement to provider.
    *
    * @param [in] id of RealTimeSampleArray measurement.
    * @return pointer to added RealTimeSampleArray measurement.
    */
    RealTimeSampleArray* addProviderRealTimeSampleArray(MSR_ID::Measurement_ID id);
    //=========================================================================================================
    /**
    * Adds a RealTimeMultiSampleArray measurement to provider.
    *
    * @param [in] id of RealTimeMultiSampleArray measurement.
    * @param [in] uiNumChannels number of channels of the RTMSA.
    * @return pointer to added RealTimeMultiSampleArray measurement.
    */
    RealTimeMultiSampleArray* addProviderRealTimeMultiSampleArray(MSR_ID::Measurement_ID id, unsigned int uiNumChannels);
    //=========================================================================================================
    /**
    * Adds a ProgressBar measurement to provider.
    *
    * @param [in] id of ProgressBar measurement.
    * @return pointer to added ProgressBar measurement.
    */
    ProgressBar* addProviderProgressBar(MSR_ID::Measurement_ID id);
    //=========================================================================================================
    /**
    * Adds a Text measurement to provider.
    *
    * @param [in] id of Text measurement.
    * @return pointer to added Text measurement.
    */
    Text* addProviderText(MSR_ID::Measurement_ID id);

//    virtual Alert* addProviderAlert(MSR_ID::Measurement_ID id);

    //=========================================================================================================
    /**
    * Returns the id's of provided measurements.
    *
    * @return a list of all id's of provided measurements.
    */
    virtual QList<MSR_ID::Measurement_ID> getProviderMeasurement_IDs() const;
    //=========================================================================================================
    /**
    * Returns the id's of provided Numeric measurements.
    *
    * @return a list of all id's of provided Numeric measurements.
    */
    virtual QList<MSR_ID::Measurement_ID> getProviderNumeric_IDs() const;
    //=========================================================================================================
    /**
    * Returns the id's of provided RealTimeSampleArray measurements.
    *
    * @return a list of all id's of provided RealTimeSampleArray measurements.
    */
    virtual QList<MSR_ID::Measurement_ID> getProviderRTSA_IDs() const;
    //=========================================================================================================
    /**
    * Returns the id's of provided RealTimeMultiSampleArray measurements.
    *
    * @return a list of all id's of provided RealTimeMultiSampleArray measurements.
    */
    virtual QList<MSR_ID::Measurement_ID> getProviderRTMSA_IDs() const;
    //=========================================================================================================
    /**
    * Returns the id's of provided ProgressBar measurements.
    *
    * @return a list of all id's of provided ProgressBar measurements.
    */
    virtual QList<MSR_ID::Measurement_ID> getProviderProgressbar_IDs() const;
    //=========================================================================================================
    /**
    * Returns the id's of provided Text measurements.
    *
    * @return a list of all id's of provided Text measurements.
    */
    virtual QList<MSR_ID::Measurement_ID> getProviderText_IDs() const;

//    virtual QList<MSR_ID::Measurement_ID> getProviderAlert_IDs() const;

    //=========================================================================================================
    /**
    * Returns provided Numeric measurements and their id's.
    *
    * @return a hash of all provided Numeric measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, Numeric*>& getProviderNumeric() {return m_hashNumeric;};
    //=========================================================================================================
    /**
    * Returns provided RealTimeSampleArray measurements and their id's.
    *
    * @return a hash of all provided RealTimeSampleArray measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, RealTimeSampleArray*>& getProviderRTSA() {return m_hashRealTimeSampleArray;};
    //=========================================================================================================
    /**
    * Returns provided RealTimeMultiSampleArray measurements and their id's.
    *
    * @return a hash of all provided RealTimeMultiSampleArray measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, RealTimeMultiSampleArray*>& getProviderRTMSA() {return m_hashRealTimeMultiSampleArray;};
    //=========================================================================================================
    /**
    * Returns provided ProgressBar measurements and their id's.
    *
    * @return a hash of all provided ProgressBar measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, ProgressBar*>& getProviderProgressBar() {return m_hashProgressBar;};
    //=========================================================================================================
    /**
    * Returns provided Text measurements and their id's.
    *
    * @return a hash of all provided Text measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, Text*>& getProviderText() {return m_hashText;};

//    virtual const QHash<MSR_ID::Measurement_ID, Alert*>& getProviderAlert() {return m_hashAlert;};

    //=========================================================================================================
    /**
    * Cleans the provider and its measurements.
    */
    virtual void cleanProvider();

protected:
    QHash<MSR_ID::Measurement_ID, Numeric*>                   m_hashNumeric;				/**< Holds the Numeric measurements.*/
    QHash<MSR_ID::Measurement_ID, RealTimeSampleArray*>       m_hashRealTimeSampleArray;	/**< Holds the RealTimeSampleArray measurements.*/
    QHash<MSR_ID::Measurement_ID, RealTimeMultiSampleArray*>      m_hashRealTimeMultiSampleArray;	/**< Holds the RealTimeSampleArray measurements.*/
    QHash<MSR_ID::Measurement_ID, ProgressBar*>               m_hashProgressBar;			/**< Holds the ProgressBar measurements.*/
    QHash<MSR_ID::Measurement_ID, Text*>                      m_hashText;		/**< Holds the Text measurements.*/
//    QHash<MSR_ID::Measurement_ID, Alert*>                 m_hashAlert;	/**< Holds the Alert measurements.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE


#endif // MEASUREMENTPROVIDER_H
