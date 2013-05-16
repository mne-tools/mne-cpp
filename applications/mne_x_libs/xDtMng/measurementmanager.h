//=============================================================================================================
/**
* @file     measurementmanager.h
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
* @brief    Contains the declaration of the MeasurementManager class.
*
*/

#ifndef MEASUREMENTMANAGER_H
#define MEASUREMENTMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdtmng_global.h"
#include <xMeas/Nomenclature/nomenclature.h>

#include <generics/observerpattern.h>



#include <mne_x/Interfaces/IPlugin.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
//#include <QVector>
#include <QHash>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace XMEASLIB
{
class IMeasurementSource;
class IMeasurementSink;
}

namespace XDISPLIB
{
class NumericWidget;
class RealTimeSampleArrayWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDTMNGLIB
//=============================================================================================================

namespace XDTMNGLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS MeasurementManager
*
* @brief The MeasurementManager class is caring about all measurement acceptors and providers; and provides methods to attach observers and widgets to measurements.
*/
class XDTMNGSHARED_EXPORT MeasurementManager
{
public:
    typedef QSharedPointer<MeasurementManager> SPtr;               /**< Shared pointer type for MeasurementManager. */
    typedef QSharedPointer<const MeasurementManager> ConstSPtr;    /**< Const shared pointer type for MeasurementManager. */

    //=========================================================================================================
    /**
    * Constructs a MeasurementManager.
    */
    MeasurementManager();

