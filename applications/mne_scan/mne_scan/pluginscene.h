//=============================================================================================================
/**
 * @file     pluginscene.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief    PluginScene class declaration
 *
 */

#ifndef PLUGINSCENE_H
#define PLUGINSCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginitem.h"

#include <scShared/Management/pluginmanager.h>
#include <scShared/Management/pluginscenemanager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGraphicsSceneMouseEvent;
class QMenu;
class QPointF;
class QGraphicsLineItem;
class QColor;
class QAction;

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace MNESCAN
{

//=============================================================================================================
// MNESCAN FORWARD DECLARATIONS
//=============================================================================================================

class PluginGui;

class PluginScene : public QGraphicsScene
{
    Q_OBJECT

public:
    typedef QSharedPointer<PluginScene> SPtr;               /**< Shared pointer type for PluginScene. */
    typedef QSharedPointer<const PluginScene> ConstSPtr;    /**< Const shared pointer type for PluginScene. */

    enum Mode { InsertPluginItem, InsertLine, MovePluginItem};

    explicit PluginScene(QMenu *pMenuPluginItem, PluginGui *pPluginGui);

    ~PluginScene();

    //=========================================================================================================
    /**
     * Inserts an item depending on the selected action
     *
     * @param[in] pos   Position where to insert the plugin.
     */
    void insertItem(const QPointF& pos);

    //=========================================================================================================
    /**
     * Inserts the m_pActionPluginItem selected plugin into PluginSceneManager
     *
     * @param[in] pActionPluginItem     Current selected action item.
     * @param[in, out] pAddedPlugin         The added plugin.
     *
     * @return true if successfull.
     */
    bool insertPlugin(QAction* pActionPluginItem, SCSHAREDLIB::AbstractPlugin::SPtr &pAddedPlugin);

    inline void setMode(Mode mode);
    inline void setActionPluginItem(QAction* pAction);

signals:
    void itemInserted(PluginItem *item);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);

private:
//    bool isItemChange(int type);

    PluginGui*  m_pPluginGui;   /**< Corresponding plugin gui. */

    //Current info
    Mode            m_mode;
    QAction*        m_pActionPluginItem;    /**< Selected plugin. */

    QMenu *m_pMenuPluginItem;         /**< Plugin context menu. */

    bool leftButtonDown;
    QPointF startPoint;
    QGraphicsLineItem *line;
    QColor m_qColorLine;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

void PluginScene::setMode(Mode mode)
{
    m_mode = mode;
}

//=============================================================================================================

void PluginScene::setActionPluginItem(QAction* pAction)
{
    m_pActionPluginItem = pAction;
}
} //NAMESPACE

#endif // PLUGINSCENE_H
