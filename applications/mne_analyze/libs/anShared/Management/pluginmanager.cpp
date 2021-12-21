//=============================================================================================================
/**
 * @file     pluginmanager.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 *           Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2017
 *
 * @section  LICENSE
 *
Copyright (C) 2017, Christoph Dinh, Lars Debor, Simon Heinke and Matti Hamalainen. All rights reserved.
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
 * @brief    Definition of the PluginManager class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginmanager.h"
#include "../Plugins/abstractplugin.h"
#include "communicator.h"
#include <algorithm>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginManager::PluginManager(QObject *parent)
: QPluginLoader(parent)
{

}

//=============================================================================================================

PluginManager::~PluginManager()
{
    for(AbstractPlugin*& plugin : m_qVecPlugins)
    {
        delete plugin;
    }
}

//=============================================================================================================

void PluginManager::loadPluginsFromDirectory(const QString& dir)
{
#ifdef STATICBUILD
    Q_UNUSED(dir);

    const auto staticInstances = QPluginLoader::staticInstances();
    for(QObject *plugin : staticInstances) {
        // AbstractPlugin
        if(AbstractPlugin* pPlugin = qobject_cast<AbstractPlugin*>(plugin)) {
            m_qVecPlugins.push_back(qobject_cast<AbstractPlugin*>(pPlugin));
        }
    }
#else
    QDir pluginsDir(dir);
    foreach(const QString &file, pluginsDir.entryList(QDir::NoDotAndDotDot | QDir::Files)) {
        loadPlugin(pluginsDir.absoluteFilePath(file));
    }
#endif
}

//=============================================================================================================

void PluginManager::loadPlugin(const QString& file)
{
    if(!file.contains(".exp") && !file.contains(".lib"))
    {
        this->setFileName(file);
        AbstractPlugin* pPlugin = qobject_cast<AbstractPlugin*>(this->instance());
        if(pPlugin) {
            if(findByName(pPlugin->getName()) == -1)
            {
                qInfo() << "[PluginManager::loadPlugin] Loading Plugin " << file.toUtf8().constData() << " succeeded.";
                qInfo() << "[PluginManager::loadPlugin] Build Info:" << pPlugin->getBuildInfo();
                insertPlugin(pPlugin);
            } else {
                qInfo() << "[PluginManager::loadPlugin] Loading Plugin " << file.toUtf8().constData() << ". Plugin already loaded.";
            }
        } else {
            qInfo() << "[PluginManager::loadPlugin] Loading Plugin " << file.toUtf8().constData() << " failed.";
        }
    }
}

//=============================================================================================================

void PluginManager::insertPlugin(AbstractPlugin* pPlugin)
{
    m_qVecPlugins.push_back(pPlugin);
    std::sort(m_qVecPlugins.begin(),m_qVecPlugins.end(),
              [](AbstractPlugin* a,AbstractPlugin* b) { return a->getOrder() > b->getOrder(); });
}

//=============================================================================================================

void PluginManager::initPlugins(QSharedPointer<AnalyzeData> data)
{
    for(AbstractPlugin* plugin : m_qVecPlugins)
    {
        if(plugin->hasBeenInitialized() == false)
        {
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
    for(int i = 0; it != m_qVecPlugins.cend(); ++i, ++it)
    {
        if((*it)->getName() == name)
        {
            return i;
        }
    }
    return -1;
}

//=============================================================================================================

void PluginManager::shutdown()
{
    for(AbstractPlugin*& plugin : m_qVecPlugins)
    {
        plugin->unload();
    }
}
