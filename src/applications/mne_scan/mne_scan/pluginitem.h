//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginitem.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    PluginItem class declaration
 */

#ifndef PLUGINITEM_H
#define PLUGINITEM_H

#include <scShared/Plugins/abstractplugin.h>

#include <QFont>
#include <QGraphicsPixmapItem>
#include <QLinearGradient>
#include <QList>

class QPixmap;
class QGraphicsItem;
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QMenu;
class QGraphicsSceneContextMenuEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class QPolygonF;

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace MNESCAN
{

class Arrow;

class PluginItem : public QGraphicsPolygonItem
{
public:
    enum { Type = UserType + 15 };
    PluginItem(SCSHAREDLIB::AbstractPlugin::SPtr pPlugin, QMenu *contextMenu, QGraphicsItem *parent = 0);

    ~PluginItem();

    void removeArrow(Arrow *arrow);
    void removeArrows();
    SCSHAREDLIB::AbstractPlugin::PluginType diagramType() const { return m_pPlugin->getType(); }
    SCSHAREDLIB::AbstractPlugin::SPtr plugin() { return m_pPlugin; }

    QPolygonF polygon() const { return m_qPolygon; }
    void addArrow(Arrow *arrow);
    QPixmap image() const;

    QRectF boundingRect() const override;
    QPointF outputPortLocalPos() const;
    QPointF inputPortLocalPos() const;

    int type() const override { return Type;}

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0) override;

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void resizeAsRectangle(int width, int height);

private:
    SCSHAREDLIB::AbstractPlugin::SPtr m_pPlugin;

    qint32 m_iWidth;
    qint32 m_iHeight;
    QPolygonF m_qPolygon;

    QLinearGradient m_qLinearGradientFace;
    QColor m_qColorContour;
    QColor m_qColorFillTop;
    QFont m_nameFont;
    QFont m_typeFont;

    static constexpr qreal PORT_RADIUS = 5.0;
    static constexpr qreal CORNER_RADIUS = 8.0;
    static constexpr int HORIZONTAL_PADDING = 16;
    static constexpr int VERTICAL_PADDING = 8;
    static constexpr int TYPE_LABEL_HEIGHT = 14;
    static constexpr int MIN_WIDTH = 120;
    static constexpr int MIN_HEIGHT = 48;

    QMenu *m_contextMenu;
    QList<Arrow *> arrows;
};
} //NAMESPACE

#endif // PLUGINITEM_H
