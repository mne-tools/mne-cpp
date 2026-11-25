//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sensor_layout.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of SensorLayout.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensor_layout.h"
#include "text_metrics.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

// Scene units per layout unit
constexpr double kScale = 10.0;
// Radius of a dot
constexpr double kRadius = 15.0;

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void SensorLayout::set_sensors(std::vector<Sensor> sensors)
{
    m_sensors = std::move(sensors);
}

//=============================================================================================================

Point SensorLayout::scene_position(std::size_t index) const
{
    const Point p = m_sensors.at(index).position;
    return {kScale * p.x, -kScale * p.y};
}

//=============================================================================================================

Rect SensorLayout::sensor_rect(std::size_t index) const
{
    const Point p = scene_position(index);
    return {p.x - 25, p.y - 30, 50, 50};
}

//=============================================================================================================

Rect SensorLayout::bounds() const
{
    Rect area;
    for (std::size_t i = 0; i < m_sensors.size(); ++i) {
        if (m_sensors[i].visible) {
            area = area.united(sensor_rect(i));
        }
    }
    return area;
}

//=============================================================================================================

int SensorLayout::sensor_at(Point point) const
{
    for (std::size_t i = m_sensors.size(); i-- > 0;) {
        if (m_sensors[i].visible && m_sensors[i].selectable && sensor_rect(i).contains(point)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

//=============================================================================================================

bool SensorLayout::click(Point point, bool extend)
{
    const int hit = sensor_at(point);
    bool changed = false;
    for (std::size_t i = 0; i < m_sensors.size(); ++i) {
        const bool isHit = static_cast<int>(i) == hit;
        bool select = m_sensors[i].selected;
        if (extend) {
            select = isHit ? !select : select;
        } else {
            select = isHit;
        }
        changed |= select != m_sensors[i].selected;
        m_sensors[i].selected = select;
    }
    return changed;
}

//=============================================================================================================

bool SensorLayout::select_in(const Rect& band, bool extend)
{
    bool changed = false;
    for (std::size_t i = 0; i < m_sensors.size(); ++i) {
        const Sensor& sensor = m_sensors[i];
        const bool inside = sensor.visible && sensor.selectable && band.intersects(sensor_rect(i));
        const bool select = inside || (extend && sensor.selected);
        changed |= select != sensor.selected;
        m_sensors[i].selected = select;
    }
    return changed;
}

//=============================================================================================================

bool SensorLayout::select_names(const std::vector<std::string>& names)
{
    bool changed = false;
    for (Sensor& sensor : m_sensors) {
        const bool select = std::ranges::find(names, sensor.name) != names.end();
        changed |= select != sensor.selected;
        sensor.selected = select;
    }
    return changed;
}

//=============================================================================================================

std::vector<std::size_t> SensorLayout::selected() const
{
    std::vector<std::size_t> indices;
    for (std::size_t i = 0; i < m_sensors.size(); ++i) {
        if (m_sensors[i].selected) {
            indices.push_back(i);
        }
    }
    return indices;
}

//=============================================================================================================

void SensorLayout::draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    canvas.save();
    canvas.set_antialiasing(true);
    canvas.set_font(font);
    const double line = metrics.height(font);
    for (std::size_t i = 0; i < m_sensors.size(); ++i) {
        const Sensor& sensor = m_sensors[i];
        if (!sensor.visible) {
            continue;
        }
        const Point p = scene_position(i);
        canvas.save();
        canvas.set_opacity(sensor.selectable ? 1.0 : 0.25);
        // Shadow, then the dot
        canvas.set_pen(Pen::none());
        canvas.set_brush(Brush::solid({128, 128, 128}));
        canvas.draw_ellipse({p.x + 3, p.y + 3}, kRadius, kRadius);
        canvas.set_brush(Brush::solid(sensor.bad ? Color{255, 0, 0} : sensor.color));
        canvas.draw_ellipse(p, kRadius, kRadius);
        canvas.set_brush(Brush{});
        if (sensor.highlighted) {
            canvas.set_pen(Pen{{255, 84, 22}, 4});
            canvas.draw_ellipse(p, kRadius, kRadius);
        }
        if (sensor.selected) {
            canvas.set_pen(Pen{{255, 0, 0}, 5});
            canvas.draw_ellipse(p, kRadius, kRadius);
        }
        canvas.set_pen(Color::black());
        const double width = metrics.advance(font, sensor.name);
        canvas.draw_text(Rect{p.x - width / 2.0, p.y - 32, width, line}, Align::Left | Align::Top, sensor.name);
        canvas.restore();
    }
    canvas.restore();
}
