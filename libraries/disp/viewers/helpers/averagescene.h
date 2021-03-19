//=============================================================================================================
/**
 * @file     averagescene.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the AverageScene class.
 *
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
 * DECLARE CLASS AverageScene
 *
 * @brief The AverageScene class provides a reimplemented QGraphicsScene for 2D layout plotting.
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

    QList<SelectionSceneItem*>      m_lSelectedChannelItems;        /**< Holds the selected channels from the selection manager.*/
};
} // NAMESPACE DISPLIB

#endif // AverageScene_H
