//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     plugininputdata.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the PluginInputData class.
 */
#ifndef PLUGININPUTDATA_H
#define PLUGININPUTDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"

#include "plugininputconnector.h"

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

template <class T>
class PluginInputData : public PluginInputConnector
{
public:
    typedef void (*callback_function)(QSharedPointer<T>);       /**< Callback function type. */

    typedef QSharedPointer<PluginInputData> SPtr;               /**< Shared pointer type for PluginInputData. */
    typedef QSharedPointer<const PluginInputData> ConstSPtr;    /**< Const shared pointer type for PluginInputData. */

    //=========================================================================================================
    /**
     * Constructs a PluginInputData with the given parent.
     *
     * @param[in] parent     pointer to parent plugin.
     * @param[in] name       connection name.
     * @param[in] descr      connection description.
     */
    PluginInputData(AbstractPlugin *parent, const QString &name, const QString &descr);

    //=========================================================================================================
    /**
     * Destructor
     */
    virtual ~PluginInputData(){}

    //=========================================================================================================
    /**
     * Creates PluginInputData with the given parent.
     *
     * @param[in] parent     pointer to parent plugin.
     * @param[in] name       connection name.
     * @param[in] descr      connection description.
     *
     * @return the created PluginInputData.
     */
    static inline QSharedPointer< PluginInputData<T> > create(AbstractPlugin *parent, const QString &name, const QString &descr);

    //=========================================================================================================
    /**
     * Convinience function - this can be used to register a function which should be called when new data are available.
     * The signal void notify(SCMEASLIB::Measurement::SPtr) can be used instead of registering a function.
     *
     * @param[in] pFunc  callback function to register.
     */
    void setCallbackMethod(callback_function pFunc);

protected:
    //=========================================================================================================
    /**
     * SLOT to notify the registered calback fucntion.
     *
     * @param[in] pMeasurement   the measurement data to downcast.
     */
    void notifyCallbackFunction(SCMEASLIB::Measurement::SPtr pMeasurement);

private:
    callback_function m_pFunc;  /**< registered callback function. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template <class T>
inline QSharedPointer< PluginInputData<T> > PluginInputData<T>::create(AbstractPlugin *parent, const QString &name, const QString &descr)
{
    QSharedPointer< PluginInputData<T> > pPluginInputData(new PluginInputData<T>(parent, name, descr));
    return pPluginInputData;
}
} // NAMESPACE

//Make the template definition visible to compiler in the first point of instantiation
#include "plugininputdata.cpp"

#endif // PLUGININPUTDATA_H
