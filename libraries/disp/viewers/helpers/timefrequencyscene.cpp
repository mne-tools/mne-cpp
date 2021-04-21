//=============================================================================================================
/**
 * @file     timefrequencyscene.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the TimeFrequencyScene Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequencyscene.h"

#include "timefrequencysceneitem.h"
#include "selectionsceneitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsProxyWidget>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequencyScene::TimeFrequencyScene(QGraphicsView* view,
                                       QObject* parent)
: LayoutScene(view, parent)
{

}

//=============================================================================================================

void TimeFrequencyScene::repaintItems(const QList<QGraphicsItem *> &selectedChannelItems)
{
//    this->clear();

//    QListIterator<QGraphicsItem*> i(selectedChannelItems);

//    while (i.hasNext()) {
//        SelectionSceneItem* selectionSceneItemTemp = static_cast<SelectionSceneItem*>(i.next());
//        TimeFrequencySceneItem* averageSceneItemTemp = new TimeFrequencySceneItem();

//        this->addItem(averageSceneItemTemp);
//    }
}

//=============================================================================================================

void TimeFrequencyScene::repaintSelectionItems(const DISPLIB::SelectionItem &selectedChannelItems)
{
    this->clear();
    m_vItems.clear();

    for (int i = 0; i < selectedChannelItems.m_iChannelKind.size(); i++){
        TimeFrequencySceneItem* averageSceneItemTemp = new TimeFrequencySceneItem(selectedChannelItems.m_sChannelName[i],
                                                                                  selectedChannelItems.m_iChannelNumber[i],
                                                                                  selectedChannelItems.m_qpChannelPosition[i],
                                                                                  selectedChannelItems.m_iChannelKind[i],
                                                                                  selectedChannelItems.m_iChannelUnit[i]);

        QGraphicsProxyWidget* pWidget = this->addWidget(averageSceneItemTemp);

        m_vItems.push_back(averageSceneItemTemp);

        pWidget->setPos(75*selectedChannelItems.m_qpChannelPosition[i].x(), -75*selectedChannelItems.m_qpChannelPosition[i].y());
//        pWidget->resize(200,150);
    }
}

//=============================================================================================================

void TimeFrequencyScene::updateScene()
{
    this->update();
}

//=============================================================================================================

std::vector<TimeFrequencySceneItem*> TimeFrequencyScene::getItems() const
{
    return m_vItems;
}


