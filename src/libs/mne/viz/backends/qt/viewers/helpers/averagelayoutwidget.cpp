//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     averagelayoutwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the AverageLayoutWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagelayoutwidget.h"
#include "selectionitem.h"
#include "../scalingview.h"
#include "../../plots/helpers/scenepainter.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;

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

AverageLayoutWidget::AverageLayoutWidget(QWidget* parent)
: SceneViewport(parent)
, m_signalColor(0, 80, 180)
{
}

//=============================================================================================================

std::vector<AverageLayoutWidget::Channel> AverageLayoutWidget::channelsFrom(const SelectionItem& selection)
{
    std::vector<Channel> channels;
    for (std::size_t i = 0; i < selection.m_sChannelName.size(); ++i) {
        Channel channel;
        channel.name = selection.m_sChannelName[i];
        channel.number = i < selection.m_iChannelNumber.size() ? selection.m_iChannelNumber[i] : -1;
        channel.position = i < selection.m_qpChannelPosition.size() ? selection.m_qpChannelPosition[i] : QPointF();
        channel.kind = i < selection.m_iChannelKind.size() ? selection.m_iChannelKind[i] : 0;
        channel.unit = i < selection.m_iChannelUnit.size() ? selection.m_iChannelUnit[i] : 0;
        channels.push_back(std::move(channel));
    }
    return channels;
}

//=============================================================================================================

void AverageLayoutWidget::setChannels(std::vector<Channel> channels)
{
    m_channels = std::move(channels);
    m_pressedTile = -1;
    contentsChanged();
}

//=============================================================================================================

const std::vector<AverageLayoutWidget::Channel>& AverageLayoutWidget::channels() const
{
    return m_channels;
}

//=============================================================================================================

std::vector<AverageLayoutWidget::Channel>& AverageLayoutWidget::channels()
{
    return m_channels;
}

//=============================================================================================================

void AverageLayoutWidget::setScaleMap(const std::map<int, float>& scaleMap)
{
    m_scaleMap = scaleMap;
    update();
}

//=============================================================================================================

void AverageLayoutWidget::setAverageActivation(const std::map<std::string, bool>& activation)
{
    m_activation = activation;
    update();
}

//=============================================================================================================

void AverageLayoutWidget::setAverageColors(const std::map<std::string, QColor>& colors)
{
    m_colors = colors;
    update();
}

//=============================================================================================================

void AverageLayoutWidget::setSignalColor(const QColor& color)
{
    m_signalColor = color;
    update();
}

//=============================================================================================================

QColor AverageLayoutWidget::signalColor() const
{
    return m_signalColor;
}

//=============================================================================================================

mne::viz::AverageLayout AverageLayoutWidget::averageLayout() const
{
    std::vector<viz::AverageTile> tiles;
    tiles.reserve(m_channels.size());
    for (std::size_t i = 0; i < m_channels.size(); ++i) {
        const Channel& channel = m_channels[i];
        viz::AverageTile tile;
        tile.box = viz::AverageLayout::box_at({channel.position.x(), channel.position.y()});
        tile.name = channel.name;
        tile.name_size = static_cast<int>(i) == m_pressedTile ? kPressedNameSize : kNameSize;
        tile.bad = channel.bad;
        tile.max_value = getScalingValue(m_scaleMap, channel.kind, channel.unit);
        tile.stim_sample = channel.firstSample;
        tile.columns = channel.averages.empty() ? 0 : channel.averages.front().samples;
        for (const Average& average : channel.averages) {
            // Averages missing from the activation map are drawn
            if (const auto active = m_activation.find(average.comment); active != m_activation.end() && !active->second) {
                continue;
            }
            viz::AverageTile::Trace trace;
            const auto color = m_colors.find(average.comment);
            trace.color = toSceneColor(color != m_colors.end() ? color->second : m_signalColor);
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
    viz::AverageLayout layout;
    layout.set_tiles(std::move(tiles));
    return layout;
}

//=============================================================================================================

mne::viz::Rect AverageLayoutWidget::sceneBounds() const
{
    viz::Rect united;
    for (const Channel& channel : m_channels) {
        const viz::Rect area = viz::AverageLayout::box_at({channel.position.x(), channel.position.y()}).adjusted(0, -kNameSize, 0, 0);
        united = united.empty() ? area : united.united(area);
    }
    return united;
}

//=============================================================================================================

void AverageLayoutWidget::drawScene(viz::Canvas& canvas) const
{
    averageLayout().draw(canvas, toSceneFont(font()));
}

//=============================================================================================================

bool AverageLayoutWidget::scenePressed(viz::Point point, Qt::KeyboardModifiers /*modifiers*/)
{
    for (std::size_t i = m_channels.size(); i-- > 0;) {
        if (viz::AverageLayout::box_at({m_channels[i].position.x(), m_channels[i].position.y()}).contains(point)) {
            m_pressedTile = static_cast<int>(i);
            update();
            return true;
        }
    }
    return false;
}

//=============================================================================================================

void AverageLayoutWidget::sceneReleased(viz::Point /*point*/)
{
    m_pressedTile = -1;
    update();
}
