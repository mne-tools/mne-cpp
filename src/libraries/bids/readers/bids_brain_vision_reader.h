//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_brain_vision_reader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    @ref BIDSLIB::AbstractFormatReader implementation for the BrainVision ``.vhdr / .vmrk / .eeg`` triplet.
 *
 * BrainVision is the de-facto wire format for sEEG / ECoG / scalp-EEG
 * recordings shared as BIDS datasets: an ASCII @c .vhdr describing
 * sampling rate, channel labels, calibration factors and the binary
 * layout; a parallel ASCII @c .vmrk carrying stimulus / response /
 * segment markers; and a flat @c .eeg payload holding the samples as
 * little-endian @c INT_16, @c INT_32 or @c IEEE_FLOAT_32 values, laid
 * out either @c MULTIPLEXED (channels interleaved at every time point)
 * or @c VECTORIZED (one contiguous channel after another).
 *
 * @ref BIDSLIB::BrainVisionReader parses the @c .vhdr / @c .vmrk once
 * on @ref BIDSLIB::BrainVisionReader::open, then services arbitrary
 * segment reads by seeking into the still-open @c .eeg file and
 * applying the per-channel @c resolution / unit scaling so callers
 * always see physical-unit values. The auxiliary @ref
 * BIDSLIB::BrainVisionMarker and @ref BIDSLIB::BrainVisionChannelInfo
 * structs expose the parsed header data unchanged for callers that
 * want to convert markers into BIDS events or channels into
 * @c _channels.tsv rows.
 *
 * Format reference: BrainVision Core Data Format 1.0
 * (https://www.brainproducts.com/downloads/more-software-tools/#bvcdf).
 */

#ifndef BIDS_BRAIN_VISION_READER_H
#define BIDS_BRAIN_VISION_READER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_abstract_format_reader.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace BASELIB
{
class FileByteStream;
}

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Marker entry from the .vmrk file.
 *
 * @snippet ex_bids/main.cpp brain_vision_metadata
 */
struct BIDSSHARED_EXPORT BrainVisionMarker
{
    std::string type;                                                     /**< Marker type (e.g., "New Segment", "Stimulus", "Response"). */
    std::string description;                                              /**< Description (e.g., "S253", "R255"). */
    long position{0};                                                     /**< 0-based sample position. */
    long duration{0};                                                     /**< Duration in samples (0 = point marker). */
    int channel{0};                                                       /**< Channel number (0 = all channels). */
    std::optional<std::chrono::sys_time<std::chrono::microseconds>> date; /**< Optional UTC date/time (for New Segment markers). */
};

//=============================================================================================================
/**
 * @brief Channel info from the .vhdr header.
 *
 * @snippet ex_bids/main.cpp brain_vision_metadata
 */
struct BIDSSHARED_EXPORT BrainVisionChannelInfo
{
    int channelNumber{0};
    std::string name;
    std::string reference;
    float resolution{1.0f}; /**< Scaling factor (calibration value). */
    std::string unit;       /**< Physical unit in UTF-8 (e.g. "µV", "V"). */

    /** @return The FIFF channel description: kind from the name and unit, calibration from the resolution. */
    FIFFLIB::FiffChInfo to_fiff_ch_info() const;
};

//=============================================================================================================
/**
 * @brief Binary format enumeration for BrainVision data files.
 */
enum class BVBinaryFormat
{
    INT_16,
    INT_32,
    IEEE_FLOAT_32
};

//=============================================================================================================
/**
 * @brief Data orientation enumeration.
 */
enum class BVOrientation
{
    MULTIPLEXED,
    VECTORIZED
};

//=============================================================================================================
/**
 * @brief The BrainVisionReader reads BrainVision file triplets (.vhdr/.vmrk/.eeg) and exposes
 *        them through the AbstractFormatReader interface.
 *
 *        BrainVision is the most common format for iEEG (sEEG/ECoG) recordings in BIDS datasets.
 *        The format stores data as little-endian binary (INT_16, INT_32, or IEEE_FLOAT_32),
 *        either MULTIPLEXED (channels interleaved per time point) or VECTORIZED (channels contiguous).
 *
 * @snippet ex_bids/main.cpp brain_vision_reader_usage
 */
class BIDSSHARED_EXPORT BrainVisionReader : public AbstractFormatReader
{
public:
    BrainVisionReader();
    ~BrainVisionReader() override;

    bool open(const std::filesystem::path& path) override;
    FIFFLIB::FiffInfo info() const override;
    Eigen::MatrixXf read_raw_segment(long first, long last) const override;
    long sample_count() const override;
    float sfreq() const override;
    int channel_count() const override;
    std::string_view format_name() const override;
    bool supports_extension(std::string_view extension) const override;

    /** @return Markers in file order; empty if no marker file was parsed. */
    const std::vector<BrainVisionMarker>& markers() const
    {
        return m_markers;
    }

    /** @return Channel infos parsed from the .vhdr header, in channel order. */
    const std::vector<BrainVisionChannelInfo>& channel_infos() const
    {
        return m_channels;
    }

    /**
     * @param[in] unit   Unit from the header in UTF-8, e.g. "µV".
     *
     * @return Factor from the unit into SI (V for voltages, S for siemens); unknown units scale by 1, like mne.
     */
    static float unit_scale(std::string_view unit);

    /**
     * @param[in] unit   Unit from the header in UTF-8.
     *
     * @return Whether the unit is a voltage (V, mV, µV, uV, nV); other channels are typed misc, like mne.
     */
    static bool is_voltage_unit(std::string_view unit);

private:
    bool parse_header(const std::filesystem::path& vhdrPath);
    bool parse_markers(const std::filesystem::path& vmrkPath);

    std::filesystem::path m_dataPath;
    std::filesystem::path m_markerPath;

    float m_sfreq{0.0f};
    int m_numChannels{0};
    BVBinaryFormat m_binaryFormat{BVBinaryFormat::INT_16};
    BVOrientation m_orientation{BVOrientation::MULTIPLEXED};
    long m_sampleCount{0};

    std::vector<BrainVisionChannelInfo> m_channels;
    std::vector<BrainVisionMarker> m_markers;
    std::optional<std::chrono::sys_time<std::chrono::microseconds>> m_measDate; /**< Date of the first dated New Segment marker. */

    std::unique_ptr<BASELIB::FileByteStream> m_dataFile;
};

} // namespace BIDSLIB

#endif // BIDS_BRAIN_VISION_READER_H
