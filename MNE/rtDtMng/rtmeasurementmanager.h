//=============================================================================================================
/**
* @file     rtmeasurementmanager.h
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
* @brief    Contains the declaration of the RTMeasurementManager class.
*
*/

#ifndef RTMEASUREMENTMANAGER_H
#define RTMEASUREMENTMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtdtmng_global.h"
#include <rtMeas/Nomenclature/nomenclature.h>

#include <generics/observerpattern.h>



#include <mne_x/Interfaces/IModule.h>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

//#include <QVector>
#include <QHash>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace RTMEASLIB
{
class IMeasurementSource;
class IMeasurementSink;
}

namespace DISPLIB
{
class NumericWidget;
class RealTimeSampleArrayWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTDTMNGLIB
//=============================================================================================================

namespace RTDTMNGLIB
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
* DECLARE CLASS RTMeasurementManager
*
* @brief The RTMeasurementManager class is caring about all measurement acceptors and providers; and provides methods to attach observers and widgets to measurements.
*/
class RTDTMNGSHARED_EXPORT RTMeasurementManager
{
public:

    //=========================================================================================================
    /**
    * Constructs a RTMeasurementManager.
    */
    RTMeasurementManager();

    //=========================================================================================================
    /**
    * Destroys the RTMeasurementManager.
    */
    virtual ~RTMeasurementManager();

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
    static void addMeasurementAcceptor(IModule* pMSRAcc);//IMeasurementSink* pMSRAcc);

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
    * @param [in] mdl_idList list of module (measurement provider) id's where the observer should attached to.
    */
    static void attachToRTSA(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList);
    //=========================================================================================================
    /**
    * Attaches an observer to given real-time sample array measurements (subjects) list of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] mdl_idList list of module (measurement provider) id's where the observer should attached to.
    * @param [in] msr_idList list of measurement id's where the observer should be attached to.
    */
    static void attachToRTSA(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList, QList<MSR_ID::Measurement_ID> msr_idList);
    //=========================================================================================================
    /**
    * Attaches widgets to all real-time sample arrays of measurement provider list of given module
    *
    * @param [in] mdl_id id of the module.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetsToRTSA(MDL_ID::Module_ID mdl_id, QTime* t);
    //=========================================================================================================
    /**
    * Attaches a widget to a specific real-time sample array measurement of given module.
    *
    * @param [in] mdl_id id of the module.
    * @param [in] msr_id id of the measurement.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetToRTSA(MDL_ID::Module_ID mdl_id, MSR_ID::Measurement_ID msr_id, QTime* t);
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
    * Detaches an observer of all real-time sample array measurements (subjects) of given modules which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] mdl_idList list of module id's where the observer should be detached from.
    */
    static void detachFromRTSA(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList);
    //=========================================================================================================
    /**
    * Detaches an observer of specific real-time sample array measurements (subjects) of given modules which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] mdl_idList list of module id's where the observer should be detached from.
    * @param [in] msr_idList list of measurement id's where the observer should be detached from.
    */
    static void detachFromRTSA(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList, QList<MSR_ID::Measurement_ID> msr_idList);

//attach RTSM
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
    * @param [in] mdl_idList list of module (measurement provider) id's where the observer should attached to.
    */
    static void attachToRTMSA(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList);
    //=========================================================================================================
    /**
    * Attaches an observer to given real-time multi sample array measurements (subjects) list of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] mdl_idList list of module (measurement provider) id's where the observer should attached to.
    * @param [in] msr_idList list of measurement id's where the observer should be attached to.
    */
    static void attachToRTMSA(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList, QList<MSR_ID::Measurement_ID> msr_idList);
    //=========================================================================================================
    /**
    * Attaches widgets to all real-time multi sample array of measurement provider list of given module
    *
    * @param [in] mdl_id id of the module.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetsToRTMSA(MDL_ID::Module_ID mdl_id, QTime* t);
    //=========================================================================================================
    /**
    * Attaches a widget to a specific real-time multi sample array measurement of given module.
    *
    * @param [in] mdl_id id of the module.
    * @param [in] msr_id id of the measurement.
    * @param [in] t pointer to the application measurement time.
    */
    static void attachWidgetToRTMSA(MDL_ID::Module_ID mdl_id, MSR_ID::Measurement_ID msr_id, QTime* t);
//detach RTSM
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time multi sample array measurements (subjects) which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    */
    static void detachFromRTMSA(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Detaches an observer of all real-time multi sample array measurements (subjects) of given modules which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] mdl_idList list of module id's where the observer should be detached from.
    */
    static void detachFromRTMSA(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList);
    //=========================================================================================================
    /**
    * Detaches an observer of specific real-time multi sample array measurements (subjects) of given modules which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] mdl_idList list of module id's where the observer should be detached from.
    * @param [in] msr_idList list of measurement id's where the observer should be detached from.
    */
    static void detachFromRTMSA(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList, QList<MSR_ID::Measurement_ID> msr_idList);

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
    * @param [in] mdl_idList list of module (measurement provider) id's where the observer should attached to.
    */
    static void attachToNumeric(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList);
    //=========================================================================================================
    /**
    * Attaches an observer to given numeric measurements (subjects) list of given measurement providers list
    *
    * @param [in] pObserver pointer to observer.
    * @param [in] mdl_idList list of module (measurement provider) id's where the observer should attached to.
    * @param [in] msr_idList list of measurement id's where the observer should be attached to.
    */
    static void attachToNumeric(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList, QList<MSR_ID::Measurement_ID> msr_idList);
    //=========================================================================================================
    /**
    * Attaches widgets to all numeric of measurement provider list of given module
    *
    * @param [in] mdl_id id of the module.
    */
    static void attachWidgetsToNumeric(MDL_ID::Module_ID mdl_id);
    //=========================================================================================================
    /**
    * Attaches a widget to a specific numeric measurement of given module.
    *
    * @param [in] mdl_id id of the module.
    * @param [in] msr_id id of the measurement.
    */
    static void attachWidgetToNumeric(MDL_ID::Module_ID mdl_id, MSR_ID::Measurement_ID msr_id) ;
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
    * Detaches an observer of all numeric measurements (subjects) of given modules which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] mdl_idList list of module id's where the observer should be detached from.
    */
    static void detachFromNumeric(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList);
    //=========================================================================================================
    /**
    * Detaches an observer of specific numeric measurements (subjects) of given modules which contains the observer.
    *
    * @param [in] pObserver pointer to observer which should be detached.
    * @param [in] mdl_idList list of module id's where the observer should be detached from.
    * @param [in] msr_idList list of measurement id's where the observer should be detached from.
    */
    static void detachFromNumeric(IObserver* pObserver, QList<MDL_ID::Module_ID> mdl_idList, QList<MSR_ID::Measurement_ID> msr_idList);

//detach Widgets
    //=========================================================================================================
    /**
    * Detaches all current widgets
    */
    static void detachWidgets();
    //=========================================================================================================
    /**
    * Detaches all widgets of given module
    *
    * @param [in] mdl_idList list of module id's where widgets should be detached from.
    */
    static void detachWidgets(QList<MDL_ID::Module_ID> mdl_idList);

//get
    //=========================================================================================================
    /**
    * Returns all current measurement providers.
    *
    * @return a hash of all current measurement providers and their corresponding module id's.
    */
    static QHash<MDL_ID::Module_ID, IMeasurementSource*>& getMeasurementProvider() {return s_hashMeasurementProvider;};
    //=========================================================================================================
    /**
    * Returns all current measurement acceptors.
    *
    * @return a hash of all current measurement acceptors and their corresponding module id's.
    */
    static QHash<MDL_ID::Module_ID, IModule*>& getMeasurementAcceptors() {return s_hashMeasurementAcceptor;};

//clean
    //=========================================================================================================
    /**
    * Cleans measurement provider and acceptor hash's.
    */
    static void clean();

private:
    static QHash<MDL_ID::Module_ID, IMeasurementSource*>    s_hashMeasurementProvider;	/**< Holds the measurement providers.*/
    static QHash<MDL_ID::Module_ID, IModule*>    s_hashMeasurementAcceptor;	/**< Holds the measurement acceptors.*/
};

} // NAMESPACE

#endif // RTMEASUREMENTMANAGER_H
