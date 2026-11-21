//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     rt_averaging.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>
 * @since    0.1.3
 * @date     July, 2012
 * @brief    Streaming stimulus-locked averaging: the moving mean of the newest epochs of every event code.
 *
 * RtAveraging cuts an epoch around every onset on a stim channel of the incoming blocks and keeps the
 * mean of the newest epochs per event code, so the evoked responses follow the current state of the
 * subject. An epoch is averaged as soon as its last sample arrives; epochs may overlap and reach back
 * into earlier blocks.
 */

#ifndef RT_AVERAGING_RTPROCESSING_H
#define RT_AVERAGING_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Streaming stimulus-locked averaging.
 *
 * An onset is a sample at which the stim channel reaches the trigger threshold from below; its value
 * rounded to an integer is the event code. Like mne.Epochs, an epoch whose pre-stimulus part lies
 * before the first sample is dropped, and an epoch exceeding a peak-to-peak limit is rejected.
 *
 * @brief Moving mean of the newest epochs of every event code, updated with each block.
 *
 * @snippet ex_dsp_rt/main.cpp rt_averaging_usage
 */
class DSPSHARED_EXPORT RtAveraging
{
public:
    /** @brief Epoch definition, window length, baseline and rejection. */
    struct Settings
    {
        int pre_stim = 0;                       /**< Samples before the onset. */
        int post_stim = 1;                      /**< Samples from the onset on, at least one. */
        int trigger_channel = -1;               /**< Row of the stim channel in the data. */
        double trigger_threshold = 0.5;         /**< Value the stim channel reaches at an onset. */
        int averages = 1;                       /**< Newest epochs averaged per event code, at least one. */
        FiffEvoked::Baseline baseline{};        /**< Interval in seconds around the onset whose mean is subtracted. */
        std::map<std::string, double> reject{}; /**< Peak-to-peak limits per channel type (mag, grad, eeg, eog). */
    };

    //=========================================================================================================
    /**
     * @param[in] info       Measurement info of the data.
     * @param[in] settings   Epoch definition, window length, baseline and rejection.
     */
    RtAveraging(std::shared_ptr<const FiffInfo> info, Settings settings);

    //=========================================================================================================
    /**
     * Cuts the epochs of the onsets in the block and averages those that are complete.
     *
     * @param[in] block   All channels of the measurement info by samples.
     *
     * @return The ascending event codes whose evoked response changed.
     */
    [[nodiscard]] std::vector<int> update(const Eigen::Ref<const Eigen::MatrixXd>& block);

    //=========================================================================================================
    /**
     * @return The evoked response of every event code seen so far, its comment the event code.
     */
    [[nodiscard]] const FiffEvokedSet& evoked() const;

    //=========================================================================================================
    /**
     * @return The current settings, with the epoch length and the averages raised to their minimum.
     */
    [[nodiscard]] const Settings& settings() const;

    //=========================================================================================================
    /**
     * Applies new settings. Another epoch definition drops all epochs; another window length, baseline or
     * rejection keeps the averaged epochs and updates the evoked responses at once.
     *
     * @param[in] settings   The new settings.
     */
    void set_settings(Settings settings);

    //=========================================================================================================
    /**
     * Drops all epochs and evoked responses and starts a new stream.
     */
    void reset();

private:
    struct PendingEpoch
    {
        int code;
        Eigen::MatrixXd data;
        Eigen::Index filled;
    };

    struct Average
    {
        std::deque<Eigen::MatrixXd> epochs;
        Eigen::MatrixXd sum;
        int removed = 0; /**< Epochs subtracted from the sum since it was last summed up. */
    };

    bool add_epoch(PendingEpoch& epoch);
    void trim(Average& average);
    void publish(int code, const Average& average);

    std::shared_ptr<const FiffInfo> m_info;
    Settings m_settings;
    FiffEvokedSet m_evoked;
    Eigen::RowVectorXf m_times;
    std::map<int, Average> m_averages;
    std::vector<PendingEpoch> m_pending;
    Eigen::MatrixXd m_history; /**< The last pre_stim samples of the stream. */
    std::int64_t m_samples = 0;
    double m_last_trigger = 0.0;
};

} // namespace mne

#endif // RT_AVERAGING_RTPROCESSING_H
