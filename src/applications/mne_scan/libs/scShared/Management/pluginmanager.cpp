//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginmanager.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the PluginManager class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include "pluginmanager.h"

#include "../Plugins/abstractplugin.h"
#include "../Plugins/abstractsensor.h"
#include "../Plugins/abstractalgorithm.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scan;

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
}

//=============================================================================================================

void PluginManager::loadPlugins(const QString& dir)
{
#ifdef STATICBUILD
    Q_UNUSED(dir);

    const auto staticInstances = QPluginLoader::staticInstances();
    for (QObject* plugin : staticInstances) {
        // AbstractPlugin
        if (plugin) {
            if (AbstractPlugin* pPlugin = qobject_cast<AbstractPlugin*>(plugin)) {
                // plugins are always disabled when they are first loaded
                m_qVecPlugins.push_back(pPlugin);

                AbstractPlugin::PluginType pluginType = pPlugin->getType();
                QString msg = "Plugin " + pPlugin->getName() + " loaded.";

                // AbstractSensor
                if (pluginType == AbstractPlugin::_ISensor) {
                    if (AbstractSensor* pSensor = qobject_cast<AbstractSensor*>(pPlugin)) {
                        m_qVecSensorPlugins.push_back(pSensor);
                        mne::Log::debug("Sensor {} loaded.", pSensor->getName());
                    }
                }
                // AbstractAlgorithm
                else if (pluginType == AbstractPlugin::_IAlgorithm) {
                    if (AbstractAlgorithm* pAlgorithm = qobject_cast<AbstractAlgorithm*>(pPlugin)) {
                        m_qVecAlgorithmPlugins.push_back(pAlgorithm);
                        mne::Log::debug("RTAlgorithm {} loaded.", pAlgorithm->getName());
                    }
                }

                emit pluginLoaded(msg);
            }
        } else {
            mne::Log::debug("Plugin could not be instantiated!");
        }
    }
#else
    QDir PluginsDir(dir);

    foreach (QString file, PluginsDir.entryList(QDir::Files)) {
        // Exclude .exp and .lib files (only relevant for windows builds)
        if (!file.contains(".exp") && !file.contains(".lib")) {
            this->setFileName(PluginsDir.absoluteFilePath(file));
            mne::Log::debug("{}", PluginsDir.absoluteFilePath(file).toStdString());
            QObject* pPlugin = this->instance();
            if (!pPlugin) {
                mne::Log::warning("[PluginManager::loadPlugins] Cannot load {} - {}", file.toStdString(), errorString().toStdString());
            }

            // AbstractPlugin
            if (pPlugin) {
                // plugins are always disabled when they are first loaded
                m_qVecPlugins.push_back(qobject_cast<AbstractPlugin*>(pPlugin));

                AbstractPlugin::PluginType pluginType = qobject_cast<AbstractPlugin*>(pPlugin)->getType();
                QString msg = "Plugin " + qobject_cast<AbstractPlugin*>(pPlugin)->getName() + " loaded.";

                if (pluginType == AbstractPlugin::_ISensor) {
                    // AbstractSensor
                    AbstractSensor* pSensor = qobject_cast<AbstractSensor*>(pPlugin);

                    if (pSensor) {
                        m_qVecSensorPlugins.push_back(pSensor);
                        mne::Log::info("[PluginManager::loadPlugins] Loading sensor plugin {} succeeded.", pSensor->getName().toStdString());
                        mne::Log::info("[PluginManager::loadPlugins] Build Info: {}", pSensor->getBuildInfo().toStdString());

                    } else {
                        mne::Log::info("[PluginManager::loadPlugins] Loading sensor plugin failed.");
                    }
                } else if (pluginType == AbstractPlugin::_IAlgorithm) {
                    // AbstractAlgorithm
                    AbstractAlgorithm* pAlgorithm = qobject_cast<AbstractAlgorithm*>(pPlugin);

                    if (pAlgorithm) {
                        m_qVecAlgorithmPlugins.push_back(pAlgorithm);
                        mne::Log::info("[PluginManager::loadPlugins] Loading algorithm plugin {} succeeded.", pAlgorithm->getName().toStdString());
                        mne::Log::info("[PluginManager::loadPlugins] Build Timestamp: {}", pAlgorithm->getBuildInfo().toStdString());
                    } else {
                        mne::Log::info("[PluginManager::loadPlugins] Loading algorithm plugin failed.");
                    }
                }

                emit pluginLoaded(msg);
            }
        }
    }
#endif
}

//=============================================================================================================

int PluginManager::findByName(const QString& name)
{
    QVector<AbstractPlugin*>::const_iterator it = m_qVecPlugins.begin();
    for (int i = 0; it != m_qVecPlugins.end(); ++i, ++it)
        if ((*it)->getName() == name)
            return i;

    return -1;
}
