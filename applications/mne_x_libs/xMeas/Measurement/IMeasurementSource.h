//=============================================================================================================
/**
* @file     IMeasurementSource.h
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
* @brief    Contains the declaration of the IMeasurementSource interface.
*
*/

#ifndef IMEASUREMENTSOURCE_H
#define IMEASUREMENTSOURCE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../xmeas_global.h"
#include "../Nomenclature/nomenclature.h"

#include <mne_x/Interfaces/IPlugin.h>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

//#include <QVector>
#include <QHash>
#include <QList>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XMEASLIB
//=============================================================================================================

namespace XMEASLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Numeric;
class RealTimeSampleArray;
class RealTimeMultiSampleArray;
class RealTimeMultiSampleArrayNew;
class RealTimeSourceEstimate;
class ProgressBar;
class Text;
//class Alert;


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;


//*************************************************************************************************************
//=============================================================================================================
// DECLARE CLASS MeasurementManager
//=============================================================================================================

//Measuerement manager
//=========================================================================================================
/**
* DECLARE INTERFACE IMeasurementSource
*
* @brief The IMeasurementSource class provides an interface for a measurement provider. Todo check what's virtual and write this to inherits todo add measurement providers, if measurement provider with id exists already return pointer to MeasurementProvider
*/
class XMEASSHARED_EXPORT IMeasurementSource : public IPlugin
{
public:
    typedef QSharedPointer<IMeasurementSource> SPtr;               /**< Shared pointer type for IMeasurementSource. */
    typedef QSharedPointer<const IMeasurementSource> ConstSPtr;    /**< Const shared pointer type for IMeasurementSource. */

    //=========================================================================================================
    /**
    * Destroys the IMeasurementSource.
    */
    virtual ~IMeasurementSource();

    //=========================================================================================================
    /**
    * Starts the IPlugin.
    * Pure virtual method.
    *
    * @return true if success, false otherwise
    */
    virtual bool start() = 0;// = 0 call is not longer possible - it has to be reimplemented in child;

    //=========================================================================================================
    /**
    * Stops the IPlugin.
    * Pure virtual method.
    *
    * @return true if success, false otherwise
    */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
    * Returns the plugin type.
    * Pure virtual method.
    *
    * @return type of the IPlugin
    */
    virtual Type getType() const = 0;

