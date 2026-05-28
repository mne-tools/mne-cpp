//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file selectionsceneitem.h
 * @since 2022
 * @date  February 2026
 * @brief Channel-selection group descriptor (@ref SelectionItem) and the matching @c QGraphicsItem (@ref SelectionSceneItem).
 *
 * SelectionItem is a value type holding a named list of channel names
 * loaded from an MNE @c .sel file. SelectionSceneItem is the visual
 * @c QGraphicsItem drawn for each sensor inside @ref SelectionScene;
 * it tracks selection / hover state and paints itself with the
 * modality-specific colour and shape.
 */

#ifndef SELECTIONSCENEITEM_H
#define SELECTIONSCENEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsItem>

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
 * @brief Value type describing one named selection group (label + list of channel names).
 *
 * Loaded from / saved to MNE @c .sel files; consumed by
 * @ref AverageScene and @ref ChannelSelectionView to limit display
 * to a subset of sensors.
 */
class DISPSHARED_EXPORT SelectionItem{
public:
    SelectionItem() {}
    ~SelectionItem() {}
    SelectionItem(const SelectionItem &);

    QList<QString>     m_sViewsToApply;
    QList<QString>     m_sChannelName;             /**< The channel's name.*/
    QList<int>         m_iChannelNumber;           /**< The channel number.*/
    QList<int>         m_iChannelKind;             /**< The channel kind.*/
    QList<int>         m_iChannelUnit;             /**< The channel unit.*/
    QList<QPointF>     m_qpChannelPosition;        /**< The channel's 2D position in the scene.*/
    bool               m_bShowAll;                 /**< Whether to show all channels. */
};

/**
 * @brief Graphics item representing a selectable electrode or channel in a 2-D layout scene.
 */
class DISPSHARED_EXPORT SelectionSceneItem : public QGraphicsItem
{

public:
    typedef QSharedPointer<SelectionSceneItem> SPtr;              /**< Shared pointer type for SelectionSceneItem. */
    typedef QSharedPointer<const SelectionSceneItem> ConstSPtr;   /**< Const shared pointer type for SelectionSceneItem. */

    //=========================================================================================================
    /**
     * Constructs a SelectionSceneItem.
     */
    SelectionSceneItem(QString channelName,
                       int channelNumber,
                       QPointF channelPosition,
                       int channelKind,
                       int channelUnit,
                       QColor channelColor = Qt::blue,
                       bool bIsBadChannel = false);

    //=========================================================================================================
    /**
     * Returns the bounding rect of the electrode item. This rect describes the area which the item uses to plot in.
     */
    QRectF boundingRect() const;

    //=========================================================================================================
    /**
     * Reimplemented paint function.
     */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QString     m_sChannelName;             /**< The channel's name.*/
    int         m_iChannelNumber;           /**< The channel number.*/
    int         m_iChannelKind;             /**< The channel kind.*/
    int         m_iChannelUnit;             /**< The channel unit.*/
    QPointF     m_qpChannelPosition;        /**< The channel's 2D position in the scene.*/
    QColor      m_cChannelColor;            /**< The current channel color.*/
    bool        m_bHighlightItem;           /**< Whether this item is to be highlighted.*/
    bool        m_bIsBadChannel;            /**< Whether this item is a bad channel.*/
};

} // NAMESPACE DISPLIB
#ifndef metatype_DISPLIB_selectionitem
#define metatype_DISPLIB_selectionitem
Q_DECLARE_METATYPE(DISPLIB::SelectionItem);
Q_DECLARE_METATYPE(DISPLIB::SelectionItem*);
#endif

#endif // SELECTIONSCENEITEM_H
