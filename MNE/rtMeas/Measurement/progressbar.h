//=============================================================================================================
/**
* @file		progressbar.h
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
* @brief	Contains the declaration of the ProgressBar class.
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
// DEFINE NAMESPACE CSART
//=============================================================================================================

namespace CSART
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
