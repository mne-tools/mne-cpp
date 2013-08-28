//=============================================================================================================
/**
* @file     pluginitem.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2013
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    PluginItem class declaration
*
*/

#ifndef PLUGINITEM_H
#define PLUGINITEM_H

#include <mne_x/Interfaces/IPlugin.h>

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



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{

class Arrow;



class PluginItem : public QGraphicsPolygonItem
{
public:
    enum { Type = UserType + 15 };

    PluginItem(IPlugin::SPtr pPlugin, QMenu *contextMenu, QGraphicsItem *parent = 0);

    void removeArrow(Arrow *arrow);
    void removeArrows();
    IPlugin::PluginType diagramType() const { return m_pPlugin->getType(); }
    IPlugin::SPtr plugin() { return m_pPlugin; }

    QPolygonF polygon() const { return m_qPolygon; }
    void addArrow(Arrow *arrow);
    QPixmap image() const;

    int type() const { return Type;}

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

private:
    IPlugin::SPtr m_pPlugin;

    qint32 m_iWidth;
    qint32 m_iHeight;
    QPolygonF m_qPolygon;

    QLinearGradient m_qLinearGradientFace;
    QColor m_qColorContour;

    QMenu *m_contextMenu;
    QList<Arrow *> arrows;
};

} //NAMESPACE

#endif // PLUGINITEM_H
