//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     measurement.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the Measurement class.
 */

#ifndef MEASUREMENT_H
#define MEASUREMENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QMutex>
#include <QMutexLocker>

//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{

class SCMEASSHARED_EXPORT Measurement : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<Measurement> SPtr;               /**< Shared pointer type for Measurement. */
    typedef QSharedPointer<const Measurement> ConstSPtr;    /**< Const shared pointer type for Measurement. */

    //=========================================================================================================
    /**
     * Constructs a Measurement.
     *
     * @param[in] type       the QMetaType id of the Measurement.
     * @param[in] parent     the parent object.
     */
    explicit Measurement(int type = QMetaType::UnknownType,
                         QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructs the Measurement.
     */
    virtual ~Measurement();

    //=========================================================================================================
    /**
     * Returns the name of the Measurement.
     *
     * @return the name of the Measurement.
     */
    inline const QString& getName() const;

    //=========================================================================================================
    /**
     * Sets the name of the Measurement.
     *
     * @param[in] name which should be set.
     */
    inline void setName(const QString& name);

    //=========================================================================================================
    /**
     * Returns whether Measurement is visible.
     *
     * @return true if Measurement is visible, otherwise false.
     */
    inline bool isVisible() const;

    //=========================================================================================================
    /**
     * Sets the visibility of the Measurement, whether Measurement is visible at the display or just data are send invisible.
     *
     * @param[in] visibility of the Measurement.
     */
    inline void setVisibility(bool visibility);

    //=========================================================================================================
    /**
     * Returns the type of the Measurement.
     *
     * @return the type of the Measurement.
     */
    inline int type() const;

signals:
    void notify();

protected:
    //=========================================================================================================
    /**
     * Sets the type of the Measurement. Use QMetaType::type("the type") to generate the type.
     *
     * @param[in] type   the QMetaType id of the Measurement.
     */
    inline void setType(int type);

private:
    mutable QMutex                      m_qMutex;           /**< Mutex to ensure thread safety. */
    int                                 m_iMetaTypeId;      /**< QMetaType id of the Measurement. */
    QString                             m_qString_Name;     /**< Name of the Measurement. */
    bool                                m_bVisibility;      /**< Visibility status. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QString& Measurement::getName() const
{
    QMutexLocker locker(&m_qMutex);
    return m_qString_Name;
}

//=============================================================================================================

inline void Measurement::setType(int type)
{
    QMutexLocker locker(&m_qMutex);
    m_iMetaTypeId = type;
}

//=============================================================================================================

inline void Measurement::setName(const QString& name)
{
    QMutexLocker locker(&m_qMutex);
    m_qString_Name = name;
}

//=============================================================================================================

inline bool Measurement::isVisible() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bVisibility;
}

//=============================================================================================================

inline void Measurement::setVisibility(bool visibility)
{
    QMutexLocker locker(&m_qMutex);
    m_bVisibility = visibility;
}

//=============================================================================================================

inline int Measurement::type() const
{
    QMutexLocker locker(&m_qMutex);
    return m_iMetaTypeId;
}

} //NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::Measurement::SPtr)

#endif // MEASUREMENT_H
