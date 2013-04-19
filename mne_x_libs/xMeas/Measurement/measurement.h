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

#include "../xmeas_global.h"
#include "../Nomenclature/nomenclature.h"
#include <generics/observerpattern.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XMEASLIB
//=============================================================================================================

namespace XMEASLIB
{


//=============================================================================================================
/**
* DECLARE CLASS Measurement
*
* @brief The Measurement class is the base class of every Measurement.
*/
class XMEASSHARED_EXPORT Measurement : public Subject
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
    * Sets the name of the Measurement.
    *
    * @param [in] name which should be set.
    */
    inline void setName(const QString& name);

    //=========================================================================================================
    /**
    * Returns the name of the Measurement.
    *
    * @return the name of the Measurement.
    */
    inline const QString& getName() const;

    //ToDo
//    inline void setModuleID(MDL_ID::Module_ID);
//    inline MDL_ID::Module_ID getModuleID() const;

    //=========================================================================================================
    /**
    * Sets the id of the Measurement.
    *
    * @param [in] id which should be set.
    */
    inline void setID(MSR_ID::Measurement_ID id); //OLD

    //=========================================================================================================
    /**
    * Sets the id of the Measurement.
    *
    * @param[in] p_MDL_ID   modul id
    * @param[in] p_MSR_idx  measurement index
    */
    inline void setID_new(MDL_ID::Module_ID p_MDL_ID, qint32 p_MSR_idx); //NEW

    //=========================================================================================================
    /**
    * Returns the Measurement id.
    *
    * @return the Measurement id.
    */
    inline MSR_ID::Measurement_ID getID() const;

    //=========================================================================================================
    /**
    * Sets the visibility of the Measurement, whether Measurement is visible at the display or just data are send invisible.
    *
    * @param [in] visibility of the Measurement.
    */
    inline void setVisibility(bool visibility);
    //=========================================================================================================
    /**
    * Returns whether Measurement is visible.
    *
    * @return true if Measurement is visible, otherwise false.
    */
    inline bool isVisible() const;

private:
    QString         m_qString_Name;         /**< Holds the name of the Measurement. */
//    MDL_ID::Module_ID         m_MDL_ID;     /**< Holds the corresponding module id. */
    MSR_ID::Measurement_ID    m_MSR_ID;     /**< Holds the Measurement id. */
    bool        m_bVisibility;              /**< Holds the visibility status. */

    /////////////// NEW
    MDL_ID::Module_ID   m_MDL_ID_new;     /**< Holds the corresponding module id. */
    qint32              m_iMSR_idx;

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

////*************************************************************************************************************

//inline void Measurement::setModuleID(MDL_ID::Module_ID id)
//{
//    m_MDL_ID = id;
//}


////*************************************************************************************************************

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

inline void Measurement::setID_new(MDL_ID::Module_ID p_MDL_ID, qint32 p_MSR_idx)
{
    m_MDL_ID_new = p_MDL_ID;
    m_iMSR_idx = p_MSR_idx;
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
