//=============================================================================================================
/**
 * @file     pluginconnector.h
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
 * @brief    Contains the declaration of the PluginConnector class.
 *
 */
#ifndef PLUGINCONNECTOR_H
#define PLUGINCONNECTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QString>
#include <QMutex>
#include <QSet>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class AbstractPlugin;

//=============================================================================================================
/**
 * Class implements plug-in data connections.
 *
 * @brief The PluginConnector class provides the base to connect plug-in data
 */
class SCSHAREDSHARED_EXPORT PluginConnector : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<PluginConnector> SPtr;               /**< Shared pointer type for PluginConnector. */
    typedef QSharedPointer<const PluginConnector> ConstSPtr;    /**< Const shared pointer type for PluginConnector. */

    //=========================================================================================================
    /**
     * Constructs a PluginConnector with the given parent.
     *
     * @param[in] parent     pointer to parent plugin.
     * @param[in] name       connection name.
     * @param[in] descr      connection description.
     */
    PluginConnector(AbstractPlugin *parent, const QString &name, const QString &descr);
    
    //=========================================================================================================
    /**
     * Destructor
     */
    virtual ~PluginConnector(){}

    //=========================================================================================================
    /**
     * Returns true if this instance is an PluginInputConnector.
     *
     * @return true if castable to PluginInputConnector.
     */
    virtual bool isInputConnector() const = 0;

    //=========================================================================================================
    /**
     * Returns true if this instance is an PluginOutputConnector.
     *
     * @return true if castable to PluginOutputConnector.
     */
    virtual bool isOutputConnector() const = 0;

    //=========================================================================================================
    /**
     * Returns the PluginConnectors name.
     *
     * @return the PluginConnectors name.
     */
    inline QString getName() const;

signals:

protected:
    AbstractPlugin* m_pPlugin;  /**< Plugin to which connector belongs to. */

    //actual obeserver pattern - think of an other implementation --> currently similiar to OpenWalnut
    //figure out how to Qt signal/slot
    QSet<PluginConnector::SPtr> m_setConnections; /**< Set of connectors connected to this connector. */

private:
    QString m_sName;        /**< Connection name. */
    QString m_sDescription; /**< Connection description. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

QString PluginConnector::getName() const
{
    return m_sName;
}
} // NAMESPACE

#endif // PLUGINCONNECTOR_H
