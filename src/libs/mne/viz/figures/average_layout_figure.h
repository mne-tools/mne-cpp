//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     average_layout_figure.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AverageLayoutFigure: the averages of each channel in a tile at its sensor position, as an interactive
 *           figure.
 */

#ifndef VIZ_AVERAGE_LAYOUT_FIGURE_H
#define VIZ_AVERAGE_LAYOUT_FIGURE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "average_layout.h"
#include "scene_figure.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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
 * @brief Channels as tiles of their averages at their layout positions; pressing a tile enlarges its name.
 *
 * Each average is scaled by the channel's entry in the scale map, coloured by its comment and drawn only if its
 * comment is active.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT AverageLayoutFigure : public SceneFigure
{
public:
    /** One average of a channel: values data[0], data[stride], ..., data[(samples - 1) stride]. */
    struct Average
    {
        std::string comment;          /**< Name of the average, the key of the colour and activation maps. */
        const double* data = nullptr; /**< First value. */
        int samples = 0;              /**< Number of values. */
        int stride = 1;               /**< Distance between values, the channel count of a column-major matrix. */
    };

    /** A channel and its averages. */
    struct Channel
    {
        std::string name;              /**< Name, drawn above the tile. */
        int number = -1;               /**< Row in the evoked data, -1 if unknown. */
        Point position;                /**< Position in layout units. */
        int kind = 0;                  /**< Channel kind, which picks the scale. */
        int unit = 0;                  /**< Channel unit, which picks the scale. */
        bool bad = false;              /**< Bad channels are drawn faint. */
        int first_sample = 0;          /**< First sample relative to the stimulus, which places the stimulus line. */
        std::vector<Average> averages; /**< The averages to draw. */
    };

    //=========================================================================================================
    /**
     * Sets the channels.
     *
     * @param[in] channels   The channels.
     */
    void set_channels(std::vector<Channel> channels);

    //=========================================================================================================
    /**
     * @return The channels.
     */
    const std::vector<Channel>& channels() const
    {
        return m_channels;
    }

    //=========================================================================================================
    /**
     * Gives access to the channels to change their data; call refresh() afterwards.
     *
     * @return The channels.
     */
    std::vector<Channel>& channels()
    {
        return m_channels;
    }

    //=========================================================================================================
    /**
     * Tells the hosts that the channels changed through channels().
     */
    void refresh();

    //=========================================================================================================
    /**
     * @param[in] scales   Value at the top edge of a tile, by channel kind, or by unit for MEG.
     */
    void set_scale_map(std::map<int, float> scales);

    //=========================================================================================================
    /**
     * @param[in] activation   Whether each average, by comment, is drawn; averages missing here are.
     */
    void set_average_activation(std::map<std::string, bool> activation);

    //=========================================================================================================
    /**
     * @param[in] colors   Colour of each average, by comment; averages missing here take the signal colour.
     */
    void set_average_colors(std::map<std::string, Color> colors);

    //=========================================================================================================
    /**
     * @param[in] color   Colour of averages without their own colour.
     */
    void set_signal_color(Color color);

    //=========================================================================================================
    /**
     * @return The colour of averages without their own colour.
     */
    Color signal_color() const
    {
        return m_signalColor;
    }

    //=========================================================================================================
    /**
     * @return The tiles as they are drawn.
     */
    AverageLayout average_layout() const;

    Rect bounds() const override;
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const override;
    bool press(Point point, Modifier modifiers) override;
    void release(Point point) override;

private:
    std::vector<Channel> m_channels;          /**< The channels. */
    std::map<int, float> m_scales;            /**< Value at the top edge, by kind or unit. */
    std::map<std::string, bool> m_activation; /**< Whether each average is drawn. */
    std::map<std::string, Color> m_colors;    /**< Colour of each average. */
    Color m_signalColor{0, 80, 180};          /**< Colour of averages without their own. */
    int m_pressedTile = -1;                   /**< Tile whose name is enlarged. */
};

} // namespace mne::viz

#endif // VIZ_AVERAGE_LAYOUT_FIGURE_H
