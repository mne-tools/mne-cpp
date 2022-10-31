//=============================================================================================================
/**
 * @file     selectionscene.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the SelectionScene class.
 *
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

