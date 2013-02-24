//=============================================================================================================
/**
* @file		IMeasurementacceptor.h
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
* @brief	Contains the declaration of the IMeasurementAcceptor interface.
*
*/

#ifndef MEASUREMENTACCEPTOR_H
#define MEASUREMENTACCEPTOR_H



//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"
#include "../Nomenclature/nomenclature.h"

#include "../IOBuffer/circularbuffer.h"

#include "../DesignPatterns/observerpattern.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

//#include <QVector>
#include <QHash>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTMEASLIB
//=============================================================================================================

namespace RTMEASLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//using namespace CSART;
using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//class Numeric;
//class RealTimeSampleArray;
//class ProgressBar;
//class Text;
////class Alert;

//=========================================================================================================
/**
* DECLARE INTERFACE IMeasurementAcceptor
*
* @brief The IMeasurementAcceptor class provides an interface for a measurement acceptor. Todo check what's virtual and write this to inherits
*/
class RTMEASSHARED_EXPORT IMeasurementAcceptor : public IObserver
{
public:

    //=========================================================================================================
    /**
    * Constructs a IMeasurementAcceptor.
    */
    IMeasurementAcceptor();
    //=========================================================================================================
    /**
    * Destroys the IMeasurementAcceptor.
    */
    virtual ~IMeasurementAcceptor();

    //=========================================================================================================
    /**
    * Updates the IObserver.
    *
    * @param [in] pSubject pointer to the subject where observer is attached to.
    */
    virtual void update(Subject* pSubject) = 0;

    //=========================================================================================================
    /**
    * Adds a module which should be accepted by the acceptor
    *
    * @param [in] id which should be accepted.
    */
    void addModule(MDL_ID::Module_ID id);

    //=========================================================================================================
    /**
    * Returns the module types from which are measurements accepted
    *
    * @return list of module id's of modules from which measurements are accepted.
    */
    inline QList<MDL_ID::Module_ID> getAcceptorModule_IDs() const;

    //=========================================================================================================
    /**
    * Adds id's of measurements and pointers to their buffers of measurements which should be accepted.
    *
    * @param [in] id of measurement which should be added.
    * @param [in] buffer pointer to the corresponding buffer of the accepted measurement.
    */
    void addAcceptorMeasurementBuffer(MSR_ID::Measurement_ID id, Buffer* buffer);
    //=========================================================================================================
    /**
    * Returns accepted measurements.
    *
    * @return id's of measurements which are accepted.
    */
    inline QList<MSR_ID::Measurement_ID> getAcceptorMeasurement_IDs() const;
    //=========================================================================================================
    /**
    * Returns the buffer of a specific accepted measurement.
    *
    * @param [in] id of measurement of which the buffer should be returned.
    * @return the buffer of the requested measurement.
    */
    Buffer* getAcceptorMeasurementBuffer(MSR_ID::Measurement_ID id);

    //=========================================================================================================
    /**
    * Cleans accepted measurements.
    */
    void cleanAcceptor();

protected:
    QList<MDL_ID::Module_ID>          m_qList_MDL_ID;	/**< Modules of which are measurements accepted of current module.*/

    QHash<MSR_ID::Measurement_ID, Buffer*>*         m_pHashBuffers;	/**< accepted measurements */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QList<MDL_ID::Module_ID> IMeasurementAcceptor::getAcceptorModule_IDs() const
{
    return m_qList_MDL_ID;
}


//*************************************************************************************************************

inline QList<MSR_ID::Measurement_ID> IMeasurementAcceptor::getAcceptorMeasurement_IDs() const
{
    return m_pHashBuffers->keys();
}

} // NAMESPACE

#endif // MEASUREMENTACCEPTOR_H
