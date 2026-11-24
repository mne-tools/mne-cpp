//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_epoch_data_list.h
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Ordered list of @ref mne::MNEEpochData objects sharing a common @ref mne::FiffInfo.
 *
 * @ref mne::MNEEpochDataList is the C++ counterpart of MNE-Python's
 * @c mne.Epochs: a vector of trials cut from a continuous raw recording
 * with common pre/post-stimulus boundaries, optional artifact rejection
 * and convenience helpers to average into a @ref mne::FiffEvoked or
 * to dump back to FIFF.
 */

#ifndef MNE_EPOCH_DATA_LIST_H
#define MNE_EPOCH_DATA_LIST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_epoch_data.h"
#include <mne/fiff/fiff_types.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_raw_data.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/**
 * @brief Per-channel peak-to-peak check record used internally by MNEEpochDataList::checkForArtifact.
 */
struct ArtifactRejectionData
{
    bool bRejected = false;  /**< Set when the channel's peak-to-peak range exceeds dThreshold. */
    Eigen::RowVectorXd data; /**< One channel of the epoch. */
    double dThreshold = 0.0; /**< Peak-to-peak limit for the channel type. */
    std::string sChName;     /**< Channel name. */
};

//=============================================================================================================
/**
 * Epoch data list, which corresponds to a set of events
 *
 * @brief Ordered list of @ref MNEEpochData objects sharing a common measurement info.
 *
 * @snippet ex_mne_api/main.cpp mne_epoch_data_list_usage
 *
 * @snippet ex_mne_api/main.cpp mne_epoch_rejection_usage
 */
class MNESHARED_EXPORT MNEEpochDataList : public std::vector<std::shared_ptr<MNEEpochData>>
{
public:
    //=========================================================================================================
    /**
     * Default constructor.
     */
    MNEEpochDataList();

    //=========================================================================================================
    /**
     * Destroys the MNEEpochDataList.
     */
    ~MNEEpochDataList();

    //=========================================================================================================
    /**
     * Read the epochs from a raw file based on provided events.
     *
     * @param[in] raw            The raw data.
     * @param[in] events         The events provided in samples and event kind.
     * @param[in] tmin           The start time relative to the event in seconds.
     * @param[in] tmax           The end time relative to the event in seconds.
     * @param[in] event          The event kind.
     * @param[in, out] mapReject Peak-to-peak rejection thresholds per channel type (e.g. "grad", "mag", "eeg").
     * @param[in] lExcludeChs    List of channel names to exclude.
     * @param[in] picks          Which channels to pick.
     *
     * @return List of epochs with rejection flags set; empty if no matching events are found.
     */
    static MNEEpochDataList readEpochs(const mne::FiffRawData& raw,
                                       const Eigen::MatrixXi& events,
                                       float tmin,
                                       float tmax,
                                       std::int32_t event,
                                       const std::map<std::string, double>& mapReject,
                                       const mne::StringList& lExcludeChs = {},
                                       const Eigen::RowVectorXi& picks = Eigen::RowVectorXi());

    //=========================================================================================================
    /**
     * Averages epoch list. Note that no baseline correction performed.
     *
     * @param[in] p_info     measurement info.
     * @param[in] first    First time sample.
     * @param[in] last     Last time sample.
     * @param[in] sel      Which epochs should be averaged (optional).
     * @param[in] proj     Apply SSP projection vectors (optional, default = false).
     *
     * @return Averaged evoked response; aspect_kind is FIFFV_ASPECT_STD_ERR with no data if the list is empty.
     */
    mne::FiffEvoked average(const mne::FiffInfo& p_info,
                            mne::fiff_int_t first,
                            mne::fiff_int_t last,
                            Eigen::VectorXi sel = mne::defaultVectorXi,
                            bool proj = false) const;

    //=========================================================================================================
    /**
     * Applies baseline correction to the evoked data.
     *
     * @param[in] baseline     time definition of the baseline in seconds [from, to].
     */
    void applyBaselineCorrection(const std::pair<float, float>& baseline);

    //=========================================================================================================
    /**
     * Drop/Remove all epochs tagged as rejected
     */
    void dropRejected();