    //=========================================================================================================
    /**
    * Returns the plugin name.
    * Pure virtual method.
    *
    * @return the name of plugin.
    */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of the IPlugin.
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
    QSharedPointer<Numeric> addProviderNumeric(MSR_ID::Measurement_ID id);
    //=========================================================================================================
    /**
    * Adds a RealTimeSampleArray measurement to provider.
    *
    * @param [in] id of RealTimeSampleArray measurement.
    * @return pointer to added RealTimeSampleArray measurement.
    */
    QSharedPointer<RealTimeSampleArray> addProviderRealTimeSampleArray(MSR_ID::Measurement_ID id);
    //=========================================================================================================
    /**
    * Adds a RealTimeMultiSampleArray measurement to provider.
    *
    * @param [in] id of RealTimeMultiSampleArray measurement.
    * @param [in] uiNumChannels number of channels of the RTMSA.
    * @return pointer to added RealTimeMultiSampleArray measurement.
    */
    QSharedPointer<RealTimeMultiSampleArray> addProviderRealTimeMultiSampleArray(MSR_ID::Measurement_ID id, unsigned int uiNumChannels);
    //=========================================================================================================
    /**
    * Adds a RealTimeMultiSampleArray measurement to provider.
    *
    * @param [in] id of RealTimeMultiSampleArray measurement.
    * @return pointer to added RealTimeMultiSampleArray measurement.
    */
    QSharedPointer<RealTimeMultiSampleArrayNew> addProviderRealTimeMultiSampleArray_New(MSR_ID::Measurement_ID id);
    //=========================================================================================================
    /**
    * Adds a RealTimeSourceEstimate measurement to provider.
    *
    * @param [in] id of RealTimeSourceEstimate measurement.
    * @return pointer to added RealTimeSourceEstimate measurement.
    */
    QSharedPointer<RealTimeSourceEstimate> addProviderRealTimeSourceEstimate(MSR_ID::Measurement_ID id);
    //=========================================================================================================
    /**
    * Adds a ProgressBar measurement to provider.
    *
    * @param [in] id of ProgressBar measurement.
    * @return pointer to added ProgressBar measurement.
    */
    QSharedPointer<ProgressBar> addProviderProgressBar(MSR_ID::Measurement_ID id);
    //=========================================================================================================
    /**
    * Adds a Text measurement to provider.
    *
    * @param [in] id of Text measurement.
    * @return pointer to added Text measurement.
    */
    QSharedPointer<Text> addProviderText(MSR_ID::Measurement_ID id);

//    virtual QSharedPointer<Alert> addProviderAlert(MSR_ID::Measurement_ID id);

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
    * Returns the id's of provided RealTimeMultiSampleArrayNew measurements.
    *
    * @return a list of all id's of provided RealTimeMultiSampleArrayNew measurements.
    */
    virtual QList<MSR_ID::Measurement_ID> getProviderRTMSA_New_IDs() const;
    //=========================================================================================================
    /**
    * Returns the id's of provided RealTimeSourceEstimates measurements.
    *
    * @return a list of all id's of provided RealTimeSourceEstimates measurements.
    */
    virtual QList<MSR_ID::Measurement_ID> getProviderRTSE_IDs() const;
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
    virtual const QHash<MSR_ID::Measurement_ID, QSharedPointer<Numeric> >& getProviderNumeric() {return m_hashNumeric;};
    //=========================================================================================================
    /**
    * Returns provided RealTimeSampleArray measurements and their id's.
    *
    * @return a hash of all provided RealTimeSampleArray measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, QSharedPointer<RealTimeSampleArray> >& getProviderRTSA() {return m_hashRealTimeSampleArray;};
    //=========================================================================================================
    /**
    * Returns provided RealTimeMultiSampleArray measurements and their id's.
    *
    * @return a hash of all provided RealTimeMultiSampleArray measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, QSharedPointer<RealTimeMultiSampleArray> >& getProviderRTMSA() {return m_hashRealTimeMultiSampleArray;};
    //=========================================================================================================
    /**
    * Returns provided RealTimeMultiSampleArray measurements and their id's.
    *
    * @return a hash of all provided RealTimeMultiSampleArray measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, QSharedPointer<RealTimeMultiSampleArrayNew> >& getProviderRTMSANew() {return m_hashRealTimeMultiSampleArrayNew;};
    //=========================================================================================================
    /**
    * Returns provided ProgressBar measurements and their id's.
    *
    * @return a hash of all provided ProgressBar measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, QSharedPointer<ProgressBar> >& getProviderProgressBar() {return m_hashProgressBar;};
    //=========================================================================================================
    /**
    * Returns provided Text measurements and their id's.
    *
    * @return a hash of all provided Text measurements and their id's.
    */
    virtual const QHash<MSR_ID::Measurement_ID, QSharedPointer<Text> >& getProviderText() {return m_hashText;};

//    virtual const QHash<MSR_ID::Measurement_ID, Alert*>& getProviderAlert() {return m_hashAlert;};

    //=========================================================================================================
    /**
    * Cleans the provider and its measurements.
    */
    virtual void cleanProvider();

protected:
    QHash<MSR_ID::Measurement_ID, QSharedPointer<Numeric> >                   m_hashNumeric;                    /**< The Numeric measurements.*/
    QHash<MSR_ID::Measurement_ID, QSharedPointer<RealTimeSampleArray> >       m_hashRealTimeSampleArray;        /**< The RealTimeSampleArray measurements.*/
    QHash<MSR_ID::Measurement_ID, QSharedPointer<RealTimeMultiSampleArray> >  m_hashRealTimeMultiSampleArray;   /**< The RealTimeSampleArray measurements.*/
    QHash<MSR_ID::Measurement_ID, QSharedPointer<RealTimeMultiSampleArrayNew> > m_hashRealTimeMultiSampleArrayNew;  /**< The RealTimeSampleArray New measurements.*/
    QHash<MSR_ID::Measurement_ID, QSharedPointer<RealTimeSourceEstimate> > m_hashRealTimeSourceEstimate;        /**< The RealTimeSourceEstimate measurements.*/


    QHash<MSR_ID::Measurement_ID, QSharedPointer<ProgressBar> >               m_hashProgressBar;    /**< Holds the ProgressBar measurements.*/
    QHash<MSR_ID::Measurement_ID, QSharedPointer<Text> >                      m_hashText;           /**< Holds the Text measurements.*/
//    QHash<MSR_ID::Measurement_ID, QSharedPointer<Alert> >                 m_hashAlert;	/**< Holds the Alert measurements.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE


#endif // IMEASUREMENTSOURCE_H
