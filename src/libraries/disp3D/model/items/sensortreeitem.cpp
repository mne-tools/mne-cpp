//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sensortreeitem.cpp
 * @since 2026
 * @date  March 2026
 * @brief Per-sensor position, coil orientation and render-scale storage for the sensor layer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensortreeitem.h"

SensorTreeItem::SensorTreeItem(const QString& text, const QVector3D& pos, const QColor& color, float scale, int type)
    : AbstractTreeItem(text, type)
    , m_pos(pos)
    , m_scale(scale)
{
    setColor(color);
}

QVector3D SensorTreeItem::position() const
{
    return m_pos;
}

float SensorTreeItem::scale() const
{
    return m_scale;
}

void SensorTreeItem::setOrientation(const QMatrix4x4 &orient)
{
    m_orientation = orient;
    m_hasOrientation = true;
}

const QMatrix4x4& SensorTreeItem::orientation() const
{
    return m_orientation;
}

bool SensorTreeItem::hasOrientation() const
{
    return m_hasOrientation;
}
