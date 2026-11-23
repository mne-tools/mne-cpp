//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     averagescene.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October 2014
 * @brief    QGraphicsScene that lays @ref mne::disp::AverageSceneItem mini-traces on the active sensor layout.
 *
 * AverageScene extends @ref mne::disp::LayoutScene with the per-condition colour
 * table, the cached @c FiffInfo channel map and the dispatcher that
 * creates / refreshes one @ref mne::disp::AverageSceneItem per visible sensor.
 * It is the data backbone of @ref mne::disp::AverageLayoutView.
 */

#ifndef AVERAGESCENE_H
#define AVERAGESCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "layoutscene.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
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
    //=========================================================================================================
    /**
     * Constructs a AverageScene.
     *
     * @param[in] view    Graphics view that displays this scene.
     * @param[in] parent  Parent QObject (default 0).
     */
    explicit AverageScene(QGraphicsView* view, QObject* parent = 0);

    //=========================================================================================================
    /**
     * Sets the scale map to scaleMap.
     *
     * @param[in] scaleMap map with all channel types and their current scaling value.
     */
    void setScaleMap(const std::map<int, float>& scaleMap);

    //=========================================================================================================
    /**
     * Repaints all items from the layout data in the scene.
     *
     *  @param[in] selectedChannelItems    items which are to painted to the average scene
     */
    void repaintItems(const QList<QGraphicsItem*>& selectedChannelItems);

    //=========================================================================================================
    /**
     * Repaints all items from the layout data in the scene from a SelectionItem object
     *
     * @param[in] selectedChannelItems     data about items which are to be painted.
     */
    void repaintSelectionItems(const mne::disp::SelectionItem& selectedChannelItems);

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
    void setActivationPerAverage(const std::shared_ptr<std::map<std::string, bool>>& qMapActivationPerAverage);

    //=========================================================================================================
    /**
     * Set the color per average map information
     *
     * @param[in] qMapAverageColor     The average color information.
     */
    void setColorPerAverage(const std::shared_ptr<std::map<std::string, QColor>>& qMapAverageColor);

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
     * @param[in] signalColor New signal color.
     */
    void setSignalItemColor(const QColor& signalColor);

private:
    QColor m_colGlobalItemSignalColor;    /**< The color used in all items to draw the signals.*/
    std::map<int, float> m_qMapChScaling; /**< Stored scale map applied to newly created items.*/

    QList<SelectionSceneItem*> m_lSelectedChannelItems; /**< Holds the selected channels from the selection manager.*/
};
} // NAMESPACE mne_disp

#endif // AverageScene_H
