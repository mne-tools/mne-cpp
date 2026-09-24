//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginscene.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    PluginScene class declaration
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
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

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
