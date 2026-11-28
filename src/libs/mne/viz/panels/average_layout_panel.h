//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     average_layout_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AverageLayoutPanel shows the averages of the selected channels at their sensor positions.
 */

#ifndef VIZ_AVERAGE_LAYOUT_PANEL_H
#define VIZ_AVERAGE_LAYOUT_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "../figures/average_layout_figure.h"
#include "panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
} // namespace mne

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief The averages of an evoked set: per average the values of the first channel in a column-major
 * channels x samples matrix, the other channels following at the channel count.
 *
 * @snippet ex_scene_sdl/main.cpp average_layout_panel_usage
 *
 * @since 3.0.0
 */
struct EvokedAverages
{
    std::vector<AverageLayoutFigure::Average> averages; /**< Each average from channel 0; its stride is the channel count. */
    int first_sample = 0;                               /**< First sample relative to the stimulus. */
    std::vector<bool> bad;                              /**< Whether each channel, by row, is bad. */
};

//=============================================================================================================
/**
 * @brief Tool panel with the averages of the selected channels in tiles at their sensor positions.
 *
 * The selection gives the channels and their layout positions; the evoked data their averages. Channel rows come
 * from the selection, or by name from the channel lookup. Kinds and units, which pick each tile's scale, come
 * from the measurement info.
 *
 * @snippet ex_scene_sdl/main.cpp average_layout_panel_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT AverageLayoutPanel : public Panel
{
public:
    //=========================================================================================================
    /**
     * Constructs a panel without channels.
     */
    AverageLayoutPanel();

    ~AverageLayoutPanel() override;

    //=========================================================================================================
    /**
     * @return The figure of the tiles.
     */
    AverageLayoutFigure& layout() const
    {
        return *m_layout;
    }

    //=========================================================================================================
    /**
     * @param[in] info   Kinds and units of the channels by row.
     */
    void set_info(std::shared_ptr<const FiffInfo> info);

    //=========================================================================================================
    /**
     * @param[in] lookup   Row of a channel by its layout name, -1 if unknown; when set, it gives the rows instead
     *                     of the selection.
     */
    void set_channel_lookup(std::function<int(const std::string&)> lookup);

    //=========================================================================================================
    /**
     * Shows the channels of a selection, at their layout positions.
     *
     * @param[in] channels   The channels, without averages.
     */
    void set_selection(std::vector<AverageLayoutFigure::Channel> channels);

    //=========================================================================================================
    /**
     * Shows the averages of an evoked set; the data must stay valid until the next call or clear_evoked().
     *
     * @param[in] evoked   The averages.
     */
    void set_evoked(EvokedAverages evoked);

    //=========================================================================================================
    /**
     * Removes the averages, keeping the channels.
     */
    void clear_evoked();

    //=========================================================================================================
    /**
     * @param[in] scales   Value at the top edge of a tile, by channel kind, or by unit for MEG.
     */
    void set_scale_map(std::map<int, float> scales);

    //=========================================================================================================
    /**
     * @param[in] colors   Colour of each average, by comment.
     */
    void set_average_colors(std::map<std::string, Color> colors);

    //=========================================================================================================
    /**
     * @return The colour of each average, by comment.
     */
    const std::map<std::string, Color>& average_colors() const
    {
        return m_colors;
    }

    //=========================================================================================================
    /**
     * Gives all averages, and those without a colour of their own, the same colour.
     *
     * @param[in] color   The colour.
     */
    void set_single_average_color(Color color);

    //=========================================================================================================
    /**
     * @param[in] activation   Whether each average, by comment, is drawn.
     */
    void set_average_activation(std::map<std::string, bool> activation);

    //=========================================================================================================
    /**
     * @return Whether each average, by comment, is drawn.
     */
    const std::map<std::string, bool>& average_activation() const
    {
        return m_activation;
    }

private:
    void update_averages();

    std::shared_ptr<AverageLayoutFigure> m_layout;   /**< The tiles. */
    std::shared_ptr<const FiffInfo> m_info;          /**< Kinds and units by row. */
    std::function<int(const std::string&)> m_lookup; /**< Row by layout name. */
    std::optional<EvokedAverages> m_evoked;          /**< The averages shown. */
    std::map<std::string, Color> m_colors;           /**< Colour of each average. */
    std::map<std::string, bool> m_activation;        /**< Whether each average is drawn. */
};

} // namespace mne::viz

#endif // VIZ_AVERAGE_LAYOUT_PANEL_H
