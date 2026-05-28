//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     averagescene.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    QGraphicsScene that lays @ref AverageSceneItem mini-traces on the active sensor layout.
 *
 * AverageScene extends @ref LayoutScene with the per-condition colour
 * table, the cached @c FiffInfo channel map and the dispatcher that
 * creates / refreshes one @ref AverageSceneItem per visible sensor.
 * It is the data backbone of @ref AverageLayoutView.
 */

#ifndef AVERAGESCENE_H
#define AVERAGESCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

#include "layoutscene.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class SelectionSceneItem;
class SelectionItem;

//=============================================================================================================
/**
 * @brief Sensor-layout @c QGraphicsScene placing one @ref AverageSceneItem per visible channel.
 *
 * Extends @ref LayoutScene with the per-condition colour table, the
 * cached @c FiffInfo channel map and the dispatcher that creates /
 * refreshes one @ref AverageSceneItem per visible sensor.
 */
class DISPSHARED_EXPORT AverageScene : public LayoutScene
{
    Q_OBJECT

public:
    typedef QSharedPointer<AverageScene> SPtr;              /**< Shared pointer type for AverageScene. */
    typedef QSharedPointer<const AverageScene> ConstSPtr;   /**< Const shared pointer type for AverageScene. */

    //=========================================================================================================
    /**
     * Constructs a AverageScene.
     */
    explicit AverageScene(QGraphicsView* view, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Sets the scale map to scaleMap.
     *
     * @param[in] scaleMap map with all channel types and their current scaling value.
     */
    void setScaleMap(const QMap<qint32, float> &scaleMap);

    //=========================================================================================================
    /**
     * Repaints all items from the layout data in the scene.
     *
     *  @param[in] selectedChannelItems    items which are to painted to the average scene
     */
    void repaintItems(const QList<QGraphicsItem*> &selectedChannelItems);

    //=========================================================================================================
    /**
     * Repaints all items from the layout data in the scene from a SelectionItem object
     *
     * @param[in] selectedChannelItems     data about items which are to be painted.
     */
    void repaintSelectionItems(const DISPLIB::SelectionItem &selectedChannelItems);

    //=========================================================================================================
    /**
     * Updates and repaints the scene
     */
    void updateScene();

    //=========================================================================================================
    /**
     * Set the activation per average map information
     *
     * @param[in] qMapActivationPerAverage     The average activation information.
     */
    void setActivationPerAverage(const QSharedPointer<QMap<QString, bool> > qMapActivationPerAverage);

    //=========================================================================================================
    /**
     * Set the color per average map information
     *
     * @param[in] qMapAverageColor     The average color information.
     */
    void setColorPerAverage(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor);

    //=========================================================================================================
    /**
     * Set the signal color for all items in the scene
     *
     * @return   The color used for all the itmes' signal paths.
     */
    const QColor& getSignalColorForAllItems();

    //=========================================================================================================
    /**
     * Sets the signal color for the items the scene holds
     *
     * @param[in] signalColor.
     */
    void setSignalItemColor(const QColor &signalColor);

private:
    QColor                          m_colGlobalItemSignalColor;     /**< The color used in all items to draw the signals.*/
    QMap<qint32, float>             m_qMapChScaling;                /**< Stored scale map applied to newly created items.*/

    QList<SelectionSceneItem*>      m_lSelectedChannelItems;        /**< Holds the selected channels from the selection manager.*/
};
} // NAMESPACE DISPLIB

#endif // AverageScene_H
