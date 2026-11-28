//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     average_layout_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AverageLayoutPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "average_layout_panel.h"
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageLayoutPanel::AverageLayoutPanel()
: Panel("AverageLayoutView", "Average Layout")
, m_layout(std::make_shared<AverageLayoutFigure>())
{
    set_figure(m_layout);
}

//=============================================================================================================

AverageLayoutPanel::~AverageLayoutPanel() = default;

//=============================================================================================================

void AverageLayoutPanel::set_info(std::shared_ptr<const FiffInfo> info)
{
    m_info = std::move(info);
    update_averages();
}

//=============================================================================================================

void AverageLayoutPanel::set_channel_lookup(std::function<int(const std::string&)> lookup)
{
    m_lookup = std::move(lookup);
    update_averages();
}

//=============================================================================================================

void AverageLayoutPanel::set_selection(std::vector<AverageLayoutFigure::Channel> channels)
{
    m_layout->set_channels(std::move(channels));
    update_averages();
}

//=============================================================================================================

void AverageLayoutPanel::set_evoked(EvokedAverages evoked)
{
    m_evoked = std::move(evoked);
    update_averages();
}

//=============================================================================================================

void AverageLayoutPanel::clear_evoked()
{
    m_evoked.reset();
    update_averages();
}

//=============================================================================================================

void AverageLayoutPanel::set_scale_map(std::map<int, float> scales)
{
    m_layout->set_scale_map(std::move(scales));
}

//=============================================================================================================

void AverageLayoutPanel::set_average_colors(std::map<std::string, Color> colors)
{
    m_colors = std::move(colors);
    m_layout->set_average_colors(m_colors);
}

//=============================================================================================================

void AverageLayoutPanel::set_single_average_color(Color color)
{
    for (auto& [comment, average] : m_colors) {
        average = color;
    }
    m_layout->set_average_colors(m_colors);
    m_layout->set_signal_color(color);
}

//=============================================================================================================

void AverageLayoutPanel::set_average_activation(std::map<std::string, bool> activation)
{
    m_activation = std::move(activation);
    m_layout->set_average_activation(m_activation);
}

//=============================================================================================================

void AverageLayoutPanel::update_averages()
{
    for (AverageLayoutFigure::Channel& channel : m_layout->channels()) {
        channel.averages.clear();
        if (!m_evoked || !m_info) {
            continue;
        }
        if (m_lookup) {
            channel.number = m_lookup(channel.name);
        }
        const int number = channel.number;
        if (number < 0 || number >= static_cast<int>(m_info->chs.size())) {
            continue;
        }
        const auto row = static_cast<std::size_t>(number);
        channel.kind = m_info->chs[row].kind;
        channel.unit = m_info->chs[row].unit;
        channel.first_sample = m_evoked->first_sample;
        channel.bad = row < m_evoked->bad.size() && m_evoked->bad[row];
        for (const AverageLayoutFigure::Average& average : m_evoked->averages) {
            AverageLayoutFigure::Average shown = average;
            if (shown.data) {
                shown.data += number;
            }
            channel.averages.push_back(std::move(shown));
        }
    }
    m_layout->refresh();
}
