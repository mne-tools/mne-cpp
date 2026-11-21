//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_annotations.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    FIFF / MNE annotations: time-tagged textual marks (BAD_*, EDGE, custom) with onset, duration and description.
 *
 * Annotations are time-localized comments attached to a continuous
 * recording: rejected segments ("BAD_*"), recording edges ("EDGE"),
 * and arbitrary user-supplied marks. MNE-Python stores them as the
 * @c mne.Annotations type and writes them under
 * @c FIFFB_MNE_ANNOTATIONS; this header is the C++ mirror.
 *
 * @ref mne::FiffAnnotation is one entry (onset in seconds since the
 * recording start, duration, description string), @ref mne::FiffAnnotations is
 * the container; together they expose JSON and CSV serialization so the
 * same annotations can be exchanged with external tools that do not
 * speak FIFF (eg. BIDS sidecar files).
 */

#ifndef FIFF_ANNOTATIONS_H
#define FIFF_ANNOTATIONS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include <base/error.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief One FIFF / MNE annotation: onset (s), duration (s) and description string.
 *
 * Field-for-field counterpart of one row of @c mne.Annotations in
 * MNE-Python. Onset is given in seconds relative to the recording start
 * (matching the @c first_samp / @c sfreq convention of @ref FiffRawData).
 *
 * @snippet ex_fiff_api/main.cpp fiff_annotations_usage
 */
struct FIFFSHARED_EXPORT FiffAnnotation
{
    /** A value of mne.Annotations.extras: None, bool, int, float or str. */
    using Extra = std::variant<std::monostate, bool, std::int64_t, double, std::string>;
    /** The extras of one annotation, by key. */
    using Extras = std::map<std::string, Extra, std::less<>>;

    double onset = 0.0;                /**< Onset in seconds. */
    double duration = 0.0;             /**< Duration in seconds (0 for point events). */
    std::string description;           /**< Label: "BAD", "BAD_ACQ_SKIP", etc. */
    std::vector<std::string> ch_names; /**< Channels the annotation applies to; none for all. */
    std::string comment;               /**< Optional free-form comment. */
    Extras extras;                     /**< Format-specific metadata, like mne.Annotations.extras. */

    /** @return Whether both annotations hold the same values. */
    bool operator==(const FiffAnnotation&) const = default;
};

//=============================================================================================================
/**
 * @brief Container for @ref FiffAnnotation entries with FIFF, JSON and CSV serializers.
 *
 * Read and written under @c FIFFB_MNE_ANNOTATIONS by FiffStream::read_annotations()
 * and FiffStream::write_annotations(), so the annotation list travels with the
 * FIFF file. The JSON and CSV files let the same annotations be consumed by
 * BIDS sidecars and by external scoring tools that do not understand FIFF.
 *
 * @snippet ex_fiff_api/main.cpp fiff_annotations_usage
 */
class FIFFSHARED_EXPORT FiffAnnotations
{
public:
    using value_type = FiffAnnotation;                                  /**< Element type. */
    using iterator = std::vector<FiffAnnotation>::iterator;             /**< Iterator. */
    using const_iterator = std::vector<FiffAnnotation>::const_iterator; /**< Constant iterator. */

    /** @return The number of annotations. */
    std::size_t size() const noexcept
    {
        return m_annotations.size();
    }

    /** @return Whether there is no annotation. */
    bool empty() const noexcept
    {
        return m_annotations.empty();
    }

    /**
     * @param[in] index   Index below size().
     *
     * @return Annotation @p index.
     */
    const FiffAnnotation& operator[](std::size_t index) const
    {
        return m_annotations[index];
    }

    /**
     * @param[in] index   Index below size().
     *
     * @return Annotation @p index.
     */
    FiffAnnotation& operator[](std::size_t index)
    {
        return m_annotations[index];
    }

    iterator begin() noexcept
    {
        return m_annotations.begin();
    } /**< @return First annotation. */
    iterator end() noexcept
    {
        return m_annotations.end();
    } /**< @return Past the last annotation. */
    const_iterator begin() const noexcept
    {
        return m_annotations.begin();
    } /**< @return First annotation. */
    const_iterator end() const noexcept
    {
        return m_annotations.end();
    } /**< @return Past the last annotation. */

    /**
     * Appends an annotation.
     *
     * @param[in] annotation   The annotation.
     */
    void push_back(FiffAnnotation annotation)
    {
        m_annotations.push_back(std::move(annotation));
    }

