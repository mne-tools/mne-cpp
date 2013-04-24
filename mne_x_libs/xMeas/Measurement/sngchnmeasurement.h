//=============================================================================================================
/**
* @file     sngchnmeasurement.h
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
* @brief    Contains the declaration of the SngChnMeasurement base class.
*
*/

#ifndef SNGCHNMEASUREMENT_H
#define SNGCHNMEASUREMENT_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../xmeas_global.h"
#include "../Nomenclature/nomenclature.h"
#include "measurement.h"


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
* DECLARE CLASS SngChnMeasurement
*
* @brief The SngChnMeasurement class is the base class of every SngChnMeasurement.
*/
class XMEASSHARED_EXPORT SngChnMeasurement : public Measurement
{
public:
    //=========================================================================================================
    /**
    * Constructs a SngChnMeasurement.
    */
    SngChnMeasurement();

    //=========================================================================================================
    /**
    * Destroys the SngChnMeasurement.
    */
    virtual ~SngChnMeasurement();

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
    * Returns whether SngChnMeasurement is visible.
    * Pure virtual method.
    *
    * @return true if SngChnMeasurement is visible, otherwise false.
    */
    virtual double getValue() const = 0;

    //=========================================================================================================
    /**
    * Returns whether Measurement is single channel measurement.
    *
    * @return true if Measurement is single channel measurement, otherwise false.
    */
    virtual bool isSingleChannel() const;

private:

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // SNGCHNMEASUREMENT_H
