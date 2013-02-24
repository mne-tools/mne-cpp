//=============================================================================================================
/**
* @file		realtimesamplearray.h
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
* @brief	Contains the declaration of the RealTimeSampleArray class.
*
*/

#ifndef REALTIMESAMPLEARRAY_H
#define REALTIMESAMPLEARRAY_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"
#include "measurement.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <QVector>


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

//using namespace IOBuffer;


//=========================================================================================================
/**
* DECLARE CLASS RealTimeSampleArray
*
* @brief The RealTimeSampleArray class is the base class of every RealTimeSampleArray Measurement.
*/
class RTMEASSHARED_EXPORT RealTimeSampleArray : public Measurement
{
public:

    //=========================================================================================================
    /**
    * Constructs a RealTimeSampleArray.
    */
    RealTimeSampleArray();
    //=========================================================================================================
    /**
    * Destroys the RealTimeSampleArray.
    */
    virtual ~RealTimeSampleArray();

    //=========================================================================================================
    /**
    * Sets the minimal value. If current value to set is smaller, current value is set to minimal value.
    *
    * @param [in] minValue minimal value.
    */
    inline void setMinValue(double minValue);
    //=========================================================================================================
    /**
    * Returns the minimal value.
    *
    * @return the minimal value.
    */
    inline double getMinValue() const;

    //=========================================================================================================
    /**
    * Sets the maximal value. If value to set is bigger, current value is set to maximal value.
    *
    * @param [in] maxValue maximal value.
    */
    inline void setMaxValue(double maxValue);
    //=========================================================================================================
    /**
    * Returns the maximal value.
    *
    * @return the maximal value.
    */
    inline double getMaxValue() const;

    //=========================================================================================================
    /**
    * Sets the sampling rate of the RealTimeSampleArray Measurement.
    *
    * @param [in] dSamplingRate the sampling rate of the RealTimeSampleArray.
    */
    inline void setSamplingRate(double dSamplingRate);
    //=========================================================================================================
    /**
    * Returns the sampling rate of the RealTimeSampleArray Measurement.
    *
    * @return the sampling rate of the RealTimeSampleArray.
    */
    inline double getSamplingRate() const;

    //=========================================================================================================
    /**
    * Sets the number of values which should be gathered before attached observers are notified by calling the Subject notify() method.
    *
    * @param [in] ucArraySize the number of values.
    */
    inline void setArraySize(unsigned char ucArraySize);
    //=========================================================================================================
    /**
    * Returns the number of values which should be gathered before attached observers are notified by calling the Subject notify() method.
    *
    * @return the number of values which are gathered before a notify() is called.
    */
    inline unsigned char getArraySize() const;
    //=========================================================================================================
    /**
    * Returns the gathered sample array vector.
    *
    * @return the current sample array vector.
    */
    inline const QVector<double>& getSampleArray();

    //=========================================================================================================
    /**
    * Sets the unit of the RealTimeSampleArray data.
    *
    * @param [in] unit of the data.
    */
    inline void setUnit(const QString& unit);
    //=========================================================================================================
    /**
    * Returns the unit of the RealTimeSampleArray measurement.
    *
    * @return the unit of the data of measurement.
    */
    inline const QString& getUnit() const;

    //=========================================================================================================
    /**
    * Attaches a value to the sample array vector.
    * This method is inherited by Measurement.
    *
    * @param [in] v the value which is attached to the sample array vector.
    */
    virtual void setValue(double v);
    //=========================================================================================================
    /**
    * Returns the current value set.
    * This method is inherited by Measurement.
    *
    * @return the last attached value.
    */
    virtual double getValue() const;

private:
    double              m_dMinValue;		/**< Holds the minimal value.*/
    double              m_dMaxValue;		/**< Holds the maximal value.*/
    double              m_dSamplingRate;	/**< Holds sampling rate of the RealTimeSampleArray.*/
    QString             m_qString_Unit;		/**< Holds unit of the data of the measurement.*/
    double              m_dValue;			/**< Holds the current attached value.*/
    unsigned char       m_ucArraySize;		/**< Holds vector size of the sample array vector.*/
    QVector<double>     m_vecSamples;		/**< Holds the sample array vector.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeSampleArray::setMinValue(double minValue)
{
    m_dMinValue = minValue;
}


//*************************************************************************************************************

inline double RealTimeSampleArray::getMinValue() const
{
    return m_dMinValue;
}


//*************************************************************************************************************

inline void RealTimeSampleArray::setMaxValue(double maxValue)
{
    m_dMaxValue = maxValue;
}


//*************************************************************************************************************

inline double RealTimeSampleArray::getMaxValue() const
{
    return m_dMaxValue;
}


//*************************************************************************************************************

inline void RealTimeSampleArray::setSamplingRate(double dSamplingRate)
{
    m_dSamplingRate = dSamplingRate;
}


//*************************************************************************************************************

inline double RealTimeSampleArray::getSamplingRate() const
{
    return m_dSamplingRate;
}


//*************************************************************************************************************

inline void RealTimeSampleArray::setArraySize(unsigned char ucArraySize)
{
	//Obsolete unsigned char can't be bigger
//    if(ucArraySize > 255)
//        m_ucArraySize = 255;
//    else
        m_ucArraySize = ucArraySize;
}


//*************************************************************************************************************

unsigned char RealTimeSampleArray::getArraySize() const
{
    return m_ucArraySize;
}


//*************************************************************************************************************

inline const QVector<double>& RealTimeSampleArray::getSampleArray()
{
    return m_vecSamples;
}


//*************************************************************************************************************

inline void RealTimeSampleArray::setUnit(const QString& unit)
{
    m_qString_Unit = unit;
}


//*************************************************************************************************************

inline const QString& RealTimeSampleArray::getUnit() const
{
    return m_qString_Unit;
}

} // NAMESPACE

#endif // REALTIMESAMPLEARRAY_H