    //=========================================================================================================
    /**
     * Appends an annotation, like mne.Annotations.append.
     *
     * @param[in] onset         Onset in seconds.
     * @param[in] duration      Duration in seconds.
     * @param[in] description   Description label.
     * @param[in] ch_names      Channels it applies to; none for all.
     */
    void append(double onset, double duration, std::string description, std::vector<std::string> ch_names = {});

    /**
     * Removes an annotation.
     *
     * @param[in] position   The annotation.
     *
     * @return The annotation after it.
     */
    iterator erase(const_iterator position)
    {
        return m_annotations.erase(position);
    }

    /**
     * Removes a range of annotations.
     *
     * @param[in] first   The first annotation to remove.
     * @param[in] last    Past the last annotation to remove.
     *
     * @return The annotation after the range.
     */
    iterator erase(const_iterator first, const_iterator last)
    {
        return m_annotations.erase(first, last);
    }

    /** Removes all annotations. */
    void clear() noexcept
    {
        m_annotations.clear();
    }

    //=========================================================================================================
    /**
     * @param[in] index          Annotation index.
     * @param[in] sfreq          Sampling frequency in Hz.
     * @param[in] first_sample   First sample of the recording.
     *
     * @return The sample at the onset of annotation @p index.
     */
    int onset_to_sample(std::size_t index, double sfreq, int first_sample = 0) const;

    //=========================================================================================================
    /**
     * @param[in] index          Annotation index.
     * @param[in] sfreq          Sampling frequency in Hz.
     * @param[in] first_sample   First sample of the recording.
     *
     * @return The sample at the end (onset + duration) of annotation @p index.
     */
    int end_to_sample(std::size_t index, double sfreq, int first_sample = 0) const;

    //=========================================================================================================
    /**
     * @param[in] prefix   Description prefix.
     *
     * @return The annotations whose description starts with @p prefix.
     */
    FiffAnnotations select(std::string_view prefix) const;

    //=========================================================================================================
    /**
     * @param[in] ch_name   Channel name.
     *
     * @return The annotations that apply to @p ch_name, including those that apply to all channels.
     */
    FiffAnnotations select_by_channel(std::string_view ch_name) const;

    //=========================================================================================================
    /**
     * @param[in] tmin   Start time in seconds.
     * @param[in] tmax   End time in seconds.
     *
     * @return The annotations overlapping [tmin, tmax], clipped to it, like mne.Annotations.crop.
     */
    FiffAnnotations crop(double tmin, double tmax) const;

    //=========================================================================================================
    /**
     * Reads annotations from a JSON file ({"annotations": [{"onset", "duration", "description",
     * "channel_names", "comment", "extras"}, ...]}).
     *
     * @param[in] path   The file.
     *
     * @return The annotations, or why the file cannot be read (Io or Format).
     */
    [[nodiscard]] static mne::Expected<FiffAnnotations> read_json(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads annotations from a CSV file ("onset,duration,description" after a header row); rows
     * whose onset or duration is not a number are skipped.
     *
     * @param[in] path   The file.
     *
     * @return The annotations, or why the file cannot be read (Io).
     */
    [[nodiscard]] static mne::Expected<FiffAnnotations> read_csv(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads annotations from a .json or .csv file; other extensions are tried as JSON, then as CSV.
     *
     * @param[in] path   The file.
     *
     * @return The annotations, or why the file cannot be read.
     */
    [[nodiscard]] static mne::Expected<FiffAnnotations> read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Writes the annotations to a JSON file.
     *
     * @param[in] path   The file.
     *
     * @return Nothing, or why the file cannot be written (Io).
     */
    [[nodiscard]] mne::Expected<void> write_json(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Writes onset, duration and description of the annotations to a CSV file.
     *
     * @param[in] path   The file.
     *
     * @return Nothing, or why the file cannot be written (Io).
     */
    [[nodiscard]] mne::Expected<void> write_csv(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Writes the annotations to a .csv file, or to JSON for any other extension.
     *
     * @param[in] path   The file.
     *
     * @return Nothing, or why the file cannot be written (Io).
     */
    [[nodiscard]] mne::Expected<void> write(const std::filesystem::path& path) const;

private:
    std::vector<FiffAnnotation> m_annotations; /**< Annotation storage. */
};

} // namespace mne

#endif // FIFF_ANNOTATIONS_H
