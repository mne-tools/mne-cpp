//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportsimpedancescene.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the implementation of the EEGoSportsImpedanceScene class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportsimpedancescene.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;
using namespace std;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsImpedanceScene::EEGoSportsImpedanceScene(QGraphicsView* view, QObject* parent)
: QGraphicsScene(parent)
, m_bRightMouseKeyPressed(false)
, m_qvView(view)
{
}

//=============================================================================================================

void EEGoSportsImpedanceScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() == Qt::RightButton)
        m_bRightMouseKeyPressed = true;

    QGraphicsScene::mousePressEvent(event);
}

//=============================================================================================================

void EEGoSportsImpedanceScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(m_bRightMouseKeyPressed)
    {
        if(m_mousePosition.x()-event->scenePos().x() > 0) // user moved mouse to the left while pressing the right mouse key
            scaleElectrodePositions(0.99);

        if(m_mousePosition.x()-event->scenePos().x() < 0) // user moved mouse to the right while pressing the right mouse key
            scaleElectrodePositions(1.01);
    }

    m_mousePosition = event->scenePos();

    QGraphicsScene::mouseMoveEvent(event);
}

//=============================================================================================================

void EEGoSportsImpedanceScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() == Qt::RightButton)
        m_bRightMouseKeyPressed = false;

    QGraphicsScene::mouseReleaseEvent(event);
}

//=============================================================================================================

void EEGoSportsImpedanceScene::scaleElectrodePositions(double scaleFactor)
{
    // Get scene items
    QList< QGraphicsItem *> itemList = this->items();

    // Update position
    for(int i = 0; i<itemList.size(); i++)
    {
        EEGoSportsElectrodeItem* item = (EEGoSportsElectrodeItem *) itemList.at(i);

        // Set both positions -> dunno why :-)
        item->setPosition(item->getPosition()*scaleFactor);
        item->setPos(item->pos()*scaleFactor);
    }

    this->update(this->sceneRect());
}