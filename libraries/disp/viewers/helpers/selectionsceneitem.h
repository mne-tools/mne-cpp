//=============================================================================================================
/**
 * @file     selectionsceneitem.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2014
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
 * @brief    Contains the declaration of the SelectionSceneItem class.
 *
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
 * DECLARE CLASS SelectionSceneItem
 *
 * @brief The SelectionSceneItem class provides a new data structure for visualizing channels in a 2D layout.
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
