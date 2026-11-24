//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_event_list.h
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2006
 * @brief    Event list with comments: MNE-C text and FIFF event files, sorting, merging and selection.
 *
 * @ref mne::MNEEventList is the C++ counterpart of MNE-C's @c mneEventList
 * (@c mne_events.c). Unlike the plain sample/before/after matrix of
 * @ref mne::FiffEvents it keeps a comment per event, reads the comments
 * stored in @c -eve.fif files and in MNE-C text event files, and removes the
 * skipped samples at the start of a recording from the sample numbers.
 */

#ifndef MNE_EVENT_LIST_H
#define MNE_EVENT_LIST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_event.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <optional>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Ordered list of trigger events with comments.
 *
 * @snippet ex_mne_api/main.cpp mne_event_list_usage
 */
class MNESHARED_EXPORT MNEEventList
{
public:
    //=========================================================================================================
    /**
     * Reads the event block of a FIFF file (MNE-C @c mne_load_events_fiff).
     *
     * @param[in] path     A @c -eve.fif file or a raw file carrying an event block.
     * @param[in] offset   Samples to subtract from every event, e.g. the first sample of the recording.
     *
     * @return The events with their comments, or no value if the file has no event block.
     */
    static std::optional<MNEEventList> read_fif(const std::filesystem::path& path, int offset = 0);

    //=========================================================================================================
    /**
     * Reads an MNE-C text event file (MNE-C @c mne_load_events).
     *
     * Each line holds sample, time, trigger value before and after, and an optional
     * comment; @c # starts a comment line. A negative sample is computed from the time.
     * A first line with both trigger values 0 marks the current format: it gives the
     * first sample of the recording and is not an event. Old files have no such line;
     * their samples are shifted by @p old_offset instead.
     *
     * @param[in] path        The text file.
     * @param[in] offset      Samples to subtract from every event (the recording's first sample).
     * @param[in] old_offset  Extra offset used by files in the old format.
     * @param[in] sfreq       Sampling frequency, to convert times of events without a sample.
     *
     * @return The events, or no value if the file cannot be read or holds no events.
     */
    static std::optional<MNEEventList> read_text(const std::filesystem::path& path, int offset, int old_offset, float sfreq);

    //=========================================================================================================
    /**
     * Writes the list as an event block with comments, readable by @c mne.read_events.
     *
     * @param[in] path     The output file.
     * @param[in] offset   Samples to add back to every event.
     *
     * @return True if the file was written.
     */
    [[nodiscard]] bool write_fif(const std::filesystem::path& path, int offset = 0) const;

    //=========================================================================================================
    /**
     * Sorts the events by sample; events at the same sample keep their order.
     */
    void sort();

    //=========================================================================================================
    /**
     * Appends the events of another list (MNE-C @c mne_add_to_event_list).
     *
     * @param[in] other   The events to append.
     */
    void append(const MNEEventList& other);

    //=========================================================================================================
    /**
     * Keeps the onsets of one trigger value (MNE-C @c filter_events): events rising from 0
     * to @p value, or every onset if @p value is 0.
     *
     * @param[in] value   The trigger value to keep, or 0 for all onsets.
     *
     * @return The selected events in their original order.
     */
    MNEEventList select_onsets(unsigned int value = 0) const;

    //=========================================================================================================
    /**
     * Returns the largest trigger value that starts from 0.
     *
     * @return The largest onset value, 0 for a list without onsets.
     */
    unsigned int max_onset() const;

    //=========================================================================================================
    /**
     * Returns the events as rows of sample, value before and value after, the layout of
     * @ref mne::FiffEvents::events and of @c mne.read_events.
     *
     * @return nEvents x 3 matrix.
     */
    Eigen::MatrixXi to_matrix() const;

    //=========================================================================================================
    /**
     * Builds a list from rows of sample, value before and value after.
     *
     * @param[in] matrix   nEvents x 3 matrix as returned by to_matrix().
     *
     * @return The events, without comments.
     */
    static MNEEventList from_matrix(const Eigen::MatrixXi& matrix);

    //=========================================================================================================
    /**
     * @brief Returns the number of events in the list.
     *
     * @return Number of events.
     */
    int nevent() const
    {
        return static_cast<int>(events.size());
    }

    std::vector<MNEEvent> events; /**< The events. */
};

} // namespace mne

#endif // MNE_EVENT_LIST_H
