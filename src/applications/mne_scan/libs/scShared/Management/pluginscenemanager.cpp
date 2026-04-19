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

#include <mna/mna_graph.h>
#include <mna/mna_node.h>
#include <mna/mna_port.h>
#include <mna/mna_types.h>
#include "mna_scan_types.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginSceneManager::PluginSceneManager(QObject *parent)
: QObject(parent)
, m_pPipelineGraph(new MNALIB::MnaGraph())
{
}

//=============================================================================================================

PluginSceneManager::~PluginSceneManager()
{
    clear();
    delete m_pPipelineGraph;
}

//=============================================================================================================

bool PluginSceneManager::addPlugin(const AbstractPlugin* pPlugin, AbstractPlugin::SPtr &pAddedPlugin)
{
    if(pPlugin->multiInstanceAllowed())
    {
        pAddedPlugin = pPlugin->clone();
        m_pluginList.append(pAddedPlugin);
        m_pluginList.last()->init();
        addGraphNode(pAddedPlugin);
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
            addGraphNode(pAddedPlugin);
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
        removeGraphNode(pPlugin);
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
    stopSensorPlugins();
    stopNonSensorPlugins();
}

//=============================================================================================================

void PluginSceneManager::clear()
{
//    m_pluginList.clear();
}

//=============================================================================================================

void PluginSceneManager::stopSensorPlugins()
{
    for(auto& plugin : m_pluginList){
        if(plugin->getType() == AbstractPlugin::_ISensor){
            if(!plugin->stop()){
                qWarning() << "Could not stop AbstractPlugin: " << plugin->getName();
            }
        }
    }
}

//=============================================================================================================

void PluginSceneManager::stopNonSensorPlugins()
{
    for(auto& plugin : m_pluginList){
        if(plugin->getType() != AbstractPlugin::_ISensor){
            if(!plugin->stop()){
                qWarning() << "Could not stop AbstractPlugin: " << plugin->getName();
            }
        }
    }
}

//=============================================================================================================
// MnaGraph management
//=============================================================================================================

MNALIB::MnaGraph& PluginSceneManager::pipelineGraph()
{
    return *m_pPipelineGraph;
}

//=============================================================================================================

const MNALIB::MnaGraph& PluginSceneManager::pipelineGraph() const
{
    return *m_pPipelineGraph;
}

//=============================================================================================================

void PluginSceneManager::addGraphNode(const AbstractPlugin::SPtr& pPlugin, qreal guiX, qreal guiY)
{
    MNALIB::MnaNode node;
    node.id      = pPlugin->getName();
    node.opType  = pPlugin->getName();
    node.execMode = MNALIB::MnaNodeExecMode::Stream;
    node.dirty   = true;

    node.attributes.insert(QStringLiteral("gui_x"), guiX);
    node.attributes.insert(QStringLiteral("gui_y"), guiY);

    // Build output ports from plugin output connectors
    for (int i = 0; i < pPlugin->getOutputConnectors().size(); ++i) {
        MNALIB::MnaPort outPort;
        outPort.name      = pPlugin->getOutputConnectors()[i]->getName();
        outPort.direction = MNALIB::MnaPortDir::Output;
        outPort.dataKind  = connectorDataTypeToMnaDataKind(
            PluginConnectorConnection::getDataType(pPlugin->getOutputConnectors()[i]));
        node.outputs.append(outPort);
    }

    // Build input ports from plugin input connectors
    for (int i = 0; i < pPlugin->getInputConnectors().size(); ++i) {
        MNALIB::MnaPort inPort;
        inPort.name      = pPlugin->getInputConnectors()[i]->getName();
        inPort.direction = MNALIB::MnaPortDir::Input;
        inPort.dataKind  = connectorDataTypeToMnaDataKind(
            PluginConnectorConnection::getDataType(pPlugin->getInputConnectors()[i]));
        node.inputs.append(inPort);
    }

    m_pPipelineGraph->addNode(node);
}

//=============================================================================================================

void PluginSceneManager::removeGraphNode(const AbstractPlugin::SPtr& pPlugin)
{
    m_pPipelineGraph->removeNode(pPlugin->getName());
}

//=============================================================================================================

void PluginSceneManager::connectGraphNodes(const AbstractPlugin::SPtr& pSender,
                                           const AbstractPlugin::SPtr& pReceiver)
{
    // Find matching output→input ports by data kind
    const MNALIB::MnaNode& srcNode = m_pPipelineGraph->node(pSender->getName());
    MNALIB::MnaNode& dstNode = m_pPipelineGraph->node(pReceiver->getName());

    for (const MNALIB::MnaPort& outPort : srcNode.outputs) {
        for (int i = 0; i < dstNode.inputs.size(); ++i) {
            if (dstNode.inputs[i].dataKind == outPort.dataKind
                && dstNode.inputs[i].sourceNodeId.isEmpty()) {
                m_pPipelineGraph->connect(pSender->getName(), outPort.name,
                                          pReceiver->getName(), dstNode.inputs[i].name);
                return;
            }
        }
    }
}

//=============================================================================================================

void PluginSceneManager::updateGraphNodePosition(const AbstractPlugin::SPtr& pPlugin,
                                                  qreal guiX, qreal guiY)
{
    if (m_pPipelineGraph->hasNode(pPlugin->getName())) {
        MNALIB::MnaNode& n = m_pPipelineGraph->node(pPlugin->getName());
        n.attributes.insert(QStringLiteral("gui_x"), guiX);
        n.attributes.insert(QStringLiteral("gui_y"), guiY);
    }
}
