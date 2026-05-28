//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file selectionscene.h
 * @since July 2018
 * @brief QGraphicsScene of the sensor-layout dots used by @ref ChannelSelectionView for interactive channel picking.
 *
 * SelectionScene places one @ref SelectionSceneItem per sensor at the
 * 2-D position read from the active layout and supports rubber-band
 * selection and lasso-style grouping. Selection changes are emitted
 * as channel-name lists so the host view can store them as named
 * @c .sel groups.
 */

#ifndef SELECTIONSCENE_H
#define SELECTIONSCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "layoutscene.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief QGraphicsScene of sensor-layout dots used by @ref ChannelSelectionView for interactive channel picking.
 *
 * Places one @ref SelectionSceneItem per sensor, supports rubber-band
 * selection and lasso grouping, and emits the resulting channel-name
 * lists so the host view can store them as named @c .sel groups.
 */
class DISPSHARED_EXPORT SelectionScene : public LayoutScene
{
    Q_OBJECT

public:
    typedef QSharedPointer<SelectionScene> SPtr;              /**< Shared pointer type for SelectionScene. */
    typedef QSharedPointer<const SelectionScene> ConstSPtr;   /**< Const shared pointer type for SelectionScene. */

    //=========================================================================================================
    /**
     * Constructs a SelectionScene.
     */
    explicit SelectionScene(QGraphicsView* view, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Updates layout data.
     *
     * @param[in] layoutMap layout data map.
     * @param[in] bad channel list.
     */
    void repaintItems(const QMap<QString, QPointF> &layoutMap,
                      QStringList badChannels);

    //=========================================================================================================
    /**
     * Hides all items described in list.
     *
     * @param[in] list string list with items name which are to be hidden.
     */
    void hideItems(QStringList visibleItems);

    int         m_iChannelTypeMode;     /**< The channel type mode. */
};
} // NAMESPACE DISPLIB

#endif // SelectionScene_H
