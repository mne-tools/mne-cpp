//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     arrow.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief     Arrow class declaration
 */

#ifndef ARROW_H
#define ARROW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginitem.h"
#include <scShared/Management/pluginconnectorconnection.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QGraphicsLineItem>
#include <QPainterPath>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGraphicsPolygonItem;
class QGraphicsLineItem;
class QGraphicsScene;
class QRectF;
class QGraphicsSceneMouseEvent;
class QPainterPath;

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace MNESCAN
{

class Arrow : public QGraphicsLineItem
{
public:
    enum { Type = UserType + 4 };

    Arrow(PluginItem *startItem, PluginItem *endItem, SCSHAREDLIB::PluginConnectorConnection::SPtr &connection, QGraphicsItem *parent = 0);

    int type() const { return Type; }
    QRectF boundingRect() const;
    QPainterPath shape() const;
    void setColor(const QColor &color) { m_qColor = color; }
    PluginItem *startItem() const { return m_StartItem; }
    PluginItem *endItem() const { return m_EndItem; }

    SCSHAREDLIB::PluginConnectorConnection::SPtr connection() { return m_pConnection; }

    void updatePosition();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

private:
    PluginItem *m_StartItem;
    PluginItem *m_EndItem;

    SCSHAREDLIB::PluginConnectorConnection::SPtr m_pConnection;

    QColor m_qColor;
    QPolygonF arrowHead;
    QPainterPath m_bezierPath;
};
} //NAMESPACE

#endif // ARROW_H
