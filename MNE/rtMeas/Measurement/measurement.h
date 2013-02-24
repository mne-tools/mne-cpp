//=============================================================================================================
/**
* @file		measurement.h
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
* @brief	Contains the declaration of the Measurement base class.
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
