//=============================================================================================================
/**
 * @file     pluginscenemanager.h
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
 * @brief    Contains declaration of PluginSceneManager class.
 *
 */

#ifndef PLUGINSCENEMANAGER_H
#define PLUGINSCENEMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"
#include "../Plugins/abstractplugin.h"
#include "pluginconnectorconnection.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=========================================================================================================
/**
 * PluginSceneManager manages plugins and connections between connectors.
 *
 * @brief The PluginSceneManager class manages plugins and connections of a set of plugins.
 */
class SCSHAREDSHARED_EXPORT PluginSceneManager : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<PluginSceneManager> SPtr;            /**< Shared pointer type for PluginSceneManager. */
    typedef QSharedPointer<const PluginSceneManager> ConstSPtr; /**< Const shared pointer type for PluginSceneManager. */

    typedef QList< AbstractPlugin::SPtr > PluginList;                                      /**< type for a list of plugins. */
    typedef QList<PluginConnectorConnection::SPtr> PluginConnectorConnectionList;   /**< Shared pointer type for PluginConnectorConnection::SPtr list. */

    //=========================================================================================================
    /**
     * Constructs a PluginSceneManager.
     */
    explicit PluginSceneManager(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destructs a PluginSceneManager.
     */
    ~PluginSceneManager();

    //=========================================================================================================
    /**
     * Adds a plugin to the stage.
     *
     * @param[in] pPlugin        plugin to be cloned and added.
     * @param[out] pAddedPlugin  if plugin is successful added, this contains a pointer to the added instance.
     *
     *@return true if plugin is added successful.
     */
    bool addPlugin(const AbstractPlugin* pPlugin, AbstractPlugin::SPtr &pAddedPlugin);

    inline PluginList& getPlugins();

    //=========================================================================================================
    /**
     * Removes a plugin from the stage.
     *
     * @param[in] pPlugin    plugin to be removed.
     *
     *@return true if plugin is removed successful.
     */
    bool removePlugin(const AbstractPlugin::SPtr pPlugin);

    //=========================================================================================================
    /**
     * Starts all plugins.
     *
     * @return true if at least one AbstractSensor plugin was started successfully, false otherwise.
     */
    bool startPlugins();

    //=========================================================================================================
    /**
     * Starts AbstractSensor Plugins
     *
     * @return true if at least one AbstractSensor plugin was started successfully, false otherwise.
     */
    bool startSensorPlugins();

    //=========================================================================================================
    /**
     * Starts AbstractAlgorithm plugins.
     *
     * @return true if at least one AbstractSensor plugin was started successfully, false otherwise.
     */
    bool startAlgorithmPlugins();

    //=========================================================================================================
    /**
     * Stops all plugins.
     */
    void stopPlugins();

    //=========================================================================================================
    /**
     * Clears the PluginStage.
     */
    void clear();

signals:

private:

    //=========================================================================================================
    /**
     * Stops all sensor plugins in m_pluginList.
     */
    void stopSensorPlugins();

    //=========================================================================================================
    /**
     * Stops all non-sensor plugins in m_pluginList.
     */
    void stopNonSensorPlugins();

    PluginList m_pluginList;    /**< List of plugins associated with this set. */
//    PluginConnectorConnectionList m_conConList; /**< List of connector connections. */

//    QSharedPointer<PluginSet> m_pPluginSet;     /**< The Plugin set of the stage -> ToDo: check, if more than one set on the stage is usefull. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline PluginSceneManager::PluginList& PluginSceneManager::getPlugins()
{
    return m_pluginList;
}
} //Namespace

#endif // PLUGINSCENEMANAGER_H
