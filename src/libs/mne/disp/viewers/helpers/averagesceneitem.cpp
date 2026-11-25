//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     averagesceneitem.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October 2014
 * @brief    Implementation of the AverageSceneItem per-channel evoked-trace graphics item.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagesceneitem.h"
#include "../../plots/helpers/scenepainter.h"
#include <mne/scene/average_tile.h>
#include "../scalingview.h"

#include <mne/fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageSceneItem::AverageSceneItem(const QString& channelName,
                                   int channelNumber,
                                   const QPointF& channelPosition,
                                   int channelKind,
                                   int channelUnit,
                                   const QColor& color)
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
    m_rectBoundingRect = QRectF(-m_iMaxWidth / 2, -m_iMaxHeigth / 2, m_iMaxWidth, m_iMaxHeigth);
}

//=============================================================================================================

QRectF AverageSceneItem::boundingRect() const
{
    return m_rectBoundingRect;
}

//=============================================================================================================

void AverageSceneItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event)
    m_iFontTextSize = 6;
    emit sceneUpdateRequested();
}

//=============================================================================================================

void AverageSceneItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event)
    m_iFontTextSize = 3;
    emit sceneUpdateRequested();
}

//=============================================================================================================

void AverageSceneItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    scene::AverageTile tile;
    tile.box = {m_rectBoundingRect.x(), m_rectBoundingRect.y(), m_rectBoundingRect.width(), m_rectBoundingRect.height()};
    tile.name = m_sChannelName.toStdString();
    tile.name_size = m_iFontTextSize;
    tile.bad = m_bIsBad;
    tile.max_value = mne::disp::getScalingValue(m_scaleMap, m_iChannelKind, m_iChannelUnit);
    tile.stim_sample = m_firstLastSample.first;
    tile.columns = m_lAverageData.empty() ? 0 : m_lAverageData.front().second.second;
    for (const auto& [comment, data] : m_lAverageData) {
        // Default to active when no activation map has been configured
        if (m_qMapAverageActivation.contains(comment) && !m_qMapAverageActivation.at(comment)) {
            continue;
        }
        scene::AverageTile::Trace trace;
        trace.color = toSceneColor(m_qMapAverageColor.contains(comment) ? m_qMapAverageColor.at(comment) : m_colorDefault);
        // The evoked matrix is stored column by column
        for (int i = 0; i < data.second; ++i) {
            trace.samples.push_back(*(data.first + (i * m_iTotalNumberChannels) + m_iChannelNumber));
        }
        tile.traces.push_back(std::move(trace));
    }

    scene::Canvas canvas(m_rectBoundingRect.width(), m_rectBoundingRect.height());
    tile.draw(canvas, toSceneFont(painter->font()));
    paintScene(*painter, canvas.display_list(), painter->font());
}

//=============================================================================================================

void AverageSceneItem::setDefaultColor(const QColor& viewColor)
{
    m_colorDefault = viewColor;
}

//=============================================================================================================
