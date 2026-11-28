//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     average_layout_figure.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AverageLayoutFigure class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "average_layout_figure.h"
#include "channel_scale.h"

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
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

// Point size of a tile's name, and of the name of the pressed tile
constexpr double kNameSize = 3.0;
constexpr double kPressedNameSize = 6.0;

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void AverageLayoutFigure::set_channels(std::vector<Channel> channels)
{
    m_channels = std::move(channels);
    m_pressedTile = -1;
    changed(Change::Bounds);
}

//=============================================================================================================

void AverageLayoutFigure::refresh()
{
    changed(Change::Bounds);
}

//=============================================================================================================

void AverageLayoutFigure::set_scale_map(std::map<int, float> scales)
{
    m_scales = std::move(scales);
    changed();
}

//=============================================================================================================

void AverageLayoutFigure::set_average_activation(std::map<std::string, bool> activation)
{
    m_activation = std::move(activation);
    changed();
}

//=============================================================================================================

void AverageLayoutFigure::set_average_colors(std::map<std::string, Color> colors)
{
    m_colors = std::move(colors);
    changed();
}

//=============================================================================================================

void AverageLayoutFigure::set_signal_color(Color color)
{
    m_signalColor = color;
    changed();
}

//=============================================================================================================

AverageLayout AverageLayoutFigure::average_layout() const
{
    std::vector<AverageTile> tiles;
    tiles.reserve(m_channels.size());
    for (std::size_t i = 0; i < m_channels.size(); ++i) {
        const Channel& channel = m_channels[i];
        AverageTile tile;
        tile.box = AverageLayout::box_at(channel.position);
        tile.name = channel.name;
        tile.name_size = static_cast<int>(i) == m_pressedTile ? kPressedNameSize : kNameSize;
        tile.bad = channel.bad;
        tile.max_value = channel_scale(m_scales, channel.kind, channel.unit);
        tile.stim_sample = channel.first_sample;
        tile.columns = channel.averages.empty() ? 0 : channel.averages.front().samples;
        for (const Average& average : channel.averages) {
            // Averages missing from the activation map are drawn
            if (const auto active = m_activation.find(average.comment); active != m_activation.end() && !active->second) {
                continue;
            }
            AverageTile::Trace trace;
            const auto color = m_colors.find(average.comment);
            trace.color = color != m_colors.end() ? color->second : m_signalColor;
            if (average.data) {
                trace.samples.reserve(static_cast<std::size_t>(average.samples));
                for (int s = 0; s < average.samples; ++s) {
                    trace.samples.push_back(average.data[static_cast<std::ptrdiff_t>(s) * average.stride]);
                }
            }
            tile.traces.push_back(std::move(trace));
        }
        tiles.push_back(std::move(tile));
    }
    AverageLayout layout;
    layout.set_tiles(std::move(tiles));
    return layout;
}

//=============================================================================================================

Rect AverageLayoutFigure::bounds() const
{
    Rect united;
    for (const Channel& channel : m_channels) {
        const Rect area = AverageLayout::box_at(channel.position).adjusted(0, -kNameSize, 0, 0);
        united = united.empty() ? area : united.united(area);
    }
    return united;
}

//=============================================================================================================

void AverageLayoutFigure::draw(Canvas& canvas, const TextMetrics& /*metrics*/, const Font& font) const
{
    average_layout().draw(canvas, font);
}

//=============================================================================================================

bool AverageLayoutFigure::press(Point point, Modifier /*modifiers*/)
{
    for (std::size_t i = m_channels.size(); i-- > 0;) {
        if (AverageLayout::box_at(m_channels[i].position).contains(point)) {
            m_pressedTile = static_cast<int>(i);
            changed();
            return true;
        }
    }
    return false;
}

//=============================================================================================================

void AverageLayoutFigure::release(Point /*point*/)
{
    m_pressedTile = -1;
    changed();
}
