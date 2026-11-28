//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     spectrum_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SpectrumPanel shows the frequency spectrum of each channel.
 */

#ifndef VIZ_SPECTRUM_PANEL_H
#define VIZ_SPECTRUM_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "../figures/spectrum_figure.h"
#include "panel.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <memory>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel with the spectra of the channels, one row each, as a SpectrumFigure, and the frequency band
 * shown as sliders between 0 Hz and the Nyquist frequency.
 *
 * Spectra that arrive before init() are dropped, as mne_scan may send them before the channels are known.
 *
 * @snippet ex_scene_sdl/main.cpp spectrum_panel_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SpectrumPanel : public Panel
{
public:
    //=========================================================================================================
    /**
     * Constructs a panel without channels.
     */
    SpectrumPanel();

    ~SpectrumPanel() override;

    //=========================================================================================================
    /**
     * @return The figure of the spectra.
     */
    SpectrumFigure& spectrum() const
    {
        return *m_spectrum;
    }

    //=========================================================================================================
    /**
     * Shows the channels of @p info, without spectra until the next add_data().
     *
     * @param[in] info    Channels and sampling frequency.
     * @param[in] scale   Spacing of the frequencies.
     */
    void init(std::shared_ptr<FiffInfo> info, SpectrumTable::Scale scale);

    //=========================================================================================================
    /**
     * @param[in] data   Spectrum of each channel (rows) from 0 Hz to the Nyquist frequency (columns); dropped
     *                   before init().
     */
    void add_data(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
     * Shows only the frequencies between @p lower and @p upper, applied once data have arrived; the bounds are
     * the panel's settings, without a report.
     *
     * @param[in] lower   Lower bound in Hz.
     * @param[in] upper   Upper bound in Hz.
     */
    void set_boundaries(float lower, float upper);

    //=========================================================================================================
    /**
     * @param[in] callback   Called with the bounds in Hz when the user changed them.
     *
     * @since 3.0.0
     */
    void set_bounds_callback(std::function<void(float lower, float upper)> callback)
    {
        m_boundsCallback = std::move(callback);
    }

    /** @return The lower bound in Hz. */
    float lower_bound() const;

    /** @return The upper bound in Hz. */
    float upper_bound() const;

private:
    void apply_bounds();

    std::shared_ptr<SpectrumFigure> m_spectrum;         /**< The spectra. */
    std::function<void(float, float)> m_boundsCallback; /**< Called when the user changed the bounds. */
    int m_listener = 0;                                 /**< Listener on the form. */
    bool m_updating = false;                            /**< Whether the panel sets the bounds itself. */
    bool m_boundsApplied = false;                       /**< Whether the figure has the bounds of the form. */
};

} // namespace mne::viz

#endif // VIZ_SPECTRUM_PANEL_H