    //=========================================================================================================
    /**
    * Destroys the MeasurementManager.
    */
    virtual ~MeasurementManager();

//add
    //=========================================================================================================
    /**
    * Adds a IMeasurementSource to measurement manager.
    *
    * @param [in] pMSRPrv pointer to IMeasurementSource.
    */
    static void addMeasurementProvider(IMeasurementSource* pMSRPrv);
    //=========================================================================================================
    /**
    * Adds a IMeasurementSink to measurement manager.
    *
    * @param [in] pMSRAcc pointer to IMeasurementSink.
    */
    static void addMeasurementAcceptor(IPlugin* pMSRAcc);//IMeasurementSink* pMSRAcc);

//ToDo with Pattern Visitor

//attach RTSA
    //=========================================================================================================
    /**
    * Attaches an observer to all real-time sample array measurements (subjects) of all measurement providers
    *
    * @param [in] pObserver pointer to observer.
    */
    static void attachToRTSA(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Attaches an observer to all  real-time sample array measurements (subjects) of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    */
    static void attachToRTSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Attaches an observer to given real-time sample array measurements (subjects) list of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    * @param [in] msr_idList list of measurement id's where the observer should be attached to.
    */
    static void attachToRTSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);
    //=========================================================================================================
    /**
    * Attaches widgets to all real-time sample arrays of measurement provider list of given plugin
    *
    * @param [in] plg_id id of the plugin.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetsToRTSA(PLG_ID::Plugin_ID plg_id, QSharedPointer<QTime> t);
    //=========================================================================================================
    /**
    * Attaches a widget to a specific real-time sample array measurement of given plugin.
    *
    * @param [in] plg_id id of the plugin.
    * @param [in] msr_id id of the measurement.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetToRTSA(PLG_ID::Plugin_ID plg_id, MSR_ID::Measurement_ID msr_id, QSharedPointer<QTime> t);
//detach RTSA
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time sample array measurements (subjects) which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    */
    static void detachFromRTSA(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time sample array measurements (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    */
    static void detachFromRTSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Detaches an observer of specific real-time sample array measurements (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    * @param [in] msr_idList list of measurement id's where the observer should be detached from.
    */
    static void detachFromRTSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);

//attach RTMS
    //=========================================================================================================
    /**
    * Attaches an observer to all real-time multi sample array measurements (subjects) of all measurement providers
    *
    * @param [in] pObserver pointer to observer.
    */
    static void attachToRTMSA(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Attaches an observer to all  real-time multi sample array measurements (subjects) of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    */
    static void attachToRTMSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Attaches an observer to given real-time multi sample array measurements (subjects) list of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    * @param [in] msr_idList list of measurement id's where the observer should be attached to.
    */
    static void attachToRTMSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);
    //=========================================================================================================
    /**
    * Attaches widgets to all real-time multi sample array of measurement provider list of given plugin
    *
    * @param [in] plg_id id of the plugin.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetsToRTMSA(PLG_ID::Plugin_ID plg_id, QSharedPointer<QTime> t);
    //=========================================================================================================
    /**
    * Attaches a widget to a specific real-time multi sample array measurement of given plugin.
    *
    * @param [in] plg_id id of the plugin.
    * @param [in] msr_id id of the measurement.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetToRTMSA(PLG_ID::Plugin_ID plg_id, MSR_ID::Measurement_ID msr_id, QSharedPointer<QTime> t);
//detach RTMS
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time multi sample array measurements (subjects) which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    */
    static void detachFromRTMSA(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time multi sample array measurements (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    */
    static void detachFromRTMSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Detaches an observer of specific real-time multi sample array measurements (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    * @param [in] msr_idList list of measurement id's where the observer should be detached from.
    */
    static void detachFromRTMSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);

//attach RTMSANew
    //=========================================================================================================
    /**
    * Attaches an observer to all real-time multi sample array measurements (subjects) of all measurement providers
    *
    * @param [in] pObserver pointer to observer.
    */
    static void attachToRTMSANew(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Attaches an observer to all  real-time multi sample array measurements (subjects) of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    */
    static void attachToRTMSANew(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Attaches an observer to given real-time multi sample array measurements (subjects) list of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    * @param [in] msr_idList list of measurement id's where the observer should be attached to.
    */
    static void attachToRTMSANew(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);
    //=========================================================================================================
    /**
    * Attaches widgets to all real-time multi sample array of measurement provider list of given plugin
    *
    * @param [in] plg_id id of the plugin.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetsToRTMSANew(PLG_ID::Plugin_ID plg_id, QSharedPointer<QTime> t);
    //=========================================================================================================
    /**
    * Attaches a widget to a specific real-time multi sample array measurement of given plugin.
    *
    * @param [in] plg_id id of the plugin.
    * @param [in] msr_id id of the measurement.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetToRTMSANew(PLG_ID::Plugin_ID plg_id, MSR_ID::Measurement_ID msr_id, QSharedPointer<QTime> t);
//detach RTMSANew
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time multi sample array measurements (subjects) which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    */
    static void detachFromRTMSANew(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time multi sample array measurements (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    */
    static void detachFromRTMSANew(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Detaches an observer of specific real-time multi sample array measurements (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    * @param [in] msr_idList list of measurement id's where the observer should be detached from.
    */
    static void detachFromRTMSANew(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);

//attach RTSE
    //=========================================================================================================
    /**
    * Attaches an observer to all real-time source estimate (subjects) of all measurement providers
    *
    * @param [in] pObserver pointer to observer.
    */
    static void attachToRTSE(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Attaches an observer to all  real-time source estimate (subjects) of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    */
    static void attachToRTSE(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Attaches an observer to given real-time source estimate (subjects) list of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    * @param [in] msr_idList list of measurement id's where the observer should be attached to.
    */
    static void attachToRTSE(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);
    //=========================================================================================================
    /**
    * Attaches widgets to all real-time multi sample array of measurement provider list of given plugin
    *
    * @param [in] plg_id id of the plugin.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetsToRTSE(PLG_ID::Plugin_ID plg_id, QSharedPointer<QTime> t);
    //=========================================================================================================
    /**
    * Attaches a widget to a specific real-time multi sample array measurement of given plugin.
    *
    * @param [in] plg_id id of the plugin.
    * @param [in] msr_id id of the measurement.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetToRTSE(PLG_ID::Plugin_ID plg_id, MSR_ID::Measurement_ID msr_id, QSharedPointer<QTime> t);
//detach RTSE
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time source estimate (subjects) which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    */
    static void detachFromRTSE(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time source estimate (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    */
    static void detachFromRTSE(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Detaches an observer of specific real-time source estimate (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    * @param [in] msr_idList list of measurement id's where the observer should be detached from.
    */
    static void detachFromRTSE(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);

//attach Numeric
    //=========================================================================================================
    /**
    * Attaches an observer to all numeric measurements (subjects) of all measurement providers
    *
    * @param [in] pObserver pointer to observer.
    */
    static void attachToNumeric(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Attaches an observer to all numeric measurements (subjects) of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    */
    static void attachToNumeric(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Attaches an observer to given numeric measurements (subjects) list of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] plg_idList list of plugin (measurement provider) id's where the observer should attached to.
    * @param [in] msr_idList list of measurement id's where the observer should be attached to.
    */
    static void attachToNumeric(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);
    //=========================================================================================================
    /**
    * Attaches widgets to all numeric of measurement provider list of given plugin
    *
    * @param [in] plg_id id of the plugin.
    */
    static void attachWidgetsToNumeric(PLG_ID::Plugin_ID plg_id);
    //=========================================================================================================
    /**
    * Attaches a widget to a specific numeric measurement of given plugin.
    *
    * @param [in] plg_id id of the plugin.
    * @param [in] msr_id id of the measurement.
    */
    static void attachWidgetToNumeric(PLG_ID::Plugin_ID plg_id, MSR_ID::Measurement_ID msr_id) ;
//detach RTSA
    //=========================================================================================================
    /**
    * Detaches an observer of all numeric measurements (subjects) which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    */
    static void detachFromNumeric(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Detaches an observer of all numeric measurements (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    */
    static void detachFromNumeric(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList);
    //=========================================================================================================
    /**
    * Detaches an observer of specific numeric measurements (subjects) of given plugins which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] plg_idList list of plugin id's where the observer should be detached from.
    * @param [in] msr_idList list of measurement id's where the observer should be detached from.
    */
    static void detachFromNumeric(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList);

//detach Widgets
    //=========================================================================================================
    /**
    * Detaches all current widgets
    */
    static void detachWidgets();
    //=========================================================================================================
    /**
    * Detaches all widgets of given plugin
    *
    * @param [in] plg_idList list of plugin id's where widgets should be detached from.
    */
    static void detachWidgets(QList<PLG_ID::Plugin_ID> plg_idList);

//get
    //=========================================================================================================
    /**
    * Returns all current measurement providers.
    *
    * @return a hash of all current measurement providers and their corresponding plugin id's.
    */
    static QHash<PLG_ID::Plugin_ID, IMeasurementSource*>& getMeasurementProvider() {return s_hashMeasurementProvider;};
    //=========================================================================================================
    /**
    * Returns all current measurement acceptors.
    *
    * @return a hash of all current measurement acceptors and their corresponding plugin id's.
    */
    static QHash<PLG_ID::Plugin_ID, IPlugin*>& getMeasurementAcceptors() {return s_hashMeasurementAcceptor;};

//clean
    //=========================================================================================================
    /**
    * Cleans measurement provider and acceptor hash's.
    */
    static void clean();

private:
    static QHash<PLG_ID::Plugin_ID, IMeasurementSource*>    s_hashMeasurementProvider;  /**< Holds the measurement providers.*/
    static QHash<PLG_ID::Plugin_ID, IPlugin*>    s_hashMeasurementAcceptor;             /**< Holds the measurement acceptors.*/
};

} // NAMESPACE

#endif // RTMEASUREMENTMANAGER_H
