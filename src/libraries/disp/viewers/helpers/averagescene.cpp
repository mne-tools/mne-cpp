//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file averagescene.cpp
 * @since July 2018
 * @brief Implementation of the AverageScene sensor-layout evoked scene.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagescene.h"

#include "averagesceneitem.h"
#include "selectionsceneitem.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageScene::AverageScene(QGraphicsView* view, QObject* parent)
: LayoutScene(view, parent)
, m_colGlobalItemSignalColor(QColor(0, 80, 180))
{
}

//=============================================================================================================

void AverageScene::setScaleMap(const QMap<qint32,float> &scaleMap)
{
    m_qMapChScaling = scaleMap;

    QList<QGraphicsItem*> itemList = this->items();

    QListIterator<QGraphicsItem*> i(itemList);
    while (i.hasNext()) {
        AverageSceneItem* AverageSceneItemTemp = static_cast<AverageSceneItem*>(i.next());
        AverageSceneItemTemp->m_scaleMap = scaleMap;
    }

    this->update();
}

//=============================================================================================================

void AverageScene::repaintItems(const QList<QGraphicsItem *> &selectedChannelItems)
{
    this->clear();

    QListIterator<QGraphicsItem*> i(selectedChannelItems);

    while (i.hasNext()) {
        SelectionSceneItem* selectionSceneItemTemp = static_cast<SelectionSceneItem*>(i.next());
        AverageSceneItem* averageSceneItemTemp = new AverageSceneItem(selectionSceneItemTemp->m_sChannelName,
                                                                      selectionSceneItemTemp->m_iChannelNumber,
                                                                      selectionSceneItemTemp->m_qpChannelPosition,
                                                                      selectionSceneItemTemp->m_iChannelKind,
                                                                      selectionSceneItemTemp->m_iChannelUnit,
                                                                      m_colGlobalItemSignalColor);

        connect(averageSceneItemTemp, &AverageSceneItem::sceneUpdateRequested,
                    this, &AverageScene::updateScene);

        averageSceneItemTemp->setPos(160 * selectionSceneItemTemp->m_qpChannelPosition.x(),
                                     -160 * selectionSceneItemTemp->m_qpChannelPosition.y());

        averageSceneItemTemp->m_scaleMap = m_qMapChScaling;
        this->addItem(averageSceneItemTemp);
    }
}

//=============================================================================================================

void AverageScene::repaintSelectionItems(const DISPLIB::SelectionItem &selectedChannelItems)
{
    this->clear();

    for (int i = 0; i < selectedChannelItems.m_iChannelKind.size(); i++){
        AverageSceneItem* averageSceneItemTemp = new AverageSceneItem(selectedChannelItems.m_sChannelName[i],
                                                                      selectedChannelItems.m_iChannelNumber[i],
                                                                      selectedChannelItems.m_qpChannelPosition[i],
                                                                      selectedChannelItems.m_iChannelKind[i],
                                                                      selectedChannelItems.m_iChannelUnit[i],
                                                                      m_colGlobalItemSignalColor);

        connect(averageSceneItemTemp, &AverageSceneItem::sceneUpdateRequested,
                this, &AverageScene::updateScene);

        averageSceneItemTemp->setPos(160 * selectedChannelItems.m_qpChannelPosition[i].x(),
                                     -160 * selectedChannelItems.m_qpChannelPosition[i].y());

        averageSceneItemTemp->m_scaleMap = m_qMapChScaling;
        this->addItem(averageSceneItemTemp);
    }
}

//=============================================================================================================

void AverageScene::setActivationPerAverage(const QSharedPointer<QMap<QString, bool> > qMapActivationPerAverage)
{
    QList<QGraphicsItem*> items = this->items();
    QListIterator<QGraphicsItem*> i(items);
    while (i.hasNext()) {
        if(AverageSceneItem* averageSceneItemTemp = dynamic_cast<AverageSceneItem*>(i.next())) {
            averageSceneItemTemp->m_qMapAverageActivation = *qMapActivationPerAverage;
        }
    }

    updateScene();
}

//=============================================================================================================

void AverageScene::setColorPerAverage(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor)
{
    QList<QGraphicsItem*> items = this->items();
    QListIterator<QGraphicsItem*> i(items);
    while (i.hasNext()) {
        if(AverageSceneItem* averageSceneItemTemp = dynamic_cast<AverageSceneItem*>(i.next())) {
            averageSceneItemTemp->m_qMapAverageColor = *qMapAverageColor;
        }
    }

    updateScene();
}

//=============================================================================================================

const QColor& AverageScene::getSignalColorForAllItems()
{
    return m_colGlobalItemSignalColor;
}

//=============================================================================================================

void AverageScene::updateScene()
{
    this->update();
}

//=============================================================================================================

void AverageScene::setSignalItemColor(const QColor &signalColor)
{
    QList<QGraphicsItem*> items = this->items();
    QListIterator<QGraphicsItem*> i(items);

    m_colGlobalItemSignalColor = signalColor;

    while (i.hasNext()) {
        if(AverageSceneItem* averageSceneItemTemp = dynamic_cast<AverageSceneItem*>(i.next())) {
            averageSceneItemTemp->setDefaultColor(signalColor);
            averageSceneItemTemp->update();
        }
    }

    repaintItems(items);
    this->update();
}
