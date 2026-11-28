//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     spectrum_figure.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SpectrumFigure: the frequency spectrum of each channel in a row of its own, with a read-out under the
 *           pointer.
 */

#ifndef VIZ_SPECTRUM_FIGURE_H
#define VIZ_SPECTRUM_FIGURE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "../models/spectrum_table.h"
#include "scene_figure.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <optional>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Spectra of the selected channels, one row each, between the frequency bounds.
 *
 * The figure is not zoomable: it lays its rows out to the view width and the host scrolls them. Each row shows
 * the channel name, a dotted frequency grid with labels and the spectrum scaled to its range between the bounds.
 * The row under the pointer is drawn red, with a vertical line and the value and frequency of the nearest bin; a
 * double click freezes the spectra, which are then drawn darker, and a second one thaws them.
 *
 * @snippet ex_scene_sdl/main.cpp spectrum_panel_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SpectrumFigure : public SceneFigure
{
public:
    static constexpr double row_height = 140.0; /**< Height of a channel's row. */
    static constexpr double name_width = 80.0;  /**< Width of the channel names left of the spectra. */

    /** Value and frequency of the bin under the pointer. */
    struct Readout
    {
        int row = -1;           /**< Row under the pointer. */
        int bin = -1;           /**< Nearest frequency bin. */
        double value = 0.0;     /**< Value of the bin. */
        double frequency = 0.0; /**< Frequency of the bin in Hz. */
    };

    //=========================================================================================================
    /**
     * Shows the channels of @p info, all selected, without data.
     *
     * @param[in] info    Channels and sampling frequency.
     * @param[in] scale   Spacing of the frequencies.
     */
    void set_info(std::shared_ptr<FiffInfo> info, SpectrumTable::Scale scale);

    //=========================================================================================================
    /**
     * @param[in] data   Spectrum of each channel (rows) from 0 Hz to the Nyquist frequency (columns).
     */
    void set_data(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
     * Shows only the frequencies between @p lower and @p upper.
     *
     * @param[in] lower   Lower bound in Hz.
     * @param[in] upper   Upper bound in Hz.
     */
    void set_bounds(float lower, float upper);

    //=========================================================================================================
    /**
     * Shows only some channels.
     *
     * @param[in] channels   Indices of the channels, in row order.
     */
    void select(const std::vector<int>& channels);

    //=========================================================================================================
    /**
     * Shows all channels again.
     */
    void reset_selection();

    //=========================================================================================================
    /**
     * Freezes the spectra shown, or thaws them.
     */
    void toggle_frozen();

    //=========================================================================================================
    /**
     * @return The spectra, the selection and the frequency axis.
     */
    const SpectrumTable& table() const
    {
        return m_table;
    }

    //=========================================================================================================
    /**
     * @return The read-out under the pointer, if it is over a spectrum with data.
     */
    std::optional<Readout> readout() const;

    Rect bounds() const override;
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const override;
    bool zoomable() const override;
    void resize(double width, double height) override;
    void hover(std::optional<Point> point) override;
    bool double_click(Point point) override;

private:
    std::vector<std::pair<int, double>> grid() const;
    double bin_frequency(int bin) const;
    void draw_row(Canvas& canvas, const TextMetrics& metrics, const Font& font, int row) const;

    SpectrumTable m_table;                                       /**< Spectra, selection and frequency axis. */
    SpectrumTable::Scale m_scale = SpectrumTable::Scale::Linear; /**< Spacing of the frequencies. */
    double m_width = 0.0;                                        /**< Width of the view. */
    std::optional<Point> m_pointer;                              /**< Pointer over the figure. */
};

} // namespace mne::viz

#endif // VIZ_SPECTRUM_FIGURE_H
