//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     bids_edf_reader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.1.0
 * @date     March 2019
 * @brief    @ref mne::AbstractFormatReader implementation for European Data Format (EDF / EDF+) and BioSemi BDF files.
 *
 * EDF stores a recording as a fixed-size ASCII header followed by a
 * stream of @c duration_seconds-long "data records"; inside each
 * record the channels appear in order, with each channel contributing
 * @c samples_per_record little-endian @c int16 samples (24-bit integers
 * in a @c .bdf file). The header
 * additionally lists per-channel physical / digital min / max which
 * @ref mne::EDFReader uses to derive the affine calibration that
 * converts the on-disk integers back into the channel's physical unit
 * (volts, microvolts, degrees Celsius, …).
 *
 * The same parser feeds @ref mne::BidsRawData and the @c mne_edf2fiff
 * converter. Each channel is scaled from its physical dimension
 * (µV, mV) into SI units, as in mne read_raw_edf, so voltages emerge
 * in volts, matching the MNE-CPP @c mne_fiff convention. A "Status" or
 * "Trigger" channel keeps the low 17 bits of its value (raw for BDF),
 * as in mne read_raw_edf / read_raw_bdf.
 *
 * Format reference: Kemp & Olivan, "European data format 'plus'
 * (EDF+)", Clin. Neurophysiol. 114 (2003) 1755–1761; spec at
 * https://www.edfplus.info/specs/edf.html.
 */

#ifndef BIDS_EDF_READER_H
#define BIDS_EDF_READER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_abstract_format_reader.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FileByteStream;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Channel-level metadata from the EDF header.
 *
 * @snippet ex_bids/main.cpp edf_channel_info
 */
struct BIDSSHARED_EXPORT EDFChannelInfo
{
    int channelNumber{-1};
    std::string label;             /**< Signal label, Latin-1 bytes as stored. */
    std::string transducerType;    /**< Transducer type. */
    std::string physicalDimension; /**< Physical dimension (e.g. "uV"), Latin-1 bytes as stored. */
    std::string prefiltering;      /**< Prefiltering description. */
    float physicalMin{0.0f};
    float physicalMax{0.0f};
    long digitalMin{0};
    long digitalMax{0};
    long samplesPerRecord{0};
    long sampleCount{0};
    float frequency{0.0f};
    bool isMeasurement{false};

    /** @return The FIFF channel description: kind from the label, volts for voltage dimensions. */
    mne::FiffChInfo to_fiff_ch_info() const;

    /** @return Factor from the physical dimension to SI (µV/uV 1e-6, mV 1e-3, otherwise 1; 1 for a "Status"/"Trigger" stim channel), as in mne read_raw_edf. */
    float to_si() const;
};

//=============================================================================================================
/**
 * @brief The EDFReader reads European Data Format (EDF/EDF+) files and exposes them through
 *        the AbstractFormatReader interface.
 *
 *        EDF stores data as 16-bit (BDF: 24-bit) little-endian integers in fixed-duration
 *        "data records", with channels interleaved within each record.
 *
 * @snippet ex_bids/main.cpp edf_reader_usage
 */
class BIDSSHARED_EXPORT EDFReader : public AbstractFormatReader
{
public:
    EDFReader();
    ~EDFReader() override;

    bool open(const std::filesystem::path& path) override;
    mne::FiffInfo info() const override;
    Eigen::MatrixXf read_raw_segment(long first, long last) const override;
    long sample_count() const override;
    float sfreq() const override;
    int channel_count() const override;
    std::string_view format_name() const override;
    bool supports_extension(std::string_view extension) const override;

    /** @return Infos for every signal in the EDF header, including annotation/extra channels. */
    const std::vector<EDFChannelInfo>& all_channel_infos() const
    {
        return m_allChannels;
    }

    /** @return Infos for the measurement channels only, in channel order. */
    const std::vector<EDFChannelInfo>& measurement_channel_infos() const
    {
        return m_measChannels;
    }

private:
    bool parse_header();

    /**
     * The recording start in UTC, as MNE-Python reads it: the EDF+ "Startdate dd-MMM-yyyy" recording field
     * when present, else the "dd.mm.yy" header date with years 85-99 in 1985-1999 and 00-84 in 2000-2084,
     * plus the "hh.mm.ss" header time. Empty when no valid date is found.
     */
    static std::optional<std::chrono::sys_time<std::chrono::microseconds>> parse_start_date(std::string_view recordingId,
                                                                                            std::string_view startDate,
                                                                                            std::string_view startTime);

    std::optional<std::chrono::sys_time<std::chrono::microseconds>> m_measDate;
    long m_numBytesInHeader{0};
    long m_numDataRecords{0};
    long m_numBytesPerDataRecord{0};

    std::vector<EDFChannelInfo> m_allChannels;
    std::vector<EDFChannelInfo> m_measChannels;

    std::unique_ptr<mne::FileByteStream> m_file;
    int m_bytesPerSample{2}; // 3 for BDF
};

} // namespace mne

#endif // BIDS_EDF_READER_H
