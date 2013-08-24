//=============================================================================================================
/**
* @file     pluginscene.h
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
* @brief    PluginScene class declaration
*
*/

#ifndef PLUGINSCENE_H
#define PLUGINSCENE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginitem.h"

#include <mne_x/Management/pluginmanager.h>
#include <mne_x/Management/pluginscenemanager.h>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGraphicsScene>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGraphicsSceneMouseEvent;
class QMenu;
class QPointF;
class QGraphicsLineItem;
class QColor;
class QAction;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class PluginGui;




class PluginScene : public QGraphicsScene
{
    Q_OBJECT
public:
    typedef QSharedPointer<PluginScene> SPtr;               /**< Shared pointer type for PluginScene. */
    typedef QSharedPointer<const PluginScene> ConstSPtr;    /**< Const shared pointer type for PluginScene. */

    enum Mode { InsertItem, InsertLine, MoveItem};

    explicit PluginScene(QMenu *itemMenu, PluginGui *pPluginGui);

//SLOTS
    inline void setMode(Mode mode);
    inline void setItemAction(QAction* pAction);
//    inline void setItemType(PluginItem::DiagramType type);
//    inline void setItemName(QString name);

signals:
    void itemInserted(PluginItem *item);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);

private:
    bool isItemChange(int type);


    PluginGui*  m_pPluginGui;   /**< Corresponding plugin gui */

    //Current info
    Mode                    m_mode;
//    PluginItem::DiagramType m_itemType;
//    QString                 m_itemName;
    QAction*                m_pItemAction;


    QMenu *m_pMenuItem;         /**< Plugin context menu */

    bool leftButtonDown;
    QPointF startPoint;
    QGraphicsLineItem *line;
    QColor m_qColorLine;
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

void PluginScene::setMode(Mode mode)
{
    m_mode = mode;
}


//*************************************************************************************************************

void PluginScene::setItemAction(QAction* pAction)
{
    m_pItemAction = pAction;
}



////*************************************************************************************************************

//void PluginScene::setItemType(PluginItem::DiagramType type)
//{
//    m_itemType = type;
//}


////*************************************************************************************************************

//void PluginScene::setItemName(QString name)
//{
//    m_itemName = name;
//}

} //NAMESPACE

#endif // PLUGINSCENE_H
