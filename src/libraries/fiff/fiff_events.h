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
 * @ref FIFFLIB::FiffEvents class owns one such list and exposes the operations the
 * rest of FIFFLIB needs: read / write FIFF event files, detect events on
 * a stim channel of a @ref FIFFLIB::FiffRawData (parity with
 * @c mne.find_events), filter by code, and clip to a sample range.
 */

#ifndef FIFF_EVENTS_H
#define FIFF_EVENTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#include <QList>
#include <QMap>

#include <optional>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QIODevice;

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
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
    //=========================================================================================================
    /**
     * Default constructor. Creates an empty event set.
     */
    FiffEvents();

    //=========================================================================================================
    /**
     * Constructs FiffEvents by reading from an I/O device.
     *
     * Attempts to read as FIFF first; if that fails, tries ASCII format.
     *
     * @param[in] p_IODevice    The I/O device to read from.
     */
    explicit FiffEvents(QIODevice& p_IODevice);

    //=========================================================================================================
    /**
     * Read events from a FIFF or ASCII event file.
     *
     * If t_sEventName is empty, the event file name is derived from the raw file name
     * (replacing ".fif" with "-eve.fif").
     *
     * @param[in] t_sEventName  Path to the event file (empty = derive from raw file name).
     * @param[in] t_fileRawName Path to the raw data file (used to derive event file name if needed).
     * @param[out] p_Events     The loaded events.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read(const QString& t_sEventName,
                     const QString& t_fileRawName,
                     FiffEvents& p_Events);

    //=========================================================================================================
    /**
     * Read an event list from a FIFF file.
     *
     * @param[in] p_IODevice    The I/O device to read from.
     * @param[out] p_Events     The loaded events.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_from_fif(QIODevice& p_IODevice,
                              FiffEvents& p_Events);

    //=========================================================================================================
    /**
     * Read a list of events from an ASCII event file.
     *
     * @param[in] p_IODevice    The I/O device to read from.
     * @param[out] p_Events     The loaded events.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_from_ascii(QIODevice& p_IODevice,
                                FiffEvents& p_Events);

    //=========================================================================================================
    /**
     * Write the event list to a FIFF file.
     *
     * @param[in] p_IODevice    The I/O device to write to.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write_to_fif(QIODevice& p_IODevice) const;

    //=========================================================================================================
    /**
     * Write the event list to a text file (MNE-C compatible format).
     *
     * @param[in] p_IODevice    The I/O device to write to.
     * @param[in] sfreq         Sampling frequency for time column computation.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write_to_ascii(QIODevice& p_IODevice,
                        float sfreq = 0.0f) const;

    //=========================================================================================================
    /**
     * Detect trigger events from raw data using a stimulus channel.
     * Ported from detect_events (MNE-C).
     *
     * @param[in] raw           The raw data.
     * @param[out] p_Events     The detected events.
     * @param[in] triggerCh     Name of the trigger channel (empty = default "STI 014").
     * @param[in] triggerMask   Bitmask to apply to trigger values.
     * @param[in] leadingEdge   If true, only detect rising edges (default = true).
     *
     * @return true if events were detected, false otherwise.
     */
    static bool detect_from_raw(const FiffRawData& raw,
                                FiffEvents& p_Events,
                                const QString& triggerCh = QString("STI 014"),
                                unsigned int triggerMask = 0xFFFFFFFF,
                                bool leadingEdge = true);

    //=========================================================================================================
    /**
     * Returns the number of events.
     *
     * @return Number of events.
     */
    inline int num_events() const;

    //=========================================================================================================
    /**
     * Returns true if no events are stored.
     *
     * @return true if the event matrix has zero rows.
     */
    inline bool is_empty() const;

    //=========================================================================================================
    /**
     * Check whether an event matches a category definition.
     *
     * @param[in] cat       The averaging category to match against.
     * @param[in] events    Full event matrix (nEvents x 3): [sample, from, to].
     * @param[in] eventIdx  Index of the current event row.
     * @return true if the event matches.
     */
    static bool matchEvent(const AverageCategory& cat,
                           const Eigen::MatrixXi& events,
                           int eventIdx);

    //=========================================================================================================
    /**
     * Select events by code, like mne.pick_events.
     *
     * @param[in] include  Codes to keep; if empty, all codes except @p exclude are kept.
     * @param[in] exclude  Codes to drop when @p include is empty.
     * @param[in] step     Also match the code before the step (column 1).
     *
     * @return The selected events in their original order.
     */
    FiffEvents pick(const QList<int>& include, const QList<int>& exclude = {}, bool step = false) const;

    //=========================================================================================================
    /**
     * Replace codes in both code columns by a new one, like mne.merge_events.
     *
     * @param[in] ids            Codes to replace.
     * @param[in] newId          The new code.
     * @param[in] replaceEvents  If false, the original events are kept as well, sorted by row.
     *
     * @return The merged events.
     */
    FiffEvents merge(const QList<int>& ids, int newId, bool replaceEvents = true) const;

    //=========================================================================================================
    /**
     * Count the events per code (column 2), like mne.count_events.
     *
     * @param[in] ids  Codes to report (0 if absent); if empty, every code that occurs.
     *
     * @return Code to count.
     */
    QMap<int, int> count(const QList<int>& ids = {}) const;

    //=========================================================================================================
    /**
     * Concatenate the events of consecutive recordings, like mne.concatenate_events.
     *
     * @param[in] events      Events of each recording.
     * @param[in] firstSamps  First sample of each recording.
     * @param[in] lastSamps   Last sample of each recording.
     *
     * @return Events on the sample axis of the concatenated recording (empty if the list lengths differ).
     */
    static FiffEvents concatenate(const QList<FiffEvents>& events, const QList<int>& firstSamps, const QList<int>& lastSamps);

    //=========================================================================================================
    /**
     * Find the samples where all stimulus channels change at once, like mne.find_stim_steps.
     *
     * @param[in] stimData   Stimulus channel data (channels x samples); steps carry the values of the first row.
     * @param[in] firstSamp  Sample number of the first column.
     * @param[in] padStart   If set, a step from this value at sample 0 is added when the data start elsewhere.
     * @param[in] padStop    If set, a step to this value after the last sample is added.
     * @param[in] merge      Merge steps at most |merge| samples apart (> 0: into the later, < 0: into the earlier).
     *
     * @return Steps as [sample, value before, value after].
     */
    static FiffEvents find_stim_steps(const Eigen::MatrixXi& stimData,
                                      int firstSamp,
                                      std::optional<int> padStart = std::nullopt,
                                      std::optional<int> padStop = std::nullopt,
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
     * @param[in] firstSamp  Whether times count from the recording's first sample (true) or from 0.
     * @param[in] overlap    Overlap of consecutive segments in s, 0 <= overlap < duration.
     *
     * @return The events (empty if none fit or the overlap is invalid).
     */
    static FiffEvents make_fixed_length(const FiffRawData& raw,
                                        int id = 1,
                                        double start = 0.0,
                                        double stop = -1.0,
                                        double duration = 1.0,
                                        bool firstSamp = true,
                                        double overlap = 0.0);

    Eigen::MatrixXi events; /**< Event matrix (nEvents x 3): [sample, before, after]. */
};

} // NAMESPACE

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline int FIFFLIB::FiffEvents::num_events() const
{
    return static_cast<int>(events.rows());
}

//=============================================================================================================

inline bool FIFFLIB::FiffEvents::is_empty() const
{
    return events.rows() == 0;
}

#endif // FIFF_EVENTS_H
