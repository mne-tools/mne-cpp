//=============================================================================================================
/**
* @file     measurement.h
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
* @brief    Contains the declaration of the Measurement base class.
*
*/

#ifndef MEASUREMENT_H
#define MEASUREMENT_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"
#include "../Nomenclature/nomenclature.h"
#include "../DesignPatterns/observerpattern.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTMEASLIB
//=============================================================================================================

namespace RTMEASLIB
{


//=============================================================================================================
/**
* DECLARE CLASS Measurement
*
* @brief The Measurement class is the base class of every Measurement.
*/
class RTMEASSHARED_EXPORT Measurement : public Subject
{
public:

    //=========================================================================================================
    /**
    * Constructs a Measurement.
    */
    Measurement();
    //=========================================================================================================
    /**
    * Destroys the Measurement.
    */
    virtual ~Measurement();

    //=========================================================================================================
    /**
    * Sets the name of the measurement.
    *
    * @param [in] name which should be set.
    */
    inline void setName(const QString& name);
    //=========================================================================================================
    /**
    * Returns the name of the measurement.
    *
    * @return the name of the measurement.
    */
    inline const QString& getName() const;

    //ToDo
//    inline void setModuleID(MDL_ID::Module_ID);
//    inline MDL_ID::Module_ID getModuleID() const;

    //=========================================================================================================
    /**
    * Sets the id of the measurement.
    *
    * @param [in] id which should be set.
    */
    inline void setID(MSR_ID::Measurement_ID id);
    //=========================================================================================================
    /**
    * Returns the measurement id.
    *
    * @return the measurement id.
    */
    inline MSR_ID::Measurement_ID getID() const;

    //=========================================================================================================
    /**
    * Sets the visibility of the measurement, whether measurement is visible at the display or just data are send invisible.
    *
    * @param [in] visibility of the measurement.
    */
    inline void setVisibility(bool visibility);
    //=========================================================================================================
    /**
    * Returns whether measurement is visible.
    *
    * @return true if measurement is visible, otherwise false.
    */
    inline bool isVisible() const;

    //=========================================================================================================
    /**
    * Sets a value.
    * Pure virtual method.
    *
    * @param [in] value which should be set.
    */
    virtual void setValue(double value) = 0;
    //=========================================================================================================
    /**
    * Returns whether measurement is visible.
    * Pure virtual method.
    *
    * @return true if measurement is visible, otherwise false.
    */
    virtual double getValue() const = 0;

private:
    QString         m_qString_Name;		/**< Holds the name of the measurement. */
//    MDL_ID::Module_ID         m_MDL_ID;		/**< Holds the corresponding module id. */
    MSR_ID::Measurement_ID    m_MSR_ID;		/**< Holds the measurement id. */
    bool        m_bVisibility;		/**< Holds the visibility status. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void Measurement::setName(const QString& name)
{
    m_qString_Name = name;
}


//*************************************************************************************************************

inline const QString& Measurement::getName() const
{
    return m_qString_Name;
}

//*************************************************************************************************************

//inline void Measurement::setModuleID(MDL_ID::Module_ID id)
//{
//	m_MDL_ID = id;
//}


//*************************************************************************************************************

//inline MDL_ID::Module_ID Measurement::getModuleID() const
//{
//    return m_MDL_ID;
//}

//*************************************************************************************************************

inline void Measurement::setID(MSR_ID::Measurement_ID id)
{
	m_MSR_ID = id;
}


//*************************************************************************************************************

inline MSR_ID::Measurement_ID Measurement::getID() const
{
    return m_MSR_ID;
}


//*************************************************************************************************************

inline void Measurement::setVisibility(bool visibility)
{
    m_bVisibility = visibility;
}


//*************************************************************************************************************

inline bool Measurement::isVisible() const
{
    return m_bVisibility;
}


} // NAMESPACE

#endif // MEASUREMENT_H
