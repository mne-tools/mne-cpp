//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_detect_trigger.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Threshold and edge-based trigger detection on streaming stim channels.
 *
 * The functions in this header scan rows of a data matrix — typically the stimulus / digital-trigger
 * channels of a MEG or EEG recording — for samples that reach a threshold, either in value
 * (detect_trigger_flanks_max) or in their sample-to-sample change (detect_trigger_flanks_grad), and
 * return the sample index and value of every detected flank. After a flank, a burst length of samples
 * is skipped so that one pulse is reported once.
 *
 * @ref mne::to_event_matrix "to_event_matrix" turns the detected flanks of each channel into the
 * three-column event layout (sample, previous value, new value) used by the rest of mne-cpp.
 */

#ifndef RT_DETECT_TRIGGER_RTPROCESSING_H
#define RT_DETECT_TRIGGER_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"

#include <Eigen/Core>

#include <map>
#include <span>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief A detected trigger flank: the sample it was found at and the value there.
 *
 * @snippet ex_dsp_filter/main.cpp trigger_detection_usage
 */
struct DSPSHARED_EXPORT TriggerFlank
{
    int sample = 0;     /**< Sample index, including the offset passed to the detector. */
    double value = 0.0; /**< Channel value (max detector) or sample-to-sample change (gradient detector). */

    bool operator==(const TriggerFlank&) const = default;
};

/** @brief Which flanks the gradient detector finds. */
enum class FlankDirection
{
    Rising, /**< Increases of at least the threshold. */
    Falling /**< Decreases of at least the threshold. */
};

//=============================================================================================================
/**
 * @param[in] triggers   Detected flanks by channel index.
 *
 * @return One event matrix per channel, in channel order, with rows (sample, 0, value).
 */
DSPSHARED_EXPORT std::vector<Eigen::MatrixXi> to_event_matrix(const std::map<int, std::vector<TriggerFlank>>& triggers);

//=============================================================================================================
/**
 * Finds the samples of one channel whose value reaches a threshold.
 *
 * @param[in] data          Data, channels by samples.
 * @param[in] channel       Row of the trigger channel; a row outside the data finds nothing.
 * @param[in] offset        Added to every reported sample index, e.g. the first sample of the block.
 * @param[in] threshold     Value a sample must reach.
 * @param[in] removeOffset  Whether to compare the value relative to the first sample of the block.
 * @param[in] burstLength   Samples skipped after each flank.
 *
 * @return The detected flanks in sample order.
 */
DSPSHARED_EXPORT std::vector<TriggerFlank> detect_trigger_flanks_max(const Eigen::MatrixXd& data,
                                                                     int channel,
                                                                     int offset,
                                                                     double threshold,
                                                                     bool removeOffset,
                                                                     int burstLength = 100);

//=============================================================================================================
/**
 * detect_trigger_flanks_max() for several channels.
 *
 * @param[in] data          Data, channels by samples.
 * @param[in] channels      Rows of the trigger channels.
 * @param[in] offset        Added to every reported sample index.
 * @param[in] threshold     Value a sample must reach.
 * @param[in] removeOffset  Whether to compare values relative to the first sample of the block.
 * @param[in] burstLength   Samples skipped after each flank.
 *
 * @return The detected flanks of every listed channel, an empty list for a row outside the data.
 */
DSPSHARED_EXPORT std::map<int, std::vector<TriggerFlank>> detect_trigger_flanks_max(const Eigen::MatrixXd& data,
                                                                                    std::span<const int> channels,
                                                                                    int offset,
                                                                                    double threshold,
                                                                                    bool removeOffset,
                                                                                    int burstLength = 100);

//=============================================================================================================
/**
 * Finds the samples of one channel whose change from the previous sample reaches a threshold.
 *
 * @param[in] data          Data, channels by samples.
 * @param[in] channel       Row of the trigger channel; a row outside the data finds nothing.
 * @param[in] offset        Added to every reported sample index.
 * @param[in] threshold     Change a sample must reach.
 * @param[in] direction     Whether to find rising or falling flanks.
 * @param[in] burstLength   Samples skipped after each flank.
 *
 * @return The detected flanks in sample order; the value is the size of the change.
 */
DSPSHARED_EXPORT std::vector<TriggerFlank> detect_trigger_flanks_grad(const Eigen::MatrixXd& data,
                                                                      int channel,
                                                                      int offset,
                                                                      double threshold,
                                                                      FlankDirection direction,
                                                                      int burstLength = 100);

//=============================================================================================================
/**
 * detect_trigger_flanks_grad() for several channels.
 *
 * @param[in] data          Data, channels by samples.
 * @param[in] channels      Rows of the trigger channels.
 * @param[in] offset        Added to every reported sample index.
 * @param[in] threshold     Change a sample must reach.
 * @param[in] direction     Whether to find rising or falling flanks.
 * @param[in] burstLength   Samples skipped after each flank.
 *
 * @return The detected flanks of every listed channel, an empty list for a row outside the data.
 */
DSPSHARED_EXPORT std::map<int, std::vector<TriggerFlank>> detect_trigger_flanks_grad(const Eigen::MatrixXd& data,
                                                                                     std::span<const int> channels,
                                                                                     int offset,
                                                                                     double threshold,
                                                                                     FlankDirection direction,
                                                                                     int burstLength = 100);

} // namespace mne

#endif // RT_DETECT_TRIGGER_RTPROCESSING_H
