//=============================================================================================================
/**
 * @file     deepviewer.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    DeepViewer class implementation.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deepviewer.h"
#include "view.h"
#include "controls.h"
#include "network.h"

#include "node.h"
#include "edge.h"

#include <deep/deep.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QHBoxLayout>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DEEPLIB;
using namespace DISPLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DeepViewer::DeepViewer(bool embeddedControl, QWidget *parent)
: QWidget(parent)
, m_pView(new View)
, m_pNetwork(new Network)
{
    initScene();

    m_pView->getGraphicsView()->setScene(m_pScene);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_pView);

    if(embeddedControl) {
        Controls *controls = new Controls(this, this);
        layout->addWidget(controls);
    }

    setLayout(layout);

    setWindowTitle(tr("Deep Model Viewer"));
}


//*************************************************************************************************************

View *DeepViewer::getView() const
{
    return m_pView;
}


//*************************************************************************************************************

Network* DeepViewer::getNetwork() const
{
    return m_pNetwork;
}


//*************************************************************************************************************

void DeepViewer::setModel(const Deep::SPtr& model)
{
    if(model != m_pNetwork->model()) {
        removeSceneItems();
        m_pNetwork->setModel(model);
        updateSceneItems();
    }
    else {
        updateModel();
    }
}


//*************************************************************************************************************

void DeepViewer::updateModel()
{
    qDebug() << "updateModel";
    //TODO Consistency checks
    m_pNetwork->updateWeights();
}


//*************************************************************************************************************

void DeepViewer::initScene()
{
    m_pScene = new QGraphicsScene(this);

    connect(m_pNetwork, &Network::update_signal, this, &DeepViewer::redrawScene);
    connect(m_pNetwork, &Network::updateWeightThreshold_signal, this, &DeepViewer::updateSceneItems);
    connect(m_pNetwork, &Network::updateWeightStrength_signal, this, &DeepViewer::redrawScene);
}


//*************************************************************************************************************

void DeepViewer::removeSceneItems()
{
    if(!m_pNetwork->isSetup())
        return;

    //
    // Remove layer nodes from scene
    //
    for (int i = 0; i < m_pNetwork->layerNodes().size(); ++i) {
        for (int j = 0; j < m_pNetwork->layerNodes()[i].size(); ++j) {
            if(m_pNetwork->layerNodes()[i][j]->scene()) {
                m_pScene->removeItem(m_pNetwork->layerNodes()[i][j]);
            }
        }
    }

    //
    // Remove edges from scene
    //
    for (int i = 0; i < m_pNetwork->edges().size(); ++i) {
        for (int j = 0; j < m_pNetwork->edges()[i].size(); ++j) {
            if(m_pNetwork->edges()[i][j]->scene()) {
                m_pScene->removeItem(m_pNetwork->edges()[i][j]);
            }
        }
    }
}


//*************************************************************************************************************

void DeepViewer::updateSceneItems()
{

    if(!m_pNetwork->isSetup())
        return;

    //
    // Append layer nodes to scene
    //
    for (int i = 0; i < m_pNetwork->layerNodes().size(); ++i) {
        for (int j = 0; j < m_pNetwork->layerNodes()[i].size(); ++j) {
            if(!m_pNetwork->layerNodes()[i][j]->scene()) {
                m_pScene->addItem(m_pNetwork->layerNodes()[i][j]);
            }
        }
    }

    //
    // Append edges to scene
    //
    for (int i = 0; i < m_pNetwork->edges().size(); ++i) {
        for (int j = 0; j < m_pNetwork->edges()[i].size(); ++j) {
            if(std::fabs(m_pNetwork->edges()[i][j]->weight()) > m_pNetwork->weightThreshold()) {
                if(!m_pNetwork->edges()[i][j]->scene()) {
                    m_pScene->addItem(m_pNetwork->edges()[i][j]);
                }
            }
            else if(m_pNetwork->edges()[i][j]->scene()) {
                m_pScene->removeItem(m_pNetwork->edges()[i][j]);
            }
        }
    }

    redrawScene();
}


//*************************************************************************************************************

void DeepViewer::redrawScene()
{
    m_pScene->update(m_pView->getGraphicsView()->sceneRect());
}
