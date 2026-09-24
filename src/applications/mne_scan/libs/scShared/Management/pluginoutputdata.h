//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginoutputdata.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the PluginOutputData class.
 */
#ifndef PLUGINOUTPUTDATA_H
#define PLUGINOUTPUTDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"
#include "pluginoutputconnector.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QMetaType>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=========================================================================================================
/**
 * Class PluginOutputData provides an output connector with a specified MEasurement type.
 *
 * @brief PluginOutputConnector with specified Measurement
 */
template <class T>
class PluginOutputData : public PluginOutputConnector
{
public:
    typedef QSharedPointer<PluginOutputData<T> > SPtr;               /**< Shared pointer type for PluginOutputData. */
    typedef QSharedPointer<const PluginOutputData<T> > ConstSPtr;    /**< Const shared pointer type for PluginOutputData. */

    //=========================================================================================================
    /**
     * Constructs PluginOutputData with the given parent.
     *
     * @param[in] parent     pointer to parent plugin.
     * @param[in] name       connection name.
     * @param[in] descr      connection description.
     */
    PluginOutputData(AbstractPlugin *parent,
                     const QString &name,
                     const QString &descr);

    //=========================================================================================================
    /**
     * Destructor
     */
    virtual ~PluginOutputData(){}

    //=========================================================================================================
    /**
     * Creates PluginOutputData with the given parent.
     *
     * @param[in] parent     pointer to parent plugin.
     * @param[in] name       connection name.
     * @param[in] descr      connection description.
     *
     * @return the created PluginOutputData.
     */
    static inline QSharedPointer< PluginOutputData<T> > create(AbstractPlugin *parent,
                                                               const QString &name,
                                                               const QString &descr);

    //=========================================================================================================
    /**
     * Returns the measurement
     *
     * @return the measurement.
     */
    inline QSharedPointer<T> measurementData();

    void update();

private:
    QSharedPointer<T> m_pMeasurement;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template <class T>
inline QSharedPointer< PluginOutputData<T> > PluginOutputData<T>::create(AbstractPlugin *parent, const QString &name, const QString &descr)
{
    QSharedPointer< PluginOutputData<T> > pPluginOutputData(new PluginOutputData<T>(parent, name, descr));
    return pPluginOutputData;
}

//=============================================================================================================

template <class T>
inline QSharedPointer<T> PluginOutputData<T>::measurementData()
{
    return m_pMeasurement;
}
} // NAMESPACE

//Make the template definition visible to compiler in the first point of instantiation
#include "pluginoutputdata.cpp"

#endif // PLUGININPUTDATA_H