    //=========================================================================================================
    /**
     * Reduces alld epochs to the selected rows.
     *
     * @param[in] sel     The selected rows to keep.
     */
    void pick_channels(const Eigen::RowVectorXi& sel);

    //=========================================================================================================
    /**
     * Checks the givven matrix for artifacts beyond a threshold value.
     *
     * @param[in] data           The data matrix.
     * @param[in] pFiffInfo      The fiff info.
     * @param[in] mapReject      The channel data types to scan for. EEG, MEG or EOG.
     * @param[in] lExcludeChs    List of channel names to exclude.
     *
     * @return   Whether a threshold artifact was detected.
     */
    static bool checkForArtifact(const Eigen::MatrixXd& data,
                                 const mne::FiffInfo& pFiffInfo,
                                 const std::map<std::string, double>& mapReject,
                                 const mne::StringList& lExcludeChs = {});

    //=========================================================================================================
    /**
     * averageCategories
     *
     * Multi-category offline averaging. Reads epochs from raw data for multiple event types
     * and returns an FiffEvokedSet with one FiffEvoked per category.
     * Ported from average.c (MNE-C).
     *
     * @param[in] raw           The raw data.
     * @param[in] events        Event matrix (nEvents x 3): [sample, before, after].
     * @param[in] eventCodes    List of event codes, one per category.
     * @param[in] comments      List of category names/comments, one per category.
     * @param[in] tmin          Start of epoch relative to event (seconds).
     * @param[in] tmax          End of epoch relative to event (seconds).
     * @param[in] mapReject     Rejection thresholds (key = channel type string, value = threshold).
     * @param[in] baseline      Baseline interval [from, to] in seconds. If from==to, no baseline correction.
     * @param[in] proj          Apply SSP projection vectors (optional, default = false).
     *
     * @return FiffEvokedSet containing one FiffEvoked per category.
     */
    static mne::FiffEvokedSet averageCategories(const mne::FiffRawData& raw,
                                                const Eigen::MatrixXi& events,
                                                const std::vector<int>& eventCodes,
                                                const mne::StringList& comments,
                                                float tmin,
                                                float tmax,
                                                const std::map<std::string, double>& mapReject = {},
                                                const std::pair<float, float>& baseline = std::pair<float, float>(0.0f, 0.0f),
                                                bool proj = false);

    //=========================================================================================================
    /**
     * Convenience function: reads epochs, optionally applies baseline correction and
     * artifact rejection, then returns the averaged evoked response.
     *
     * @param[in] raw               The raw data.
     * @param[in] matEvents         The events provided in samples and event kinds.
     * @param[in] fTMinS            The start time relative to the event in seconds.
     * @param[in] fTMaxS            The end time relative to the event in seconds.
     * @param[in] eventType         The event type.
     * @param[in] bApplyBaseline    Whether to use baseline correction (mode=mean).
     * @param[in] fTBaselineFromS   The start baseline correction time relative to the event in seconds.
     * @param[in] fTBaselineToS     The end baseline correction time relative to the event in seconds.
     * @param[in] mapReject         The thresholds per channel type to reject epochs.
     * @param[in] lExcludeChs       List of channel names to exclude.
     * @param[in] vecPicks          Which channels to pick.
     *
     * @return The averaged evoked data.
     */
    static mne::FiffEvoked computeAverage(const mne::FiffRawData& raw,
                                          const Eigen::MatrixXi& matEvents,
                                          float fTMinS,
                                          float fTMaxS,
                                          std::int32_t eventType,
                                          bool bApplyBaseline,
                                          float fTBaselineFromS,
                                          float fTBaselineToS,
                                          const std::map<std::string, double>& mapReject,
                                          const mne::StringList& lExcludeChs = {},
                                          const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi());

private:
    //=========================================================================================================
    /**
     * Per-channel worker of checkForArtifact: flags the channel when its peak-to-peak range exceeds the threshold.
     *
     * @param[in, out] inputData     The channel data and threshold; bRejected is set on output.
     */
    static void checkChThreshold(ArtifactRejectionData& inputData);
};
} // NAMESPACE

#endif // MNE_EPOCH_DATA_LIST_H
