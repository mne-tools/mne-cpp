//=============================================================================================================
/**
* @file     progressbar.h
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
* @brief    Contains the declaration of the ProgressBar class.
*
*/

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurement.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTMEASLIB
//=============================================================================================================

namespace RTMEASLIB
{

//=============================================================================================================
/**
* DECLARE CLASS ProgressBar
*
* @brief The ProgressBar class is the base class of every ProgressBar Measurement.
*/
class RTMEASSHARED_EXPORT ProgressBar : public Measurement
{
public:

    //=========================================================================================================
    /**
    * Constructs a ProgressBar.
    */
    ProgressBar();
    //=========================================================================================================
    /**
    * Destroys the ProgressBar.
    */
    virtual ~ProgressBar();

    //=========================================================================================================
    /**
    * Sets the minimal value. If current value to set is smaller, current value is set to minimal value.
    *
    * @param [in] iMin minimal value.
    */
    inline void setMinScale(int iMin);
    //=========================================================================================================
    /**
    * Returns the minimal value.
    *
    * @return the minimal value.
    */
    inline int getMinScale() const;

    //=========================================================================================================
    /**
    * Sets the maximal value. If value to set is bigger, current value is set to maximal value.
    *
    * @param [in] iMax maximal value.
    */
    inline void setMaxScale(int iMax);
    //=========================================================================================================
    /**
    * Returns the maximal value.
    *
    * @return the maximal value.
    */
    inline int getMaxScale() const;

    //=========================================================================================================
    /**
    * Sets a value and notify() all attached observers.
    * This Method is inherited by Measurement.
    *
    * @param [in] v the value which is set to the ProgressBar measurement.
    */
    void setValue(double v);
    //=========================================================================================================
    /**
    * Returns the current value.
    * This method is inherited by Measurement.
    *
    * @return the current value of the Numeric measurement.
    */
    virtual double getValue() const;

private:
    int m_iMin;		/**< Holds the minimal value.*/
    int m_iMax;		/**< Holds the maximal value.*/
    int m_iValue;	/**< Holds the current value.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void ProgressBar::setMinScale(int iMin)
{
    m_iMin = iMin;
}


//*************************************************************************************************************

inline int ProgressBar::getMinScale() const
{
    return m_iMin;
}


//*************************************************************************************************************

inline void ProgressBar::setMaxScale(int iMax)
{
    m_iMax = iMax;
}


//*************************************************************************************************************

inline int ProgressBar::getMaxScale() const
{
    return m_iMax;
}


//*************************************************************************************************************

inline double ProgressBar::getValue() const
{
    return m_iValue;
}

} // NAMESPACE

#endif // PROGRESSBAR_H
