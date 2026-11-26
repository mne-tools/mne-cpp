//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_raw_display.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    RtRawDisplay prepares streamed raw data for display: SSP, compensator, SPHARA, filter and triggers.
 *
 * A real-time raw viewer corrects every incoming block with the active SSP projectors and the CTF compensator,
 * detects trigger flanks on one stimulus channel, optionally reduces noise with SPHARA and filters a selection
 * of channels with FIR kernels. RtRawDisplay does all of that block by block and keeps the filter overlap
 * between blocks; the caller places the results into its display buffer. It has no GUI dependency; the Qt model
 * of the raw viewer (viz::qt::RtFiffRawViewModel) is an adapter around it.
 */

#ifndef DSP_RT_RAW_DISPLAY_H
#define DSP_RT_RAW_DISPLAY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../display_correction.h"
#include "../dsp_global.h"
#include "../filterkernel.h"
#include "rt_detect_trigger.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

class FiffInfo;

//=============================================================================================================
/**
 * @brief Prepares blocks of streamed raw data for a real-time viewer.
 *
 * @since 3.0.0
 */
class DSPSHARED_EXPORT RtRawDisplay
{
public:
    //=========================================================================================================
    /**
     * @brief One block after SSP and compensator, with the trigger flanks found in it.
     */
    struct Corrected
    {
        Eigen::MatrixXd data;               /**< Corrected data, channels by samples. */
        std::vector<TriggerFlank> triggers; /**< Flanks on the trigger channel; samples relative to the block. */
    };

    //=========================================================================================================
    /**
     * @brief One block after filtering.
     *
     * The rows in filtered_rows hold the filter output, which belongs filter_delay() samples before the block
     * start; its first zero_columns columns are the start-up transient of new kernels and are zero. The rows in
     * passthrough_rows are not filtered and belong to the block start itself.
     */
    struct Filtered
    {
        Eigen::MatrixXd data;              /**< Data, channels by samples of the block. */
        std::vector<int> filtered_rows;    /**< Rows holding filter output. */
        std::vector<int> passthrough_rows; /**< Rows holding the unfiltered input. */
        int zero_columns = 0;              /**< Leading zero columns of the filtered rows. */
    };

    //=========================================================================================================
    /**
     * Sets the measurement info: resets the SSP projector and compensator, the filter overlap, the filtered
     * channels and the SPHARA channel indices. The info is shared, so later changes of its projectors and bad
     * channels are picked up by the next update_projection().
     *
     * @param[in] info  Measurement info of the stream.
     *
     * @since 3.0.0
     */
    void set_info(std::shared_ptr<const FiffInfo> info);

    //=========================================================================================================
    /**
     * Rebuilds the SSP projector from the projectors and bad channels of the info.
     *
     * @since 3.0.0
     */
    void update_projection();

    //=========================================================================================================
    /**
     * Rebuilds the compensator for the compensation grade @p to; 0 deactivates it.
     *
     * @param[in] to    Target compensation grade.
     *
     * @since 3.0.0
     */
    void update_compensator(int to);

    //=========================================================================================================
    /**
     * @return The SSP projector and compensator.
     *
     * @since 3.0.0
     */
    const DisplayCorrection& correction() const
    {
        return m_correction;
    }

    //=========================================================================================================
    /**
     * Loads the SPHARA basis functions of VectorView, BabyMEG and EEG from @p directory. Missing files leave
     * the respective basis empty, so SPHARA stays the identity for that system.
     *
     * @param[in] directory     Directory with the SPHARA basis files.
     *
     * @since 3.0.0
     */
    void load_sphara_bases(const std::string& directory);

    //=========================================================================================================
    /**
     * Rebuilds the SPHARA operator for @p system ("VectorView", "BabyMEG" or "EEG").
     *
     * @param[in] system    Name of the sensor system.
     * @param[in] first     Number of basis functions of the first sensor group.
     * @param[in] second    Number of basis functions of the second sensor group (VectorView magnetometers).
     *
     * @since 3.0.0
     */
    void update_sphara_options(const std::string& system, int first, int second);

    //=========================================================================================================
    /**
     * Turns SPHARA on or off.
     *
     * @param[in] active    Whether SPHARA is applied.
     *
     * @since 3.0.0
     */
    void set_sphara_active(bool active)
    {
        m_spharaActive = active;
    }

    //=========================================================================================================
    /**
     * @param[in] rows  Number of channels of the data.
     *
     * @return Whether SPHARA is active and its operator matches @p rows channels.
     *
     * @since 3.0.0
     */
    bool applies_sphara(Eigen::Index rows) const;

    //=========================================================================================================
    /**
     * @return The SPHARA operator, channels by channels.
     *
     * @since 3.0.0
     */
    const Eigen::SparseMatrix<double>& sphara_operator() const
    {
        return m_sphara;
    }

