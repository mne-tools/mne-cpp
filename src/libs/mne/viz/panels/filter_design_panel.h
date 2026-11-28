//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_design_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FilterDesignPanel designs an FIR filter kernel and shows its magnitude response.
 */

#ifndef VIZ_FILTER_DESIGN_PANEL_H
#define VIZ_FILTER_DESIGN_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "../figures/filter_response_figure.h"
#include "panel.h"
#include <mne/base/error.h>
#include <mne/dsp/filterkernel.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel that designs an FIR filter kernel from its type, pass band, number of taps, transition band
 * and design method, and shows its magnitude response in a FilterResponseFigure.
 *
 * The channel type tells the host which channels to filter. The panel writes the coefficients of the current
 * kernel and its response plot (SVG or PNG) to files the user picks, and designs a kernel like one loaded from a
 * coefficient file. The settings keep the keys of the former Qt view; the design method is stored as its index
 * in mne::FilterKernel::design_methods.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT FilterDesignPanel : public Panel
{
public:
    /** What the panel reports when the user changed the design. */
    struct Callbacks
    {
        std::function<void(const mne::FilterKernel& filter)> filter_changed; /**< A new kernel was designed. */
        std::function<void(const std::string& type)> channel_type_changed;   /**< The channel type changed. */
        std::function<void(double from)> from_changed;                       /**< The lower edge in Hz, on each design. */
        std::function<void(double to)> to_changed;                           /**< The upper edge in Hz, on each design. */
    };

    static constexpr const char* type_key = "filterType";                  /**< Key of the filter type. */
    static constexpr const char* from_key = "filterFrom";                  /**< Key of the lower edge in Hz. */
    static constexpr const char* to_key = "filterTo";                      /**< Key of the upper edge in Hz. */
    static constexpr const char* taps_key = "filterOrder";                 /**< Key of the number of taps. */
    static constexpr const char* transition_key = "filterTransition";      /**< Key of the transition band in Hz. */
    static constexpr const char* method_key = "filterDesignMethod";        /**< Key of the design method. */
    static constexpr const char* channel_type_key = "filterChannelType";   /**< Key of the channel type. */
    static constexpr const char* export_coefficients_key = "exportFilter"; /**< Key of the coefficient file to write. */
    static constexpr const char* load_filter_key = "loadFilter";           /**< Key of the coefficient file to read. */
    static constexpr const char* export_plot_key = "exportPlot";           /**< Key of the plot file to write. */

    //=========================================================================================================
    /**
     * Constructs the panel with a 1 to 40 Hz band pass of 128 taps, a 0.1 Hz transition band and the cosine
     * method at 600 Hz, applied to all channels.
     */
    FilterDesignPanel();

    ~FilterDesignPanel() override;

    //=========================================================================================================
    /**
     * @param[in] callbacks   What to call on changes.
     */
    void set_callbacks(Callbacks callbacks)
    {
        m_callbacks = std::move(callbacks);
    }

    //=========================================================================================================
    /**
     * Designs the kernel for a new sampling frequency, limiting the edges to its Nyquist frequency; silent.
     *
     * @param[in] sfreq   Sampling frequency in Hz; ignored if not positive.
     */
    void set_sampling_rate(double sfreq);

    //=========================================================================================================
    /**
     * Limits the number of taps to 16 up to @p taps, rounded down to even; silent.
     *
     * @param[in] taps   The largest number of taps.
     */
    void set_max_taps(int taps);

    //=========================================================================================================
    /**
     * @param[in] from   Lower edge in Hz; designs the kernel without a report.
     */
    void set_from(double from);

    //=========================================================================================================
    /**
     * @param[in] to   Upper edge in Hz; designs the kernel without a report.
     */
    void set_to(double to);

    //=========================================================================================================
    /**
     * @param[in] type   Channel type to filter: "All", "MEG", "EEG", "ECG", "EOG" or "EMG"; silent.
     */
    void set_channel_type(const std::string& type);

    /** @return The lower edge in Hz. */
    double from() const;

    /** @return The upper edge in Hz. */
    double to() const;

    /** @return The number of taps, rounded down to even. */
    int taps() const;

    /** @return The channel type to filter. */
    std::string channel_type() const;

    /** @return The sampling frequency in Hz. */
    double sampling_rate() const
    {
        return m_sfreq;
    }

    //=========================================================================================================
    /**
     * @return The current kernel.
     */
    const mne::FilterKernel& filter() const
    {
        return m_filter;
    }

    //=========================================================================================================
    /**
     * @return The figure of the magnitude response.
     */
    FilterResponseFigure& response() const
    {
        return *m_response;
    }

    //=========================================================================================================
    /**
     * Takes type, edges, taps, transition band and method of the filter in @p path and designs the kernel with
     * them at the current sampling frequency, reporting it like a user edit.
     *
     * @param[in] path   Coefficient file written by mne::FilterIO.
     *
     * @return Nothing, or why the file could not be read.
     */
    Expected<void> load_filter(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Writes the coefficients of the current kernel, named after its type, edges and sampling frequency.
     *
     * @param[in] path   Text file to write.
     *
     * @return Nothing, or why the file could not be written.
     */
    Expected<void> export_coefficients(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Writes the response plot as SVG, or as PNG if @p path ends in ".png".
     *
     * @param[in] path     File to write.
     * @param[in] width    Width of the plot in pixels.
     * @param[in] height   Height of the plot in pixels.
     *
     * @return Nothing, or why the file could not be written.
     */
    Expected<void> export_plot(const std::filesystem::path& path, double width = 800.0, double height = 500.0) const;

    //=========================================================================================================
    /**
     * Restores the settings without reports; the design method is read as its index.
     *
     * @param[in] store   Settings to read.
     * @param[in] path    Settings path of the host.
     */
    void load(const SettingsStore& store, const std::string& path) override;

    //=========================================================================================================
    /**
     * Stores the settings; the design method as its index.
     *
     * @param[in, out] store   Settings to write.
     * @param[in]      path    Settings path of the host.
     */
    void save(SettingsStore& store, const std::string& path) const override;

private:
    void changed(const std::string& key);
    void design(bool report);
    void update_type_fields();
    void report_status(const Expected<void>& result, const std::string& done);

    Callbacks m_callbacks;                            /**< What to call on changes. */
    mne::FilterKernel m_filter;                       /**< The current kernel. */
    std::shared_ptr<FilterResponseFigure> m_response; /**< Magnitude response of the kernel. */
    double m_sfreq = 600.0;                           /**< Sampling frequency in Hz. */
    int m_listener = 0;                               /**< Listener on the form. */
    bool m_updating = false;                          /**< Whether the panel sets fields itself. */
};

} // namespace mne::viz

#endif // VIZ_FILTER_DESIGN_PANEL_H
