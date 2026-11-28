//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sensor_layout_figure.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SensorLayoutFigure: the sensors of a 2-D layout as an interactive figure to pick channels from.
 */

#ifndef VIZ_SENSOR_LAYOUT_FIGURE_H
#define VIZ_SENSOR_LAYOUT_FIGURE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "scene_figure.h"
#include "sensor_layout.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief The sensors of one channel kind as labelled dots, picked by click, Ctrl+click and rubber band.
 *
 * Sensors whose name contains "EEG" are EEG channels, all others MEG gradiometers. Only sensors of the shown
 * kind are drawn, and only the selectable ones can be picked. As in a QGraphicsScene, sensors that are hidden or
 * become unselectable lose their selection, and every change of the selection is reported.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SensorLayoutFigure : public SceneFigure
{
public:
    //=========================================================================================================
    /**
     * Constructs a figure without sensors that shows MEG channels.
     */
    SensorLayoutFigure();

    //=========================================================================================================
    /**
     * @param[in] callback   Called when the selection changed.
     */
    void set_selection_callback(std::function<void()> callback);

    //=========================================================================================================
    /**
     * Sets the sensors, all unselected.
     *
     * @param[in] layout   Layout position of each sensor by name, y pointing up.
     * @param[in] bad      Names of the bad sensors.
     */
    void set_layout(const std::map<std::string, Point>& layout, const std::vector<std::string>& bad);

    //=========================================================================================================
    /**
     * @param[in] kind   Channel kind to show, FIFFV_MEG_CH or FIFFV_EEG_CH; applied by set_selectable().
     */
    void set_channel_kind(int kind);

    //=========================================================================================================
    /**
     * @return The channel kind shown.
     */
    int channel_kind() const
    {
        return m_kind;
    }

    //=========================================================================================================
    /**
     * Shows the sensors of the channel kind and lets only those in @p names be picked.
     *
     * @param[in] names   Names of the sensors that can be selected.
     */
    void set_selectable(const std::vector<std::string>& names);

    //=========================================================================================================
    /**
     * Selects the sensors named in @p names and deselects all others.
     *
     * @param[in] names   Names of the sensors to select.
     */
    void select_names(const std::vector<std::string>& names);

    //=========================================================================================================
    /**
     * Highlights the sensors named in @p names with an orange ring.
     *
     * @param[in] names   Names of the sensors to highlight.
     */
    void highlight_names(const std::vector<std::string>& names);

    //=========================================================================================================
    /**
     * Removes all sensors.
     */
    void clear();

    //=========================================================================================================
    /**
     * @return The names of the selected sensors, in layout order.
     */
    std::vector<std::string> selected_names() const;

    //=========================================================================================================
    /**
     * @return The selected sensors, or all shown sensors if none is selected.
     */
    std::vector<SensorLayout::Sensor> selection() const;

    //=========================================================================================================
    /**
     * @return The sensors and their state.
     */
    const SensorLayout& sensor_layout() const
    {
        return m_layout;
    }

    Rect bounds() const override;
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const override;
    bool press(Point point, Modifier modifiers) override;
    bool band_selection_enabled() const override;
    void band_selected(const Rect& band, Modifier modifiers) override;

private:
    void selection_changed() const;

    SensorLayout m_layout;                     /**< The sensors and their selection. */
    int m_kind;                                /**< Channel kind shown. */
    std::function<void()> m_selectionCallback; /**< Called when the selection changed. */
};

} // namespace mne::viz

#endif // VIZ_SENSOR_LAYOUT_FIGURE_H
