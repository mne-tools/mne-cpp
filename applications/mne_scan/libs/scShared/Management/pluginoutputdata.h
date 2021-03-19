//=============================================================================================================
/**
 * @file     pluginoutputdata.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the PluginOutputData class.
 *
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
