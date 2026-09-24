//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginscenemanager.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains declaration of PluginSceneManager class.
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
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNALIB
{
class MnaGraph;
}

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

    //=========================================================================================================
    /**
     * Returns a reference to the pipeline MnaGraph.
     * The graph mirrors the current plugin/connection state.
     */
    MNALIB::MnaGraph& pipelineGraph();

    /**
     * Returns a const reference to the pipeline MnaGraph.
     */
    const MNALIB::MnaGraph& pipelineGraph() const;

    //=========================================================================================================
    /**
     * Add a graph node for the given plugin.
     * Called automatically by addPlugin().
     */
    void addGraphNode(const AbstractPlugin::SPtr& pPlugin, qreal guiX = 0, qreal guiY = 0);

    //=========================================================================================================
    /**
     * Remove the graph node for the given plugin.
     * Called automatically by removePlugin().
     */
    void removeGraphNode(const AbstractPlugin::SPtr& pPlugin);

    //=========================================================================================================
    /**
     * Record a connection in the pipeline graph.
     */
    void connectGraphNodes(const AbstractPlugin::SPtr& pSender,
                           const AbstractPlugin::SPtr& pReceiver);

    //=========================================================================================================
    /**
     * Update the GUI position attributes for a plugin's graph node.
     */
    void updateGraphNodePosition(const AbstractPlugin::SPtr& pPlugin,
                                 qreal guiX, qreal guiY);

    //=========================================================================================================
    /**
     * Refresh all graph node attributes from plugin settings.
     * Call this before saving to ensure node attributes are up to date.
     */
    void refreshGraphNodeAttributes();

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
    MNALIB::MnaGraph* m_pPipelineGraph;  /**< MNA graph mirroring the plugin scene. */
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
