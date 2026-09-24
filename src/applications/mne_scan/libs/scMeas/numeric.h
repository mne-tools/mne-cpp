//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     numeric.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the Numeric class.
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
