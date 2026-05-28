//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sensortreeitem.h
 * @since March 2026
 * @brief Tree item for a single MEG / EEG sensor with position, optional coil orientation and rendered scale.
 *
 * A sensor item couples a 3-D position with an optional 3x3
 * coil-orientation matrix (embedded in a 4x4 for GPU upload). The
 * renderer picks a primitive per sensor type: oriented plate for
 * MEG magnetometers, barbell for gradiometers, sphere for EEG.
 * Without an explicit orientation the sensor falls back to a
 * world-aligned sphere &mdash; useful for raw position dumps.
 */

#ifndef SENSORTREEITEM_H
#define SENSORTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "abstracttreeitem.h"
#include <QVector3D>
#include <QMatrix4x4>

/**
 * @brief Tree item representing MEG or EEG sensor positions in the 3-D scene hierarchy.
 */
class DISP3DSHARED_EXPORT SensorTreeItem : public AbstractTreeItem
{
public:
    explicit SensorTreeItem(const QString& text, const QVector3D& pos, const QColor& color, float scale, int type = AbstractTreeItem::SensorItem);
    ~SensorTreeItem() = default;

    QVector3D position() const;
    float scale() const;

    //=========================================================================================================
    /**
     * Set the coil orientation matrix (3x3 rotation from coil_trans).
     */
    void setOrientation(const QMatrix4x4 &orient);

    //=========================================================================================================
    /**
     * Returns the coil orientation (identity if not set).
     */
    const QMatrix4x4& orientation() const;

    //=========================================================================================================
    /**
     * Returns true if an explicit orientation was set.
     */
    bool hasOrientation() const;

private:
    QVector3D m_pos;
    float m_scale;
    QMatrix4x4 m_orientation;   /**< Coil orientation (3x3 rotation in 4x4). */
    bool m_hasOrientation = false;
};

#endif // SENSORTREEITEM_H
