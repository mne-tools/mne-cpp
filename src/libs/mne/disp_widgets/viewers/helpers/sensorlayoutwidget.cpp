//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sensorlayoutwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the SensorLayoutWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorlayoutwidget.h"
#include <mne/disp/viewers/helpers/selectionitem.h>
#include <mne/disp/plots/helpers/scenepainter.h>

#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorLayoutWidget::SensorLayoutWidget(QWidget* parent)
: SceneViewport(parent)
, m_kind(FIFFV_MEG_CH)
{
}

//=============================================================================================================

void SensorLayoutWidget::setLayout(const std::map<std::string, QPointF>& layoutMap, const std::vector<std::string>& badChannels)
{
    std::vector<scene::SensorLayout::Sensor> sensors;
    sensors.reserve(layoutMap.size());
    for (const auto& [name, position] : layoutMap) {
        scene::SensorLayout::Sensor sensor;
        sensor.name = name;
        sensor.position = {position.x(), position.y()};
        sensor.kind = name.find("EEG") != std::string::npos ? FIFFV_EEG_CH : FIFFV_MEG_CH;
        sensor.unit = FIFF_UNIT_T_M;
        sensor.bad = std::ranges::contains(badChannels, name);
        sensors.push_back(std::move(sensor));
    }
    // New sensors start unselected; as when a QGraphicsScene clears its items, a lost selection is reported
    const bool hadSelection = !m_layout.selected().empty();
    m_layout.set_sensors(std::move(sensors));
    contentsChanged();
    if (hadSelection) {
        emit selectionChanged();
    }
}

//=============================================================================================================

void SensorLayoutWidget::setChannelKind(int kind)
{
    m_kind = kind;
}

//=============================================================================================================

int SensorLayoutWidget::channelKind() const
{
    return m_kind;
}

//=============================================================================================================

void SensorLayoutWidget::setSelectable(const std::vector<std::string>& names)
{
    // As in a QGraphicsScene, hidden or unselectable sensors lose their selection
    bool deselected = false;
    for (scene::SensorLayout::Sensor& sensor : m_layout.sensors()) {
        sensor.visible = sensor.kind == m_kind;
        sensor.selectable = sensor.visible && std::ranges::contains(names, sensor.name);
        deselected |= sensor.selected && !sensor.selectable;
        sensor.selected = sensor.selected && sensor.selectable;
    }
    contentsChanged();
    if (deselected) {
        emit selectionChanged();
    }
}

//=============================================================================================================

void SensorLayoutWidget::selectNames(const std::vector<std::string>& names)
{
    if (m_layout.select_names(names)) {
        update();
        emit selectionChanged();
    }
}

//=============================================================================================================

void SensorLayoutWidget::highlightNames(const std::vector<std::string>& names)
{
    for (scene::SensorLayout::Sensor& sensor : m_layout.sensors()) {
        sensor.highlighted = std::ranges::contains(names, sensor.name);
    }
    update();
}

//=============================================================================================================

void SensorLayoutWidget::clear()
{
    const bool hadSelection = !m_layout.selected().empty();
    m_layout.set_sensors({});
    contentsChanged();
    if (hadSelection) {
        emit selectionChanged();
    }
}

//=============================================================================================================

std::vector<std::string> SensorLayoutWidget::selectedNames() const
{
    std::vector<std::string> names;
    for (std::size_t index : m_layout.selected()) {
        names.push_back(m_layout.sensors()[index].name);
    }
    return names;
}

//=============================================================================================================

SelectionItem SensorLayoutWidget::selection() const
{
    const bool anySelected = !m_layout.selected().empty();
    SelectionItem item;
    for (const scene::SensorLayout::Sensor& sensor : m_layout.sensors()) {
        if (anySelected ? sensor.selected : sensor.visible) {
            item.m_sChannelName.push_back(sensor.name);
            item.m_iChannelNumber.push_back(0);
            item.m_iChannelKind.push_back(sensor.kind);
            item.m_iChannelUnit.push_back(sensor.unit);
            item.m_qpChannelPosition.emplace_back(sensor.position.x, sensor.position.y);
        }
    }
    return item;
}

//=============================================================================================================

const scene::SensorLayout& SensorLayoutWidget::sensorLayout() const
{
    return m_layout;
}

//=============================================================================================================

scene::Rect SensorLayoutWidget::sceneBounds() const
{
    return m_layout.bounds();
}

//=============================================================================================================

void SensorLayoutWidget::drawScene(scene::Canvas& canvas) const
{
    m_layout.draw(canvas, QtTextMetrics(font()), toSceneFont(font()));
}

//=============================================================================================================

bool SensorLayoutWidget::scenePressed(scene::Point point, Qt::KeyboardModifiers modifiers)
{
    const bool extend = modifiers.testFlag(Qt::ControlModifier);
    const bool hit = m_layout.sensor_at(point) >= 0;
    // A press beside the sensors clears the selection before a band starts, unless Ctrl is held
    if ((hit || !extend) && m_layout.click(point, extend)) {
        update();
        emit selectionChanged();
    }
    return hit;
}

//=============================================================================================================

void SensorLayoutWidget::sceneBandSelected(const scene::Rect& band, Qt::KeyboardModifiers modifiers)
{
    if (m_layout.select_in(band, modifiers.testFlag(Qt::ControlModifier))) {
        update();
        emit selectionChanged();
    }
}

//=============================================================================================================

bool SensorLayoutWidget::bandSelectionEnabled() const
{
    return true;
}
