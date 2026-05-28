//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     averagesceneitem.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Wayne F. Mead <isk@imsorrykun.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the AverageSceneItem per-channel evoked-trace graphics item.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagesceneitem.h"
#include "../scalingview.h"

#include <fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QDebug>
#include <QStaticText>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageSceneItem::AverageSceneItem(const QString& channelName,
                                   int channelNumber,
                                   const QPointF &channelPosition,
                                   int channelKind,
                                   int channelUnit,
                                   const QColor &color)
: m_sChannelName(channelName)
, m_iChannelNumber(channelNumber)
, m_iChannelKind(channelKind)
, m_iChannelUnit(channelUnit)
, m_iTotalNumberChannels(0)
, m_iFontTextSize(3)
, m_iMaxWidth(120)
, m_iMaxHeigth(60)
, m_bIsBad(false)
, m_qpChannelPosition(channelPosition)
, m_colorDefault(color)
{
    m_rectBoundingRect = QRectF(-m_iMaxWidth/2, -m_iMaxHeigth/2, m_iMaxWidth, m_iMaxHeigth);
}

//=============================================================================================================

QRectF AverageSceneItem::boundingRect() const
{
    return m_rectBoundingRect;
}

//=============================================================================================================

void AverageSceneItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    m_iFontTextSize = 6;
    emit sceneUpdateRequested();
}

//=============================================================================================================

void AverageSceneItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    m_iFontTextSize = 3;
    emit sceneUpdateRequested();
}

//=============================================================================================================

void AverageSceneItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);

    if(m_bIsBad) {
        painter->setOpacity(0.20);
    }

//    //Plot bounding rect / drawing region of this item
//    painter->drawRect(this->boundingRect());

    //Plot stim time
    painter->save();
    paintStimLine(painter);
    painter->restore();

    //Plot average data
    painter->save();
    paintAveragePath(painter);
    painter->restore();

    // Plot channel name
    QStaticText staticElectrodeName = QStaticText(m_sChannelName);
    painter->save();
    QPen pen;
    pen.setColor(Qt::red);
    pen.setWidthF(0);
    pen.setCosmetic(true);

    QFont f = painter->font();
    f.setPointSizeF(m_iFontTextSize);
    painter->setFont(f);

    painter->setPen(pen);
    painter->drawStaticText(boundingRect().x(), boundingRect().y() - m_iFontTextSize, staticElectrodeName);
    painter->restore();

    //Plot bounding rect
//    painter->save();
//    pen.setColor(Qt::red);
//    painter->setPen(pen);
//    painter->drawRect(this->boundingRect());
//    painter->restore();
}

//=============================================================================================================

void AverageSceneItem::paintAveragePath(QPainter *painter)
{
    if(m_lAverageData.size() == 0)
        return;

    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)

    float fMaxValue = DISPLIB::getScalingValue(m_scaleMap, m_iChannelKind, m_iChannelUnit);

    //Plot averaged data
    QRectF boundingRect = this->boundingRect();
    double dScaleY = (boundingRect.height())/(2*fMaxValue);

    //do for all currently stored evoked set data
    for(int dataIndex = 0; dataIndex < m_lAverageData.size(); ++dataIndex) {
        QString sAvrComment = m_lAverageData.at(dataIndex).first;

        // Default to active when no activation map has been configured
        if(!m_qMapAverageActivation.contains(sAvrComment) || m_qMapAverageActivation[sAvrComment]) {
            const double* averageData = m_lAverageData.at(dataIndex).second.first;
            int totalCols =  m_lAverageData.at(dataIndex).second.second;
            if(totalCols <= 0)
                continue;

            //Calculate X step to fill the full bounding rect width
            double xStep = boundingRect.width() / static_cast<double>(totalCols);

            //Create path starting at the left-center of the bounding rect
            double centerY = boundingRect.y() + boundingRect.height() / 2.0;
            QPainterPath path(QPointF(boundingRect.x(), centerY));
            QPen pen;
            pen.setStyle(Qt::SolidLine);
            pen.setColor(m_colorDefault);

            if(m_qMapAverageColor.contains(sAvrComment)) {
                pen.setColor(m_qMapAverageColor[sAvrComment]);
            }

            pen.setWidthF(0);
            pen.setCosmetic(true);
            painter->setPen(pen);

            for(int i = 0; i < totalCols; ++i) {
                //evoked matrix is stored in column major
                double val = (*(averageData + (i * m_iTotalNumberChannels) + m_iChannelNumber)) * dScaleY;

                //Clamp to bounding rect height
                double halfH = boundingRect.height() / 2.0;
                if(val > halfH) val = halfH;
                else if(val < -halfH) val = -halfH;

                double xPos = boundingRect.x() + i * xStep;
                path.lineTo(QPointF(xPos, centerY - val));
            }

            painter->drawPath(path);
        }
    }
}

//=============================================================================================================

void AverageSceneItem::paintStimLine(QPainter *painter)
{
    if(m_lAverageData.size() == 0)
        return;

    //Plot vertical and horizontal lines
    QRectF boundingRect = this->boundingRect();
    int totalCols = m_lAverageData.first().second.second;
    if(totalCols <= 0)
        return;

    double xStep = boundingRect.width() / static_cast<double>(totalCols);

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setColor(Qt::red);
    pen.setWidthF(0);
    pen.setCosmetic(true);
    painter->setPen(pen);

    QPainterPath path;

    //Stim line (at sample index corresponding to time=0)
    double stimX = boundingRect.x() + std::abs(m_firstLastSample.first) * xStep;
    path.moveTo(stimX, boundingRect.y());
    path.lineTo(stimX, boundingRect.y() + boundingRect.height());

    //Zero line
    double centerY = boundingRect.y() + boundingRect.height() / 2.0;
    path.moveTo(boundingRect.x(), centerY);
    path.lineTo(boundingRect.x() + boundingRect.width(), centerY);

    painter->drawPath(path);
}

//=============================================================================================================

void AverageSceneItem::setDefaultColor(const QColor &viewColor)
{
    m_colorDefault = viewColor;
}

//=============================================================================================================
