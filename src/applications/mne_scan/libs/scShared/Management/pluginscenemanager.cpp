//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginscenemanager.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the PluginSceneManager class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include "pluginscenemanager.h"

#include <mna/mna_graph.h>
#include <mna/mna_node.h>
#include <mna/mna_port.h>
#include <mna/mna_types.h>
#include "mna_scan_types.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scan;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginSceneManager::PluginSceneManager(QObject* parent)
: QObject(parent)
, m_pPipelineGraph(std::make_unique<mne::MnaGraph>())
{
}

//=============================================================================================================

PluginSceneManager::~PluginSceneManager()
{
    clear();
}

//=============================================================================================================

bool PluginSceneManager::addPlugin(const AbstractPlugin* pPlugin, AbstractPlugin::SPtr& pAddedPlugin)
{
    if (pPlugin->multiInstanceAllowed()) {
        pAddedPlugin = pPlugin->clone();
        m_pluginList.append(pAddedPlugin);
        m_pluginList.last()->init();
        addGraphNode(pAddedPlugin);
        return true;
    } else {
        //multi instance not allowed -> check if already added
        QString sPluginName = pPlugin->getName();
        bool bPluginFound = false;

        for (qint32 i = 0; i < m_pluginList.size(); ++i) {
            if (sPluginName == m_pluginList[i]->getName()) {
                bPluginFound = true;
                break;
            }
        }

        //Not added jet
        if (!bPluginFound) {
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
    for (qint32 i = 0; i < m_pluginList.size(); ++i) {
        if (m_pluginList[i] == pPlugin) {
            pos = i;
            break;
        }
    }
    if (pos != -1) {
        removeGraphNode(pPlugin);
        m_pluginList.removeAt(pos);
        return true;
    } else
        return false;
}

//=============================================================================================================

bool PluginSceneManager::startPlugins()
{
    // Start AbstractSensor and IRTAlgorithm plugins first!
    bool bFlag = startSensorPlugins();

    if (bFlag) {
        bFlag = startAlgorithmPlugins();
    }

    return bFlag;
}

//=============================================================================================================

bool PluginSceneManager::startSensorPlugins()
{
    bool bFlag = false;

    QList<AbstractPlugin::SPtr>::iterator it = m_pluginList.begin();
    for (; it != m_pluginList.end(); ++it) {
        if ((*it)->getType() == AbstractPlugin::_ISensor) {
            if (!(*it)->start())
                mne::Log::warning("Could not start AbstractSensor: {}", ((*it)->getName()).toStdString());
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
    for (; it != m_pluginList.end(); ++it) {
        if ((*it)->getType() == AbstractPlugin::_IAlgorithm) {
            if (!(*it)->start()) {
                bFlag = false;
                mne::Log::warning("Could not start AbstractAlgorithm: {}", ((*it)->getName()).toStdString());
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
    for (auto& plugin : m_pluginList) {
        if (plugin->getType() == AbstractPlugin::_ISensor) {
            if (!plugin->stop()) {
                mne::Log::warning("Could not stop AbstractPlugin: {}", plugin->getName().toStdString());
            }
        }
    }
}

//=============================================================================================================

void PluginSceneManager::stopNonSensorPlugins()
{
    for (auto& plugin : m_pluginList) {
        if (plugin->getType() != AbstractPlugin::_ISensor) {
            if (!plugin->stop()) {
                mne::Log::warning("Could not stop AbstractPlugin: {}", plugin->getName().toStdString());
            }
        }
    }
}

//=============================================================================================================
// MnaGraph management
//=============================================================================================================

mne::MnaGraph& PluginSceneManager::pipelineGraph()
{
    return *m_pPipelineGraph;
}

//=============================================================================================================

const mne::MnaGraph& PluginSceneManager::pipelineGraph() const
{
    return *m_pPipelineGraph;
}

//=============================================================================================================

void PluginSceneManager::addGraphNode(const AbstractPlugin::SPtr& pPlugin, qreal guiX, qreal guiY)
{
    mne::MnaNode node;
    node.id = pPlugin->getName().toStdString();
    node.op_type = node.id;
    node.exec_mode = mne::MnaNodeExecMode::Stream;
    node.dirty = true;

    node.attributes = toNodeAttributes(pPlugin->getAttributes());
    node.attributes["gui_x"] = guiX;
    node.attributes["gui_y"] = guiY;

    // Build output ports from plugin output connectors
    for (int i = 0; i < pPlugin->getOutputConnectors().size(); ++i) {
        mne::MnaPort outPort;
        outPort.name = pPlugin->getOutputConnectors()[i]->getName().toStdString();
        outPort.direction = mne::MnaPortDir::Output;
        outPort.data_kind = connectorDataTypeToMnaDataKind(
            PluginConnectorConnection::getDataType(pPlugin->getOutputConnectors()[i]));
        node.outputs.push_back(outPort);
    }

    // Build input ports from plugin input connectors
    for (int i = 0; i < pPlugin->getInputConnectors().size(); ++i) {
        mne::MnaPort inPort;
        inPort.name = pPlugin->getInputConnectors()[i]->getName().toStdString();
        inPort.direction = mne::MnaPortDir::Input;
        inPort.data_kind = connectorDataTypeToMnaDataKind(
            PluginConnectorConnection::getDataType(pPlugin->getInputConnectors()[i]));
        node.inputs.push_back(inPort);
    }

    m_pPipelineGraph->add_node(std::move(node));
}

//=============================================================================================================

void PluginSceneManager::removeGraphNode(const AbstractPlugin::SPtr& pPlugin)
{
    m_pPipelineGraph->remove_node(pPlugin->getName().toStdString());
}

//=============================================================================================================

void PluginSceneManager::connectGraphNodes(const AbstractPlugin::SPtr& pSender,
                                           const AbstractPlugin::SPtr& pReceiver)
{
    // Find matching output→input ports by data kind
    const std::string sender = pSender->getName().toStdString();
    const std::string receiver = pReceiver->getName().toStdString();
    const mne::MnaNode* srcNode = m_pPipelineGraph->find_node(sender);
    const mne::MnaNode* dstNode = m_pPipelineGraph->find_node(receiver);
    if (!srcNode || !dstNode) {
        return;
    }
    for (const mne::MnaPort& outPort : srcNode->outputs) {
        for (const mne::MnaPort& inPort : dstNode->inputs) {
            if (inPort.data_kind == outPort.data_kind && inPort.source_node_id.empty()) {
                m_pPipelineGraph->connect(sender, outPort.name, receiver, inPort.name);
                return;
            }
        }
    }
}

//=============================================================================================================

void PluginSceneManager::updateGraphNodePosition(const AbstractPlugin::SPtr& pPlugin,
                                                 qreal guiX, qreal guiY)
{
    if (mne::MnaNode* n = m_pPipelineGraph->find_node(pPlugin->getName().toStdString())) {
        n->attributes["gui_x"] = guiX;
        n->attributes["gui_y"] = guiY;
    }
}

//=============================================================================================================

void PluginSceneManager::refreshGraphNodeAttributes()
{
    for (const auto& pPlugin : m_pluginList) {
        mne::MnaNode* node = m_pPipelineGraph->find_node(pPlugin->getName().toStdString());
        if (!node)
            continue;
        for (const auto& [key, value] : toNodeAttributes(pPlugin->getAttributes()))
            node->attributes.insert(key, value);
    }
}
