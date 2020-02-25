//=============================================================================================================
/**
 * @file     realtimesamplearray.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the declaration of the RealTimeSampleArray class.
 *
 */

#ifndef REALTIMESAMPLEARRAY_H
#define REALTIMESAMPLEARRAY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>


//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{


//=========================================================================================================
/**
 * DECLARE CLASS RealTimeSampleArray
 *
 * @brief The RealTimeSampleArray class is the base class of every RealTimeSampleArray Measurement.
 */
class SCMEASSHARED_EXPORT RealTimeSampleArray : public Measurement
{
    Q_OBJECT
public:
    typedef QSharedPointer<RealTimeSampleArray> SPtr;               /**< Shared pointer type for RealTimeSampleArray. */
    typedef QSharedPointer<const RealTimeSampleArray> ConstSPtr;    /**< Const shared pointer type for RealTimeSampleArray. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeSampleArray.
     *
     * @param[in] parent     the QObject parent of this measurement
     */
    RealTimeSampleArray(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeSampleArray.
     */
    virtual ~RealTimeSampleArray();

    //=========================================================================================================
    /**
     * Clears all the data stored in the buffer.
     */
    void clear();

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
    mutable QMutex      m_qMutex;           /**< Mutex to ensure thread safety */

    double              m_dMinValue;        /**< Holds the minimal value.*/
    double              m_dMaxValue;        /**< Holds the maximal value.*/
    double              m_dSamplingRate;    /**< Holds sampling rate of the RealTimeSampleArray.*/
    QString             m_qString_Unit;     /**< Holds unit of the data of the measurement.*/
    double              m_dValue;           /**< Holds the current attached value.*/
    unsigned char       m_ucArraySize;      /**< Holds vector size of the sample array vector.*/
    QVector<double>     m_vecSamples;       /**< Holds the sample array vector.*/
};


//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeSampleArray::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_vecSamples.clear();
}

inline void RealTimeSampleArray::setMinValue(double minValue)
{
    QMutexLocker locker(&m_qMutex);
    m_dMinValue = minValue;
}


//=============================================================================================================

inline double RealTimeSampleArray::getMinValue() const
{
    QMutexLocker locker(&m_qMutex);
    return m_dMinValue;
}


//=============================================================================================================

inline void RealTimeSampleArray::setMaxValue(double maxValue)
{
    QMutexLocker locker(&m_qMutex);
    m_dMaxValue = maxValue;
}


//=============================================================================================================

inline double RealTimeSampleArray::getMaxValue() const
{
    QMutexLocker locker(&m_qMutex);
    return m_dMaxValue;
}


//=============================================================================================================

inline void RealTimeSampleArray::setSamplingRate(double dSamplingRate)
{
    QMutexLocker locker(&m_qMutex);
    m_dSamplingRate = dSamplingRate;
}


//=============================================================================================================

inline double RealTimeSampleArray::getSamplingRate() const
{
    QMutexLocker locker(&m_qMutex);
    return m_dSamplingRate;
}


//=============================================================================================================

inline void RealTimeSampleArray::setArraySize(unsigned char ucArraySize)
{
    QMutexLocker locker(&m_qMutex);
    //Obsolete unsigned char can't be bigger
//    if(ucArraySize > 255)
//        m_ucArraySize = 255;
//    else
        m_ucArraySize = ucArraySize;
}


//=============================================================================================================

unsigned char RealTimeSampleArray::getArraySize() const
{
    QMutexLocker locker(&m_qMutex);
    return m_ucArraySize;
}


//=============================================================================================================

inline const QVector<double>& RealTimeSampleArray::getSampleArray()
{
    QMutexLocker locker(&m_qMutex);
    return m_vecSamples;
}


//=============================================================================================================

inline void RealTimeSampleArray::setUnit(const QString& unit)
{
    QMutexLocker locker(&m_qMutex);
    m_qString_Unit = unit;
}


//=============================================================================================================

inline const QString& RealTimeSampleArray::getUnit() const
{
    QMutexLocker locker(&m_qMutex);
    return m_qString_Unit;
}

} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeSampleArray::SPtr)

#endif // REALTIMESAMPLEARRAY_H
