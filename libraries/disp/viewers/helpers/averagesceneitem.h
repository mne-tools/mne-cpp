//=============================================================================================================
/**
 * @file     averagesceneitem.h
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
 * @brief    Contains the declaration of the AverageSceneItem class.
 *
 */

#ifndef AVERAGESCENEITEM_H
#define AVERAGESCENEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "../scalingview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsObject>

//=============================================================================================================
// EIGEN INCLUDES
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
// DEFINE TYPEDEFS
//=============================================================================================================

typedef QPair<const double*,qint32> RowVectorPair;

//=============================================================================================================
/**
 * DECLARE CLASS AverageSceneItem
 *
 * @brief The AverageSceneItem class provides a new data structure for visualizing averages in a 2D layout.
 */
class DISPSHARED_EXPORT AverageSceneItem : public QGraphicsObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a AverageSceneItem.
     */
    AverageSceneItem(const QString& channelName,
                     int channelNumber,
                     const QPointF& channelPosition,
                     int channelKind,
                     int channelUnit,
                     const QColor& color = Qt::yellow);

    //=========================================================================================================
    /**
     * Reimplemented virtual functions
     */
    QRectF boundingRect() const;
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    //=========================================================================================================
    /**
     * Sets the item color to the input parameter viewColor
     *
     * @param[in] viewColor    desired item color.
     */
    void setDefaultColor(const QColor& viewColor);

    QString                                         m_sChannelName;             /**< The channel name.*/
    int                                             m_iChannelNumber;           /**< The channel number.*/
    int                                             m_iChannelKind;             /**< The channel kind.*/
    int                                             m_iChannelUnit;             /**< The channel unit.*/
    int                                             m_iTotalNumberChannels;     /**< The total number of channels loaded in the curent evoked data set.*/
    int                                             m_iFontTextSize;            /**< The font text size of the electrode names.*/
    int                                             m_iMaxWidth;                /**< The max width. */
    int                                             m_iMaxHeigth;               /**< The max heigth. */

    bool                                            m_bIsBad;                   /**< Whether this channel is bad. */

    QPointF                                         m_qpChannelPosition;        /**< The channels 2D position in the scene.*/
    QList<QPair<QString, RowVectorPair> >           m_lAverageData;             /**< The channels average data which is to be plotted.*/

    QPair<int,int>                                  m_firstLastSample;          /**< The first and last sample.*/

    QMap<qint32,float>                              m_scaleMap;                 /**< Map with all channel types and their current scaling value.*/
    QMap<QString, bool>                             m_qMapAverageActivation;    /**< The average activation information.*/
    QMap<QString, QColor>                           m_qMapAverageColor;         /**< The average color information.*/

    QRectF                                          m_rectBoundingRect;         /**< The bounding rect. */

    QColor                                          m_colorDefault;             /**< The color for the avergaed signal. */

protected:
    //=========================================================================================================
    /**
     * Create a plot path and paint the average data
     *
     * @param[in] painter The painter used to plot in this item.
     */
    void paintAveragePath(QPainter *painter);

    //=========================================================================================================
    /**
     * Create a plot path and paint the average data
     *
     * @param[in] painter The painter used to plot in this item.
     */
    void paintStimLine(QPainter *painter);

signals:
    //=========================================================================================================
    /**
     * Signal to request a scene update.
     */
    void sceneUpdateRequested();
};
} // NAMESPACE DISPLIB

#endif // AVERAGESCENEITEM_H
