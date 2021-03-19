//=============================================================================================================
/**
 * @file     numeric.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
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
 * @brief    Contains the declaration of the Numeric class.
 *
 */

#ifndef NEWNUMERIC_H
#define NEWNUMERIC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QMutex>
#include <QMutexLocker>

//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{

//=============================================================================================================
/**
 * The Numeric class provides a Numeric Measurement.
 *
 * @brief The Numeric class provides a Numeric Measurement.
 */
class SCMEASSHARED_EXPORT Numeric : public Measurement
{
    Q_OBJECT

public:
    typedef QSharedPointer<Numeric> SPtr;               /**< Shared pointer type for Numeric. */
    typedef QSharedPointer<const Numeric> ConstSPtr;    /**< Const shared pointer type for Numeric. */

    //=========================================================================================================
    /**
     * Constructs a Numeric.
     */
    Numeric(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the Numeric.
     */
    virtual ~Numeric();

    //=========================================================================================================
    /**
     * Sets the unit of the numeric data.
     *
     * @param[in] unit of the data.
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
     * @param[in] v the value which is set to the Numeric measurement.
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
    mutable QMutex  m_qMutex;   /**< Mutex to ensure thread safety. */

    QString m_qString_Unit;     /**< Holds unit of the data of the measurement.*/
    double  m_dValue;           /**< Holds current set value.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void Numeric::setUnit(const QString& unit)
{
    QMutexLocker locker(&m_qMutex);
    m_qString_Unit = unit;
}

//=============================================================================================================

inline const QString& Numeric::getUnit() const
{
    QMutexLocker locker(&m_qMutex);
    return m_qString_Unit;
}
} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::Numeric::SPtr)

#endif // NUMERIC_H
