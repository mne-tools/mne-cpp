//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sensor_layout.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SensorLayout draws the sensors of a 2-D layout as labelled dots and keeps which of them are selected.
 */

#ifndef SCENE_SENSOR_LAYOUT_H
#define SCENE_SENSOR_LAYOUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"
#include "scene_global.h"
#include "style.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

class TextMetrics;

/**
 * @brief Sensors of a 2-D layout (an .lout file) as labelled dots, with their selection.
 *
 * A sensor at layout position (x, y) sits at (10 x, -10 y) in scene coordinates, y pointing down. Hidden sensors are
 * neither drawn nor hit; sensors that cannot be selected are drawn faint. Clicks and rubber bands change the
 * selection as in a QGraphicsScene: without @c extend they replace it, with @c extend (Ctrl held) a click toggles
 * one sensor and a band adds to the selection.
 *
 * @snippet ex_scene/main.cpp sensor_layout_usage
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT SensorLayout
{
public:
    /** One sensor. */
    struct Sensor
    {
        std::string name;         /**< Name, drawn above the dot. */
        Point position;           /**< Position in layout units, y pointing up. */
        int kind = 0;             /**< Channel kind, e.g. FIFFV_MEG_CH. */
        int unit = 0;             /**< Channel unit, e.g. FIFF_UNIT_T_M. */
        bool bad = false;         /**< Bad sensors are filled red. */
        bool visible = true;      /**< Hidden sensors are neither drawn nor hit. */
        bool selectable = true;   /**< Sensors that cannot be selected are drawn faint. */
        bool selected = false;    /**< Selected sensors have a red ring. */
        bool highlighted = false; /**< Highlighted sensors have an orange ring. */
        Color color{0, 0, 255};   /**< Fill of good sensors. */
    };

    /**
     * Sets the sensors, drawn in this order, so later ones lie on top.
     *
     * @param[in] sensors   The sensors.
     */
    void set_sensors(std::vector<Sensor> sensors);

    /** @return The sensors. */
    const std::vector<Sensor>& sensors() const
    {
        return m_sensors;
    }

    /** @return The sensors, to change their state. */
    std::vector<Sensor>& sensors()
    {
        return m_sensors;
    }

    /**
     * @param[in] index   Index of a sensor.
     *
     * @return The centre of its dot in scene coordinates.
     */
    Point scene_position(std::size_t index) const;

    /**
     * @param[in] index   Index of a sensor.
     *
     * @return The area of its dot and name in scene coordinates, which clicks and bands hit.
     */
    Rect sensor_rect(std::size_t index) const;

    /** @return The area of the visible sensors in scene coordinates; empty without any. */
    Rect bounds() const;

    /**
     * @param[in] point   Point in scene coordinates.
     *
     * @return The index of the topmost visible, selectable sensor at @p point, or -1.
     */
    int sensor_at(Point point) const;

    /**
     * Changes the selection as a click at @p point does.
     *
     * @param[in] point    Point in scene coordinates.
     * @param[in] extend   Toggle the sensor there instead of selecting only it (Ctrl held).
     *
     * @return Whether the selection changed.
     */
    bool click(Point point, bool extend);

    /**
     * Selects the visible, selectable sensors whose area intersects @p band.
     *
     * @param[in] band     Rubber band in scene coordinates.
     * @param[in] extend   Add to the selection instead of replacing it (Ctrl held).
     *
     * @return Whether the selection changed.
     */
    bool select_in(const Rect& band, bool extend);

    /**
     * Selects the sensors named in @p names and deselects all others.
     *
     * @param[in] names   Names of the sensors to select.
     *
     * @return Whether the selection changed.
     */
    bool select_names(const std::vector<std::string>& names);

    /** @return The indices of the selected sensors, in order. */
    std::vector<std::size_t> selected() const;

    /**
     * Draws the sensors in scene coordinates with the current transform of @p canvas.
     *
     * @param[in, out] canvas    Canvas to draw on.
     * @param[in]      metrics   Text metrics of the backend, to centre the names.
     * @param[in]      font      Font of the names.
     */
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const;

private:
    std::vector<Sensor> m_sensors;
};

} // namespace mne::scene

#endif // SCENE_SENSOR_LAYOUT_H
