//=============================================================================================================
/**
* @file     ISink.h
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
* @brief    Contains the declaration of the ISink interface.
*
*/

#ifndef ISINK_H
#define ISINK_H



//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"
#include "../Nomenclature/nomenclature.h"

#include <generics/circularbuffer_old.h>
#include <generics/buffer_old.h>

#include <generics/observerpattern.h>


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
* DECLARE INTERFACE ISink
*
* @brief The ISink class provides an interface for a measurement acceptor. Todo check what's virtual and write this to inherits
*/
class RTMEASSHARED_EXPORT ISink : public IObserver
{
public:

    //=========================================================================================================
    /**
    * Constructs a ISink.
    */
    ISink();
    //=========================================================================================================
    /**
    * Destroys the ISink.
    */
    virtual ~ISink();

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
    void addAcceptorMeasurementBuffer(MSR_ID::Measurement_ID id, Buffer_old* buffer);
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
    Buffer_old* getAcceptorMeasurementBuffer(MSR_ID::Measurement_ID id);

    //=========================================================================================================
    /**
    * Cleans accepted measurements.
    */
    void cleanAcceptor();

protected:
    QList<MDL_ID::Module_ID>          m_qList_MDL_ID;	/**< Modules of which are measurements accepted of current module.*/

    QHash<MSR_ID::Measurement_ID, Buffer_old*>*         m_pHashBuffers;	/**< accepted measurements */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QList<MDL_ID::Module_ID> ISink::getAcceptorModule_IDs() const
{
    return m_qList_MDL_ID;
}


//*************************************************************************************************************

inline QList<MSR_ID::Measurement_ID> ISink::getAcceptorMeasurement_IDs() const
{
    return m_pHashBuffers->keys();
}

} // NAMESPACE

#endif // ISINK_H
