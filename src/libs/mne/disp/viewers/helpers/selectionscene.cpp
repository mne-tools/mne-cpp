//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     selectionscene.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2014
 * @brief    Implementation of the SelectionScene sensor-layout picker scene.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "selectionscene.h"
#include "selectionsceneitem.h"

#include <mne/fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>
#include <QWidget>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SelectionScene::SelectionScene(QGraphicsView* view, QObject* parent)
: LayoutScene(view, parent)
, m_iChannelTypeMode(FIFFV_MEG_CH)
{
}

//=============================================================================================================

void SelectionScene::repaintItems(const std::map<std::string, QPointF>& layoutMap, const std::vector<std::string>& badChannels)
{
    this->clear();

    for (const auto& [name, position] : layoutMap) {
        auto SelectionSceneItemTemp = std::make_unique<SelectionSceneItem>(QString::fromStdString(name),
                                                                           0,
                                                                           position,
                                                                           name.contains("EEG") ? FIFFV_EEG_CH : FIFFV_MEG_CH,
                                                                           FIFF_UNIT_T_M,
                                                                           Qt::blue,
                                                                           std::ranges::contains(badChannels, name));

        this->addItem(SelectionSceneItemTemp.release());
    }
}

//=============================================================================================================

void SelectionScene::hideItems(const std::vector<std::string>& visibleItems)
{
    //Hide all items which names are in the the string list visibleItems. All other items' opacity is set to 0.25 an dthey are no longer selectable.
    QList<QGraphicsItem*> itemList = this->items();

    for (int i = 0; i < itemList.size(); i++) {
        SelectionSceneItem* item = static_cast<SelectionSceneItem*>(itemList.at(i));

        if (item->m_iChannelKind == m_iChannelTypeMode) {
            item->show();

            if (!std::ranges::contains(visibleItems, item->m_sChannelName.toStdString())) {
                item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                item->setOpacity(0.25);
            } else {
                item->setFlag(QGraphicsItem::ItemIsSelectable, true);
                item->setOpacity(1);
            }
        } else
            item->hide();
    }
}
