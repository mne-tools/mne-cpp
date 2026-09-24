//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     butterflyscene.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     October, 2014
 * @version  2.1.0
 * @brief    Contains the declaration of the ButterflyScene class.
 */

#ifndef BUTTERFLYSCENE_H
#define BUTTERFLYSCENE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "butterflysceneitem.h"

#include <disp/viewers/helpers/layoutscene.h>
#include <disp/viewers/helpers/selectionsceneitem.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace MNEBROWSE
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//=============================================================================================================
/**
 * ButterflyScene...
 *
 * @brief The ButterflyScene class provides a reimplemented QGraphicsScene for 2D layout plotting.
 */
class ButterflyScene : public LayoutScene
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a ButterflyScene.
     */
    explicit ButterflyScene(QGraphicsView* view, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Sets the scale map to scaleMap.
     *
     * @param [in] scaleMap map with all channel types and their current scaling value.
     */
    void setScaleMap(const QMap<QString,double> &scaleMap);

    //=========================================================================================================
    /**
     * Enable or disable the GFP (Global Field Power) overlay on all items.
     */
    void setShowGFP(bool show);

    //=========================================================================================================
    /**
     * Repaints all items from the layout data in the scene.
     *
     *  @param [in] selectedChannelItems items which are to painted to the average scene
     */
    void repaintItems(const QList<QGraphicsItem*> &selectedChannelItems);

protected:
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;

private:
    QPointF m_crosshairPos;
    bool    m_crosshairVisible = false;
};

} // NAMESPACE

#endif // BUTTERFLYSCENE_H
