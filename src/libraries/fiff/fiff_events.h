//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_events.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Stim-channel event list (sample, previous value, new value triples) with FIFF read/write helpers.
 *
 * FIFF events are the standard integer representation of trigger / stim
 * information: each event is a @c (sample, previous_value, new_value)
 * triple, and a list of them is stored under @c FIFFB_MNE_EVENTS. The
 * @ref mne::FiffEvents class owns one such list and exposes the operations the
 * rest of mne_fiff needs: read / write FIFF event files, detect events on
 * a stim channel of a @ref mne::FiffRawData (parity with
 * @c mne.find_events), filter by code, and clip to a sample range.
 */

#ifndef FIFF_EVENTS_H
#define FIFF_EVENTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include <base/error.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

// Forward declaration
class FiffRawData;
struct AverageCategory;

//=============================================================================================================
/**
 * @brief FIFF event list: (sample, prev, new) integer triples with read / write / detect / filter helpers.
 *
 * On-disk representation matches @c mne.io.write.write_events output and
 * the @c -eve.fif files consumed by @c mne.read_events; the detection
 * path mirrors @c mne.find_events. Used by @ref FiffEpochs and the
 * batch-averaging path in @ref FiffEvokedSet to drive epoch extraction.
 *
 * @snippet ex_fiff_api/main.cpp fiff_events_usage
 */
class FIFFSHARED_EXPORT FiffEvents
{
public:
    /** Constructs an empty event list. */
    FiffEvents() = default;

    //=========================================================================================================
    /**
     * Constructs an event list.
     *
     * @param[in] events   Events, one row [sample, before, after] each.
     */
    explicit FiffEvents(Eigen::MatrixXi events)
    : events(std::move(events))
    {
    }

