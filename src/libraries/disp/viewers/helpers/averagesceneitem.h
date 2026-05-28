//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Wayne F. Mead <isk@imsorrykun.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file averagesceneitem.h
 * @since July 2018
 * @brief QGraphicsObject painting one channel's averaged evoked trace at its sensor position.
 *
 * AverageSceneItem is a fixed-size mini-plot drawn for every channel
 * by @ref AverageScene. It stores the per-condition data vectors,
 * their @c QColor mapping and the y-scale settings, then paints the
 * overlaid traces, the t = 0 marker and the baseline inside its
 * bounding rect during @c paint().
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
 * @brief QGraphicsObject painting one channel's averaged evoked traces inside @ref AverageScene.
 *
 * Stores per-condition data vectors and colours and paints overlaid
 * traces, the t = 0 marker and the baseline inside its bounding
 * rect during @c paint().
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
