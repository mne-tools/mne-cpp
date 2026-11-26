//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     spectrum_table.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SpectrumTable is the view model of a frequency spectrum viewer: channel rows, spectra and frequency axis.
 *
 * A spectrum viewer shows one power spectrum per selected channel over a frequency axis that is linear or
 * logarithmic and can be cut to a frequency band. SpectrumTable holds the latest spectra, the channel selection,
 * a frozen copy and the normalised frequency axis, without a GUI toolkit; disp::FrequencySpectrumModel adapts it
 * to a Qt item model.
 */

#ifndef VIEWMODELS_SPECTRUM_TABLE_H
#define VIEWMODELS_SPECTRUM_TABLE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "viewmodels_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viewmodels
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

namespace mne::viewmodels
{

//=============================================================================================================
/**
 * @brief Spectra, channel selection and frequency axis of a spectrum viewer.
 *
 * @since 3.0.0
 */
class VIEWMODELSSHARED_EXPORT SpectrumTable
{
public:
    //=========================================================================================================
    /**
     * @brief Spacing of the frequency axis.
     */
    enum class Scale
    {
        Linear, /**< Frequencies evenly spaced. */
        Log     /**< log10(f + 1) evenly spaced. */
    };

    //=========================================================================================================
    /**
     * Sets the measurement info and selects all its channels.
     *
     * @param[in] info  Measurement info; its sampling frequency defines the frequency axis.
     *
     * @since 3.0.0
     */
    void set_info(std::shared_ptr<FiffInfo> info);

    //=========================================================================================================
    /**
     * @return The measurement info.
     *
     * @since 3.0.0
     */
    const std::shared_ptr<FiffInfo>& info() const
    {
        return m_info;
    }

    //=========================================================================================================
    /**
     * Sets the spacing of the frequency axis used from the next spectrum with a new length on.
     *
     * @param[in] scale     Spacing of the frequency axis.
     *
     * @since 3.0.0
     */
    void set_scale(Scale scale)
    {
        m_scale = scale;
    }

    //=========================================================================================================
    /**
     * Sets the latest spectra. A new number of frequency bins rebuilds the frequency axis and resets the band.
     *
     * @param[in] data  Spectra, channels by frequency bins from 0 Hz to the Nyquist frequency.
     *
     * @since 3.0.0
     */
    void set_data(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
     * @return Number of selected channels, i.e. rows.
     *
     * @since 3.0.0
     */
    int row_count() const
    {
        return static_cast<int>(m_selection.size());
    }

    //=========================================================================================================
    /**
     * @param[in] row   Row of the table.
     *
     * @return The channel shown in @p row, or 0 when @p row is not selected.
     *
     * @since 3.0.0
     */
    int channel(int row) const;

    //=========================================================================================================
    /**
     * @param[in] row   Row of the table.
     *
     * @return Name of the channel shown in @p row, empty without info.
     *
     * @since 3.0.0
     */
    std::string channel_name(int row) const;

    //=========================================================================================================
    /**
     * @param[in] row   Row of the table.
     *
     * @return The shown spectrum of the channel in @p row (the frozen one while frozen), or nothing before data
     *         for that channel arrived.
     *
     * @since 3.0.0
     */
    std::optional<Eigen::RowVectorXd> spectrum(int row) const;

    //=========================================================================================================
    /**
     * Shows the given channels, skipping channels the info does not have.
     *
     * @param[in] channels  Channel indices in display order.
     *
     * @since 3.0.0
     */
    void select(const std::vector<int>& channels);

    //=========================================================================================================
    /**
     * Shows all channels of the info.
     *
     * @since 3.0.0
     */
    void reset_selection();

    //=========================================================================================================
    /**
     * @return Map from row to channel index.
     *
     * @since 3.0.0
     */
    const std::map<int, int>& selection() const
    {
        return m_selection;
    }

    //=========================================================================================================
    /**
     * Freezes or unfreezes the shown spectra; freezing keeps a copy of the current ones.
     *
     * @since 3.0.0
     */
    void toggle_frozen();

    //=========================================================================================================
    /**
     * @return Whether the shown spectra are frozen.
     *
     * @since 3.0.0
     */
    bool frozen() const
    {
        return m_frozen;
    }

    //=========================================================================================================
    /**
     * Cuts the frequency axis to a band and renormalises it to [0, 1] over the band.
     *
     * @param[in] lower     Lower band edge in Hz.
     * @param[in] upper     Upper band edge in Hz.
     *
     * @return False when no spectrum arrived yet, so there is no axis to cut.
     *
     * @since 3.0.0
     */
    bool set_bounds(float lower, float upper);

    //=========================================================================================================
    /**
     * @return Normalised frequency axis, 0 to 1 over all bins.
     *
     * @since 3.0.0
     */
    const Eigen::RowVectorXd& freq_scale() const
    {
        return m_freqScale;
    }

    //=========================================================================================================
    /**
     * @return Frequency axis normalised to 0 to 1 over the band.
     *
     * @since 3.0.0
     */
    const Eigen::RowVectorXd& freq_scale_bound() const
    {
        return m_freqScaleBound;
    }

    //=========================================================================================================
    /**
     * @return Bin of the lower band edge.
     *
     * @since 3.0.0
     */
    int lower_bin() const
    {
        return m_lowerBin;
    }

    //=========================================================================================================
    /**
     * @return Bin of the upper band edge.
     *
     * @since 3.0.0
     */
    int upper_bin() const
    {
        return m_upperBin;
    }

    //=========================================================================================================
    /**
     * @return Number of frequency bins of the latest spectra.
     *
     * @since 3.0.0
     */
    int bins() const
    {
        return static_cast<int>(m_data.cols());
    }

    //=========================================================================================================
    /**
     * @return Number of channels of the latest spectra.
     *
     * @since 3.0.0
     */
    int data_rows() const
    {
        return static_cast<int>(m_data.rows());
    }

private:
    std::shared_ptr<FiffInfo> m_info;
    std::map<int, int> m_selection;
    Eigen::RowVectorXd m_freqScale;
    Eigen::RowVectorXd m_freqScaleBound;
    Eigen::MatrixXd m_data;
    Eigen::MatrixXd m_frozenData;
    Scale m_scale = Scale::Linear;
    int m_lowerBin = 0;
    int m_upperBin = 0;
    bool m_frozen = false;
    bool m_initialized = false;
};

} // namespace mne::viewmodels

#endif // VIEWMODELS_SPECTRUM_TABLE_H