    //=========================================================================================================
    /**
     * Reads an event file like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The event file.
     */
    explicit FiffEvents(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads an event file, like mne.read_events: a FIFF file if its name holds ".fif", text otherwise.
     *
     * @param[in] path   The event file.
     *
     * @return The events, or why the file cannot be read.
     */
    [[nodiscard]] static mne::Expected<FiffEvents> read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the event list of a FIFF file.
     *
     * @param[in] path   The FIFF file.
     *
     * @return The events, or why the file cannot be read: Io if it cannot be opened, NotFound if it
     *         holds no event list.
     */
    [[nodiscard]] static mne::Expected<FiffEvents> read_fif(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads a text event file: one event per line as "sample [time] before after"; lines starting
     * with '%' or '#' are comments.
     *
     * @param[in] path   The text file.
     *
     * @return The events, or why the file cannot be read: Io if it cannot be opened, Format if a line
     *         does not start with a sample number.
     */
    [[nodiscard]] static mne::Expected<FiffEvents> read_ascii(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Writes the events, like mne.write_events: to FIFF if the name holds ".fif", as text otherwise.
     *
     * @param[in] path    The file.
     * @param[in] sfreq   Sampling frequency for the time column of a text file; 0 writes zeros.
     *
     * @return Nothing, or why the file cannot be written.
     */
    [[nodiscard]] mne::Expected<void> write(const std::filesystem::path& path, float sfreq = 0.0f) const;

    //=========================================================================================================
    /**
     * Writes the events to a FIFF file.
     *
     * @param[in] path   The FIFF file.
     *
     * @return Nothing, or why the file cannot be written: Format if there are no events, Io otherwise.
     */
    [[nodiscard]] mne::Expected<void> write_fif(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Writes the events as text, one "sample time before after" line each, as MNE-C does.
     *
     * @param[in] path    The text file.
     * @param[in] sfreq   Sampling frequency for the time column; 0 writes zeros.
     *
     * @return Nothing, or why the file cannot be written (Io).
     */
    [[nodiscard]] mne::Expected<void> write_ascii(const std::filesystem::path& path, float sfreq = 0.0f) const;

    //=========================================================================================================
    /**
     * Finds the trigger events of a stimulus channel, like mne.find_events.
     *
     * @param[in] raw            The recording.
     * @param[in] stim_channel   Name of the stimulus channel; matched as same_channel() does ("STI014" finds "STI 014").
     * @param[in] mask           Bits of the channel value that count.
     * @param[in] leading_edge   Only changes from 0 to a non-zero value, rather than every change.
     *
     * @return The events, which may be none, or why there are none: NotFound if the channel is
     *         missing, Io if its data cannot be read.
     */
    [[nodiscard]] static mne::Expected<FiffEvents> find_events(const FiffRawData& raw,
                                                               std::string_view stim_channel = "STI 014",
                                                               unsigned int mask = 0xFFFFFFFF,
                                                               bool leading_edge = true);

    /** @return The number of events. */
    int num_events() const
    {
        return static_cast<int>(events.rows());
    }

    /** @return Whether there is no event. */
    bool empty() const
    {
        return events.rows() == 0;
    }

    //=========================================================================================================
    /**
     * Checks whether an event matches an averaging category, as MNE-C's mne_process_raw does.
     *
     * @param[in] cat         The category.
     * @param[in] events      Events, one row [sample, before, after] each.
     * @param[in] event_idx   Row of the event.
     *
     * @return Whether the event matches, including the category's conditions on the events around it.
     */
    static bool match_event(const AverageCategory& cat, const Eigen::MatrixXi& events, int event_idx);

    //=========================================================================================================
    /**
     * Selects events by code, like mne.pick_events.
     *
     * @param[in] include   Codes to keep; if none, all codes except @\p exclude are kept.
     * @param[in] exclude   Codes to drop when @p include is empty.
     * @param[in] step      Also match the code before the step (column 1).
     *
     * @return The selected events in their original order.
     */
    FiffEvents pick(const std::vector<int>& include, const std::vector<int>& exclude = {}, bool step = false) const;

    //=========================================================================================================
    /**
     * Replaces codes in both code columns by a new one, like mne.merge_events.
     *
     * @param[in] ids               Codes to replace.
     * @param[in] new_id            The new code.
     * @param[in] replace_events    If false, the original events are kept as well, sorted by row.
     *
     * @return The merged events.
     */
    FiffEvents merge(const std::vector<int>& ids, int new_id, bool replace_events = true) const;

    //=========================================================================================================
    /**
     * Counts the events per code (column 2), like mne.count_events.
     *
     * @param[in] ids   Codes to report (0 if absent); if none, every code that occurs.
     *
     * @return The number of events with each code.
     */
    std::map<int, int> count(const std::vector<int>& ids = {}) const;

    //=========================================================================================================
    /**
     * Concatenates the events of consecutive recordings, like mne.concatenate_events.
     *
     * @param[in] events        Events of each recording.
     * @param[in] first_samps   First sample of each recording.
     * @param[in] last_samps    Last sample of each recording.
     *
     * @return Events on the sample axis of the concatenated recording (none if the list lengths differ).
     */
    static FiffEvents concatenate(const std::vector<FiffEvents>& events, const std::vector<int>& first_samps, const std::vector<int>& last_samps);

    //=========================================================================================================
    /**
     * Find the samples where all stimulus channels change at once, like mne.find_stim_steps.
     *
     * @param[in] stim_data   Stimulus channel data (channels x samples); steps carry the values of the first row.
     * @param[in] first_samp  Sample number of the first column.
     * @param[in] pad_start   If set, a step from this value at sample 0 is added when the data start elsewhere.
     * @param[in] pad_stop    If set, a step to this value after the last sample is added.
     * @param[in] merge      Merge steps at most |merge| samples apart (> 0: into the later, < 0: into the earlier).
     *
     * @return Steps as [sample, value before, value after].
     */
    static FiffEvents find_stim_steps(const Eigen::MatrixXi& stim_data,
                                      int first_samp,
                                      std::optional<int> pad_start = std::nullopt,
                                      std::optional<int> pad_stop = std::nullopt,
                                      int merge = 0);

    //=========================================================================================================
    /**
     * Events at regular intervals, like mne.make_fixed_length_events.
     *
     * @param[in] raw        The recording.
     * @param[in] id         Event code.
     * @param[in] start      First event time in s.
     * @param[in] stop       Time in s after which no event may start (negative: end of the recording).
     * @param[in] duration   Duration of each segment in s.
     * @param[in] first_samp  Whether times count from the recording's first sample (true) or from 0.
     * @param[in] overlap    Overlap of consecutive segments in s, 0 <= overlap < duration.
     *
     * @return The events (empty if none fit or the overlap is invalid).
     */
    static FiffEvents make_fixed_length(const FiffRawData& raw,
                                        int id = 1,
                                        double start = 0.0,
                                        double stop = -1.0,
                                        double duration = 1.0,
                                        bool first_samp = true,
                                        double overlap = 0.0);

    Eigen::MatrixXi events; /**< Event matrix (nEvents x 3): [sample, before, after]. */
};

} // NAMESPACE

#endif // FIFF_EVENTS_H