    //=========================================================================================================
    /**
     * Sets the FIR kernels. The overlap is cleared and the start-up transient of the next block is dropped.
     *
     * @param[in] kernels   Filter kernels, applied one after the other.
     *
     * @since 3.0.0
     */
    void set_filters(std::vector<FilterKernel> kernels);

    //=========================================================================================================
    /**
     * Turns filtering on or off.
     *
     * @param[in] active    Whether the filters are applied.
     *
     * @since 3.0.0
     */
    void set_filter_active(bool active)
    {
        m_filterActive = active;
    }

    //=========================================================================================================
    /**
     * @return Whether kernels are set and filtering is on.
     *
     * @since 3.0.0
     */
    bool filtering() const
    {
        return m_filterActive && !m_kernels.empty();
    }

    //=========================================================================================================
    /**
     * @return The FIR kernels.
     *
     * @since 3.0.0
     */
    const std::vector<FilterKernel>& filters() const
    {
        return m_kernels;
    }

    //=========================================================================================================
    /**
     * @return The longest kernel order, at least 1 once kernels were set.
     *
     * @since 3.0.0
     */
    int max_filter_length() const
    {
        return m_maxFilterLength;
    }

    //=========================================================================================================
    /**
     * @return How many samples the filter output lags behind its input.
     *
     * @since 3.0.0
     */
    int filter_delay() const
    {
        return m_maxFilterLength / 2;
    }

    //=========================================================================================================
    /**
     * Selects the filtered channels: MEG, EEG, EOG, ECG and EMG channels whose name contains @p type, or all
     * of them for "All".
     *
     * @param[in] type  Channel name part or "All".
     *
     * @since 3.0.0
     */
    void set_filter_channel_type(const std::string& type);

    //=========================================================================================================
    /**
     * @return The channel name part that selects the filtered channels.
     *
     * @since 3.0.0
     */
    const std::string& filter_channel_type() const
    {
        return m_filterChannelType;
    }

    //=========================================================================================================
    /**
     * Clears the filter overlap, e.g. when the display is cleared.
     *
     * @since 3.0.0
     */
    void clear_filter_state();

    //=========================================================================================================
    /**
     * Sets the trigger detection.
     *
     * @param[in] active        Whether flanks are detected.
     * @param[in] channel       Row of the trigger channel.
     * @param[in] threshold     Detection threshold.
     *
     * @since 3.0.0
     */
    void set_trigger_detection(bool active, int channel, double threshold);

    //=========================================================================================================
    /**
     * @return Whether trigger flanks are detected.
     *
     * @since 3.0.0
     */
    bool trigger_detection_active() const
    {
        return m_triggerActive;
    }

    //=========================================================================================================
    /**
     * @return Row of the trigger channel.
     *
     * @since 3.0.0
     */
    int trigger_channel() const
    {
        return m_triggerChannel;
    }

    //=========================================================================================================
    /**
     * @return Trigger detection threshold.
     *
     * @since 3.0.0
     */
    double trigger_threshold() const
    {
        return m_triggerThreshold;
    }

    //=========================================================================================================
    /**
     * Applies SSP and compensator to @p block and detects trigger flanks on the uncorrected block.
     *
     * @param[in] block     Raw data, channels by samples.
     *
     * @return The corrected block and its trigger flanks.
     *
     * @since 3.0.0
     */
    Corrected correct(const Eigen::MatrixXd& block) const;

    //=========================================================================================================
    /**
     * Filters the selected channels of a corrected block with overlap-add across blocks.
     *
     * @param[in] corrected     Corrected data, channels by samples.
     *
     * @return The filtered block, or nothing when the block is shorter than the longest kernel.
     *
     * @since 3.0.0
     */
    std::optional<Filtered> filter(const Eigen::MatrixXd& corrected);

private:
    std::shared_ptr<const FiffInfo> m_info;
    DisplayCorrection m_correction;

    Eigen::MatrixXd m_spharaVVGrad;
    Eigen::MatrixXd m_spharaVVMag;
    Eigen::MatrixXd m_spharaBabyMEGInner;
    Eigen::MatrixXd m_spharaEEG;
    Eigen::VectorXi m_indicesVVGrad;
    Eigen::VectorXi m_indicesVVMag;
    Eigen::VectorXi m_indicesBabyMEGInner;
    Eigen::VectorXi m_indicesEEG;
    Eigen::SparseMatrix<double> m_sphara;
    bool m_spharaActive = false;

    std::vector<FilterKernel> m_kernels;
    std::vector<bool> m_filteredRows;
    std::string m_filterChannelType = "MEG";
    Eigen::MatrixXd m_overlap;
    int m_maxFilterLength = 128;
    bool m_filterActive = false;
    bool m_drawFilterFront = true;

    bool m_triggerActive = false;
    int m_triggerChannel = 0;
    double m_triggerThreshold = 0.01;
};

} // namespace mne

#endif // DSP_RT_RAW_DISPLAY_H
