//=============================================================================================================
/**
* @file     numeric.h
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
* @brief    Contains the declaration of the Numeric class.
*
*/

#ifndef NUMERIC_H
#define NUMERIC_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../xmeas_global.h"
#include "measurement.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XMEASLIB
//=============================================================================================================

namespace XMEASLIB
{


//=============================================================================================================
/**
* DECLARE CLASS Numeric
*
* @brief The Numeric class is the base class of every Numeric Measurement.
*/
class XMEASSHARED_EXPORT Numeric : public Measurement
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
