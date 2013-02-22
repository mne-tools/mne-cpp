//=============================================================================================================
/**
* @file		numeric.h
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
* @brief	Contains the declaration of the Numeric class.
*
*/

#ifndef NUMERIC_H
#define NUMERIC_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurement.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CSART
//=============================================================================================================

namespace CSART
{


//=============================================================================================================
/**
* DECLARE CLASS Numeric
*
* @brief The Numeric class is the base class of every Numeric Measurement.
*/
class RTMEASSHARED_EXPORT Numeric : public Measurement
{
public:

    //=========================================================================================================
    /**
    * Constructs a Numeric.
    */
    Numeric();
    //=========================================================================================================
    /**
    * Destroys the Numeric.
    */
    virtual ~Numeric();

    //=========================================================================================================
    /**
    * Sets the unit of the numeric data.
    *
    * @param [in] unit of the data.
    */
    inline void setUnit(const QString& unit);
    //=========================================================================================================
    /**
    * Returns the unit of the numeric measurement.
    *
    * @return the unit of the data of measurement.
    */
    inline const QString& getUnit() const;
    //=========================================================================================================
    /**
    * Sets a value and notify() all attached observers.
    * This method is inherited by Measurement.
    *
    * @param [in] v the value which is set to the Numeric measurement.
    */
    virtual void setValue(double v);
    //=========================================================================================================
    /**
    * Returns the current value.
    * This method is inherited by Measurement.
    *
    * @return the current value of the Numeric measurement.
    */
    virtual double getValue() const;

private:
    QString m_qString_Unit;		/**< Holds unit of the data of the measurement.*/
    double  m_dValue;			/**< Holds current set value.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void Numeric::setUnit(const QString& unit)
{
    m_qString_Unit = unit;
}


//*************************************************************************************************************

inline const QString& Numeric::getUnit() const
{
    return m_qString_Unit;
}

} // NAMESPACE

#endif // NUMERIC_H
