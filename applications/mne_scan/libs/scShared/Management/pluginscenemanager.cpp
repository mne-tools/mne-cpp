//=============================================================================================================
/**
 * @file     pluginscenemanager.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
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
 * @brief    Definition of the PluginSceneManager class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginscenemanager.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginSceneManager::PluginSceneManager(QObject *parent)
: QObject(parent)
{
}

//=============================================================================================================

PluginSceneManager::~PluginSceneManager()
{
    clear();
}

//=============================================================================================================

bool PluginSceneManager::addPlugin(const AbstractPlugin* pPlugin, AbstractPlugin::SPtr &pAddedPlugin)
{
    if(pPlugin->multiInstanceAllowed())
    {
        pAddedPlugin = pPlugin->clone();
        m_pluginList.append(pAddedPlugin);
        m_pluginList.last()->init();
        return true;
    }
    else
    {
        //multi instance not allowed -> check if already added
        QString sPluginName = pPlugin->getName();
        bool bPluginFound = false;

        for(qint32 i = 0; i < m_pluginList.size(); ++i)
        {
            if(sPluginName == m_pluginList[i]->getName())
            {
                bPluginFound = true;
                break;
            }
        }

        //Not added jet
        if(!bPluginFound)
        {
            pAddedPlugin = pPlugin->clone();
            m_pluginList.append(pAddedPlugin);
            m_pluginList.last()->init();
            return true;
        }
    }
    pAddedPlugin.clear();
    return false;
}

//=============================================================================================================

bool PluginSceneManager::removePlugin(const AbstractPlugin::SPtr pPlugin)
{
    qint32 pos = -1;
    for(qint32 i = 0; i < m_pluginList.size(); ++i)
    {
        if(m_pluginList[i] == pPlugin)
        {
            pos = i;
            break;
        }
    }
    if(pos != -1)
    {
        m_pluginList.removeAt(pos);
        return true;
    }
    else
        return false;
}

//=============================================================================================================

bool PluginSceneManager::startPlugins()
{
    // Start AbstractSensor and IRTAlgorithm plugins first!
    bool bFlag = startSensorPlugins();

    if(bFlag) {
        bFlag = startAlgorithmPlugins();
    }

    return bFlag;
}

//=============================================================================================================

bool PluginSceneManager::startSensorPlugins()
{
    bool bFlag = false;

    QList<AbstractPlugin::SPtr>::iterator it = m_pluginList.begin();
    for( ; it != m_pluginList.end(); ++it)
    {
        if((*it)->getType() == AbstractPlugin::_ISensor)
        {
            if(!(*it)->start())
                qWarning() << "Could not start AbstractSensor: " << (*it)->getName();
            else
                bFlag = true; //At least one sensor has to be started
        }
    }

    return bFlag;
}

//=============================================================================================================

bool PluginSceneManager::startAlgorithmPlugins()
{
    bool bFlag = true;

    QList<AbstractPlugin::SPtr>::iterator it = m_pluginList.begin();
    for( ; it != m_pluginList.end(); ++it) {
        if((*it)->getType() == AbstractPlugin::_IAlgorithm) {
            if(!(*it)->start()) {
                bFlag = false;
                qWarning() << "Could not start AbstractAlgorithm: " << (*it)->getName();
            }
        }
    }

    return bFlag;
}

//=============================================================================================================

void PluginSceneManager::stopPlugins()
{
    // Stop AbstractSensor plugins first!
    QList<AbstractPlugin::SPtr>::iterator it = m_pluginList.begin();
    for( ; it != m_pluginList.end(); ++it)
        if((*it)->getType() == AbstractPlugin::_ISensor){
            if(!(*it)->stop()){
                qWarning() << "Could not stop AbstractPlugin: " << (*it)->getName();
            }
        }

    // Stop all other plugins!
    it = m_pluginList.begin();
    for( ; it != m_pluginList.end(); ++it)
        if((*it)->getType() != AbstractPlugin::_ISensor){
            if(!(*it)->stop()){
                qWarning() << "Could not stop AbstractPlugin: " << (*it)->getName();
            }
        }
}

//=============================================================================================================

void PluginSceneManager::clear()
{
//    m_pluginList.clear();
}
