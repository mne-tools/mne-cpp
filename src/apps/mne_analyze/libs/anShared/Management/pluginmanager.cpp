//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     pluginmanager.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2017
 * @brief    Definition of the PluginManager class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "pluginmanager.h"

#include "../Plugins/abstractplugin.h"
#include "communicator.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::analyze;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginManager::PluginManager(QObject* parent)
: QPluginLoader(parent)
{
}

//=============================================================================================================

PluginManager::~PluginManager()
{
    // The plugin instances are children of this manager and are destroyed with it
}

//=============================================================================================================

void PluginManager::loadPluginsFromDirectory(const QString& dir)
{
#ifdef STATICBUILD
    Q_UNUSED(dir);

    const auto staticInstances = QPluginLoader::staticInstances();
    for (QObject* plugin : staticInstances) {
        // AbstractPlugin
        if (AbstractPlugin* pPlugin = qobject_cast<AbstractPlugin*>(plugin)) {
            // The loader does not delete its root instances; the manager takes them over
            pPlugin->setParent(this);
            m_qVecPlugins.push_back(qobject_cast<AbstractPlugin*>(pPlugin));
        }
    }
#else
    QDir pluginsDir(dir);
    foreach (const QString& file, pluginsDir.entryList(QDir::NoDotAndDotDot | QDir::Files)) {
        emit pluginLoaded("Loading " + file);
        loadPlugin(pluginsDir.absoluteFilePath(file));
    }
#endif
}

//=============================================================================================================

void PluginManager::loadPlugin(const QString& file)
{
    if (!file.contains(".exp") && !file.contains(".lib")) {
        this->setFileName(file);
        AbstractPlugin* pPlugin = qobject_cast<AbstractPlugin*>(this->instance());
        if (pPlugin) {
            if (findByName(pPlugin->getName()) == -1) {
                mne::Log::info("[PluginManager::loadPlugin] Loading Plugin {} succeeded.", file.toStdString());
                mne::Log::info("[PluginManager::loadPlugin] Build Info: {}", pPlugin->getBuildInfo().toStdString());
                insertPlugin(pPlugin);
            } else {
                mne::Log::info("[PluginManager::loadPlugin] Loading Plugin {} . Plugin already loaded.", file.toStdString());
            }
        } else {
            mne::Log::info("[PluginManager::loadPlugin] Loading Plugin {} failed.", file.toStdString());
        }
    }
}

//=============================================================================================================

void PluginManager::insertPlugin(AbstractPlugin* pPlugin)
{
    // The loader does not delete its root instances; the manager takes them over
    pPlugin->setParent(this);
    m_qVecPlugins.push_back(pPlugin);
    std::sort(m_qVecPlugins.begin(), m_qVecPlugins.end(),
              [](AbstractPlugin* a, AbstractPlugin* b) { return a->getOrder() > b->getOrder(); });
}

//=============================================================================================================

void PluginManager::initPlugins(QSharedPointer<AnalyzeData> data)
{
    for (AbstractPlugin* plugin : m_qVecPlugins) {
        emit pluginLoaded("Initializing " + plugin->getName());
        if (plugin->hasBeenInitialized() == false) {
            plugin->setGlobalData(data);
            plugin->init();
            plugin->setInitState(true);
        }
    }

    // tell everyone that INIT-phase is finished
    Communicator con;
    con.publishEvent(EVENT_TYPE::PLUGIN_INIT_FINISHED);
}

//=============================================================================================================

int PluginManager::findByName(const QString& name)
{
    QVector<AbstractPlugin*>::const_iterator it = m_qVecPlugins.cbegin();
    for (int i = 0; it != m_qVecPlugins.cend(); ++i, ++it) {
        if ((*it)->getName() == name) {
            return i;
        }
    }
    return -1;
}

//=============================================================================================================

void PluginManager::shutdown()
{
    for (AbstractPlugin*& plugin : m_qVecPlugins) {
        plugin->unload();
    }
}
