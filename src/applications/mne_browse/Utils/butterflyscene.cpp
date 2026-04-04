//=============================================================================================================
/**
 * @file     butterflyscene.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
 * @date     September, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the ButterflyScene class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "butterflyscene.h"

#include <QPainter>
#include <QGraphicsView>
#include <QGraphicsSceneWheelEvent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ButterflyScene::ButterflyScene(QGraphicsView* view, QObject* parent)
: LayoutScene(view, parent)
{
}


//*************************************************************************************************************

void ButterflyScene::setScaleMap(const QMap<QString,double> &scaleMap)
{
    QList<QGraphicsItem*> itemList = this->items();

    QListIterator<QGraphicsItem*> i(itemList);
    while (i.hasNext()) {
        ButterflySceneItem* ButterflySceneItemTemp = static_cast<ButterflySceneItem*>(i.next());
        ButterflySceneItemTemp->m_scaleMap = scaleMap;
    }

    this->update();
}


//*************************************************************************************************************

void ButterflyScene::setShowGFP(bool show)
{
    QList<QGraphicsItem*> itemList = this->items();

    QListIterator<QGraphicsItem*> i(itemList);
    while (i.hasNext()) {
        ButterflySceneItem* item = static_cast<ButterflySceneItem*>(i.next());
        item->m_bShowGFP = show;
    }

    this->update();
}


//*************************************************************************************************************

void ButterflyScene::repaintItems(const QList<QGraphicsItem *> &selectedChannelItems)
{
    this->clear();

    QListIterator<QGraphicsItem*> i(selectedChannelItems);
    while (i.hasNext()) {
        SelectionSceneItem* SelectionSceneItemTemp = static_cast<SelectionSceneItem*>(i.next());
        ButterflySceneItem* ButterflySceneItemTemp = new ButterflySceneItem(SelectionSceneItemTemp->m_sChannelName,
                                                                          SelectionSceneItemTemp->m_iChannelKind);

        this->addItem(ButterflySceneItemTemp);
    }
}


//*************************************************************************************************************

void ButterflyScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    m_crosshairPos = event->scenePos();
    m_crosshairVisible = true;
    invalidate(sceneRect(), QGraphicsScene::ForegroundLayer);
    LayoutScene::mouseMoveEvent(event);
}


//*************************************************************************************************************

void ButterflyScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);

    if(!m_crosshairVisible || items().isEmpty())
        return;

    // Find the first ButterflySceneItem to get time/amplitude info
    ButterflySceneItem* item = nullptr;
    for(auto* gi : items()) {
        item = dynamic_cast<ButterflySceneItem*>(gi);
        if(item && item->m_pFiffInfo)
            break;
    }
    if(!item || !item->m_pFiffInfo)
        return;

    // Map scene pos to item coords
    QPointF localPos = item->mapFromScene(m_crosshairPos);
    QRectF pa = item->plotArea();

    // Only draw if inside the plot area
    if(!pa.contains(localPos))
        return;

    QPointF sceneTopLeft = item->mapToScene(pa.topLeft());
    QPointF sceneBottomRight = item->mapToScene(pa.bottomRight());
    QRectF scenePlotArea(sceneTopLeft, sceneBottomRight);

    // Crosshair lines
    QPen crossPen(QColor(0, 0, 0, 150));
    crossPen.setWidthF(0.5);
    crossPen.setStyle(Qt::DashLine);
    painter->setPen(crossPen);

    // Vertical line
    painter->drawLine(QPointF(m_crosshairPos.x(), scenePlotArea.top()),
                      QPointF(m_crosshairPos.x(), scenePlotArea.bottom()));
    // Horizontal line
    painter->drawLine(QPointF(scenePlotArea.left(), m_crosshairPos.y()),
                      QPointF(scenePlotArea.right(), m_crosshairPos.y()));

    // Time/amplitude label
    double timeMs = item->xToTime(localPos.x()) * 1000.0;
    double amplitude = item->yToAmplitude(localPos.y());

    // Format amplitude with unit
    QString ampStr;
    if(item->m_iSetKind == FIFFV_MEG_CH && item->m_iSetUnit == FIFF_UNIT_T_M) {
        ampStr = QString::number(amplitude * 1e13, 'f', 1) + " fT/cm";
    } else if(item->m_iSetKind == FIFFV_MEG_CH) {
        ampStr = QString::number(amplitude * 1e15, 'f', 1) + " fT";
    } else {
        ampStr = QString::number(amplitude * 1e6, 'f', 2) + QStringLiteral(" \u00B5V");
    }

    QString label = QString("%1 ms, %2").arg(timeMs, 0, 'f', 1).arg(ampStr);

    // Draw label background
    QFont labelFont = painter->font();
    labelFont.setPointSizeF(8);
    painter->setFont(labelFont);
    QFontMetricsF fm(labelFont);
    QRectF textRect = fm.boundingRect(label);
    textRect.adjust(-4, -2, 4, 2);

    // Position: offset from crosshair to avoid overlap
    double labelX = m_crosshairPos.x() + 10;
    double labelY = m_crosshairPos.y() - 20;
    // Keep label in scene
    if(labelX + textRect.width() > scenePlotArea.right())
        labelX = m_crosshairPos.x() - textRect.width() - 10;
    if(labelY < scenePlotArea.top())
        labelY = m_crosshairPos.y() + 5;

    textRect.moveTo(labelX, labelY);
    painter->setBrush(QColor(255, 255, 255, 200));
    painter->setPen(Qt::NoPen);
    painter->drawRect(textRect);

    painter->setPen(Qt::black);
    painter->drawText(textRect, Qt::AlignCenter, label);
}


//*************************************************************************************************************

void ButterflyScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    // Ignore wheel events to prevent zoom in butterfly view
    event->accept();
}
