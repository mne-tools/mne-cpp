//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sensor_layout_figure.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SensorLayoutFigure class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensor_layout_figure.h"
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorLayoutFigure::SensorLayoutFigure()
: m_kind(FIFFV_MEG_CH)
{
}

//=============================================================================================================

void SensorLayoutFigure::set_selection_callback(std::function<void()> callback)
{
    m_selectionCallback = std::move(callback);
}

//=============================================================================================================

void SensorLayoutFigure::set_layout(const std::map<std::string, Point>& layout, const std::vector<std::string>& bad)
{
    std::vector<SensorLayout::Sensor> sensors;
    sensors.reserve(layout.size());
    for (const auto& [name, position] : layout) {
        SensorLayout::Sensor sensor;
        sensor.name = name;
        sensor.position = position;
        sensor.kind = name.find("EEG") != std::string::npos ? FIFFV_EEG_CH : FIFFV_MEG_CH;
        sensor.unit = FIFF_UNIT_T_M;
        sensor.bad = std::ranges::contains(bad, name);
        sensors.push_back(std::move(sensor));
    }
    // New sensors start unselected; a lost selection is reported
    const bool hadSelection = !m_layout.selected().empty();
    m_layout.set_sensors(std::move(sensors));
    changed(Change::Bounds);
    if (hadSelection) {
        selection_changed();
    }
}

//=============================================================================================================

void SensorLayoutFigure::set_channel_kind(int kind)
{
    m_kind = kind;
}

//=============================================================================================================

void SensorLayoutFigure::set_selectable(const std::vector<std::string>& names)
{
    bool deselected = false;
    for (SensorLayout::Sensor& sensor : m_layout.sensors()) {
        sensor.visible = sensor.kind == m_kind;
        sensor.selectable = sensor.visible && std::ranges::contains(names, sensor.name);
        deselected |= sensor.selected && !sensor.selectable;
        sensor.selected = sensor.selected && sensor.selectable;
    }
    changed(Change::Bounds);
    if (deselected) {
        selection_changed();
    }
}

//=============================================================================================================

void SensorLayoutFigure::select_names(const std::vector<std::string>& names)
{
    if (m_layout.select_names(names)) {
        changed();
        selection_changed();
    }
}

//=============================================================================================================

void SensorLayoutFigure::highlight_names(const std::vector<std::string>& names)
{
    for (SensorLayout::Sensor& sensor : m_layout.sensors()) {
        sensor.highlighted = std::ranges::contains(names, sensor.name);
    }
    changed();
}

//=============================================================================================================

void SensorLayoutFigure::clear()
{
    const bool hadSelection = !m_layout.selected().empty();
    m_layout.set_sensors({});
    changed(Change::Bounds);
    if (hadSelection) {
        selection_changed();
    }
}

//=============================================================================================================

std::vector<std::string> SensorLayoutFigure::selected_names() const
{
    std::vector<std::string> names;
    for (std::size_t index : m_layout.selected()) {
        names.push_back(m_layout.sensors()[index].name);
    }
    return names;
}

//=============================================================================================================

std::vector<SensorLayout::Sensor> SensorLayoutFigure::selection() const
{
    const bool anySelected = !m_layout.selected().empty();
    std::vector<SensorLayout::Sensor> sensors;
    for (const SensorLayout::Sensor& sensor : m_layout.sensors()) {
        if (anySelected ? sensor.selected : sensor.visible) {
            sensors.push_back(sensor);
        }
    }
    return sensors;
}

//=============================================================================================================

Rect SensorLayoutFigure::bounds() const
{
    return m_layout.bounds();
}

//=============================================================================================================

void SensorLayoutFigure::draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    m_layout.draw(canvas, metrics, font);
}

//=============================================================================================================

bool SensorLayoutFigure::press(Point point, Modifier modifiers)
{
    const bool extend = has_flag(modifiers, Modifier::Ctrl);
    const bool hit = m_layout.sensor_at(point) >= 0;
    // A press beside the sensors clears the selection before a band starts, unless Ctrl is held
    if ((hit || !extend) && m_layout.click(point, extend)) {
        changed();
        selection_changed();
    }
    return hit;
}

//=============================================================================================================

bool SensorLayoutFigure::band_selection_enabled() const
{
    return true;
}

//=============================================================================================================

void SensorLayoutFigure::band_selected(const Rect& band, Modifier modifiers)
{
    if (m_layout.select_in(band, has_flag(modifiers, Modifier::Ctrl))) {
        changed();
        selection_changed();
    }
}

//=============================================================================================================

void SensorLayoutFigure::selection_changed() const
{
    if (m_selectionCallback) {
        m_selectionCallback();
    }
}
