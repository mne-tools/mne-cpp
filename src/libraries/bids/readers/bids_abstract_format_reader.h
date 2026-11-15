//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_abstract_format_reader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Strategy interface for format-specific raw-data readers (BrainVision, EDF, …) consumed by @ref mne::BidsRawData.
 *
 * BIDS allows several wire formats for electrophysiology raw data
 * (BrainVision @c .vhdr triplets and @c .edf are by far the most common
 * for iEEG; @c .set and @c .nwb are also permitted). @ref
 * mne::AbstractFormatReader is the polymorphic interface every
 * concrete reader implements so @ref mne::BidsRawData can drive
 * them uniformly: @ref mne::AbstractFormatReader::open "open" parses the format-specific header,
 * @ref mne::AbstractFormatReader::info "info" emits a populated @c mne::FiffInfo, @ref
 * mne::AbstractFormatReader::read_raw_segment "read_raw_segment" returns a calibrated @c (n_channels × n_samples)
 * matrix in physical units, and @ref mne::AbstractFormatReader::to_fiff_raw_data "to_fiff_raw_data" assembles the
 * whole recording into a @c mne::FiffRawData ready to hand to MNE-CPP's
 * filtering / source-localisation stack. @ref mne::AbstractFormatReader::format_name "format_name" and
 * @ref mne::AbstractFormatReader::supports_extension "supports_extension" power the dispatch implemented by @ref
 * mne::BidsRawData::create_reader.
 *
 * Readers are owned through @c std::unique_ptr because the concrete
 * readers keep the underlying file open for lazy segment reads.
 */

#ifndef BIDS_ABSTRACT_FORMAT_READER_H
#define BIDS_ABSTRACT_FORMAT_READER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../bids_global.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief The AbstractFormatReader class provides a common interface for reading raw neurophysiology
 *        data from various file formats (EDF, BrainVision, etc.) used in BIDS datasets.
 *
 *        Subclasses implement format-specific parsing and expose the data uniformly as FIFF structures.
 *
 * @snippet ex_bids/main.cpp edf_reader_usage
 */
class BIDSSHARED_EXPORT AbstractFormatReader
{
public:
    virtual ~AbstractFormatReader() = default;

    //=========================================================================================================
    /**
     * @brief Open and parse the file header. Must be called before reading data.
     *
     * @param[in] path   Path to the primary data file.
     *
     * @return True if the file was opened and parsed successfully.
     */
    virtual bool open(const std::filesystem::path& path) = 0;

    /** @return Measurement metadata populated from the native format header. */
    virtual mne::FiffInfo info() const = 0;

    //=========================================================================================================
    /**
     * @brief Read a segment of raw data.
     *
     * @param[in] first   First sample index (0-based, inclusive).
     * @param[in] last    Last sample index (0-based, exclusive).
     *
     * @return Matrix of shape (n_channels x n_samples) in SI units; empty for an invalid range.
     */
    virtual Eigen::MatrixXf read_raw_segment(long first, long last) const = 0;

    /** @return Number of samples per channel in the whole recording. */
    virtual long sample_count() const = 0;

    /** @return Sampling frequency in Hz. */
    virtual float sfreq() const = 0;

    /** @return Number of measurement channels (rows returned by read_raw_segment). */
    virtual int channel_count() const = 0;

    /** @return Human-readable format name (e.g. "EDF", "BrainVision"). */
    virtual std::string_view format_name() const = 0;

    //=========================================================================================================
    /**
     * @param[in] extension   File extension including the dot (e.g. ".edf", ".vhdr"), in any letter case.
     *
     * @return True if this reader supports the extension.
     */
    virtual bool supports_extension(std::string_view extension) const = 0;

    /** @return The whole recording as FiffRawData: info, calibrations and sample bounds. */
    mne::FiffRawData to_fiff_raw_data() const
    {
        mne::FiffRawData raw;
        raw.info = info();
        raw.first_samp = 0;
        raw.last_samp = static_cast<int>(sample_count()) - 1;
        raw.cals.resize(static_cast<Eigen::Index>(raw.info.chs.size()));
        for (Eigen::Index i = 0; i < raw.cals.size(); ++i) {
            raw.cals[i] = raw.info.chs[static_cast<std::size_t>(i)].cal;
        }
        return raw;
    }

protected:
    AbstractFormatReader() = default;
};

} // namespace mne

#endif // BIDS_ABSTRACT_FORMAT_READER_H
