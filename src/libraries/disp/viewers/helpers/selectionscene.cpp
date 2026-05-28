//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file selectionscene.cpp
 * @since July 2018
 * @brief Implementation of the SelectionScene sensor-layout picker scene.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "selectionscene.h"
#include "selectionsceneitem.h"

#include <fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>
#include <QWidget>
#include <QMutableListIterator>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SelectionScene::SelectionScene(QGraphicsView* view, QObject* parent)
: LayoutScene(view, parent)
, m_iChannelTypeMode(FIFFV_MEG_CH)
{
}

//=============================================================================================================

void SelectionScene::repaintItems(const QMap<QString,QPointF> &layoutMap, QStringList badChannels)
{
    this->clear();

    QMapIterator<QString,QPointF > i(layoutMap);
    while (i.hasNext()) {
        i.next();
        SelectionSceneItem* SelectionSceneItemTemp;

        if(i.key().contains("EEG"))
            SelectionSceneItemTemp = new SelectionSceneItem(i.key(),
                                                              0,
                                                              i.value(),
                                                              FIFFV_EEG_CH,
                                                              FIFF_UNIT_T_M,
                                                              Qt::blue,
                                                              badChannels.contains(i.key()));
        else
            SelectionSceneItemTemp = new SelectionSceneItem(i.key(),
                                                              0,
                                                              i.value(),
                                                              FIFFV_MEG_CH,
                                                              FIFF_UNIT_T_M,
                                                              Qt::blue,
                                                              badChannels.contains(i.key()));

        this->addItem(SelectionSceneItemTemp);
    }
}

//=============================================================================================================

void SelectionScene::hideItems(QStringList visibleItems)
{
    //Hide all items which names are in the the string list visibleItems. All other items' opacity is set to 0.25 an dthey are no longer selectable.
    QList<QGraphicsItem *> itemList = this->items();

    for(int i = 0; i<itemList.size(); i++) {
        SelectionSceneItem* item = static_cast<SelectionSceneItem*>(itemList.at(i));

        if(item->m_iChannelKind == m_iChannelTypeMode) {
            item->show();

            if(!visibleItems.contains(item->m_sChannelName)) {
                item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                item->setOpacity(0.25);
            }
            else {
                item->setFlag(QGraphicsItem::ItemIsSelectable, true);
                item->setOpacity(1);
            }
        }
        else
            item->hide();
    }
}

