//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginconnector.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the PluginConnector class.
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
