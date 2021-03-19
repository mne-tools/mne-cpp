//=============================================================================================================
/**
 * @file     plugininputdata.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the PluginInputData class.
 *
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
