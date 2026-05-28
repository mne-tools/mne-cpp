//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_abstract_format_reader.h
 * @since 2026
 * @date  April 2026
 * @brief Strategy interface for format-specific raw-data readers (BrainVision, EDF, …) consumed by @ref BIDSLIB::BidsRawData.
 *
 * BIDS allows several wire formats for electrophysiology raw data
 * (BrainVision @c .vhdr triplets and @c .edf are by far the most common
 * for iEEG; @c .set and @c .nwb are also permitted). @ref
 * BIDSLIB::AbstractFormatReader is the polymorphic interface every
 * concrete reader implements so @ref BIDSLIB::BidsRawData can drive
 * them uniformly: @ref open parses the format-specific header,
 * @ref getInfo emits a populated @c FIFFLIB::FiffInfo, @ref
 * readRawSegment returns a calibrated @c (n_channels × n_samples)
 * matrix in physical units, and @ref toFiffRawData assembles the whole
 * recording into a @c FIFFLIB::FiffRawData ready to hand to MNE-CPP's
 * filtering / source-localisation stack. @ref formatName and
 * @ref supportsExtension power the dispatch implemented by @ref
 * BIDSLIB::BidsRawData::createReader.
 *
 * Ownership is via @ref UPtr (@c std::unique_ptr) because the concrete
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
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief The AbstractFormatReader class provides a common interface for reading raw neurophysiology
 *        data from various file formats (EDF, BrainVision, etc.) used in BIDS datasets.
 *
 *        Subclasses implement format-specific parsing and expose the data uniformly as FIFF structures.
 */
class BIDSSHARED_EXPORT AbstractFormatReader
{
public:
    using UPtr = std::unique_ptr<AbstractFormatReader>;

    //=========================================================================================================
    /**
     * @brief Virtual destructor.
     */
    virtual ~AbstractFormatReader() = default;

    //=========================================================================================================
    /**
     * @brief Open and parse the file header. Must be called before reading data.
     *
     * @param[in] sFilePath   Absolute path to the primary data file.
     *
     * @return True if the file was opened and parsed successfully.
     */
    virtual bool open(const QString& sFilePath) = 0;

    //=========================================================================================================
    /**
     * @brief Return measurement metadata as FiffInfo.
     *
     * @return FiffInfo populated from the native format header.
     */
    virtual FIFFLIB::FiffInfo getInfo() const = 0;

    //=========================================================================================================
    /**
     * @brief Read a segment of raw data.
     *
     * @param[in] iStartSampleIdx   First sample index (0-based, inclusive).
     * @param[in] iEndSampleIdx     Last sample index (0-based, exclusive).
     *
     * @return Matrix of shape (n_channels x n_samples) with calibrated physical values.
     */
    virtual Eigen::MatrixXf readRawSegment(int iStartSampleIdx, int iEndSampleIdx) const = 0;

    //=========================================================================================================
    /**
     * @brief Return total number of samples across the recording.
     */
    virtual long getSampleCount() const = 0;

    //=========================================================================================================
    /**
     * @brief Return the sampling frequency in Hz.
     */
    virtual float getFrequency() const = 0;

    //=========================================================================================================
    /**
     * @brief Return the number of measurement channels.
     */
    virtual int getChannelCount() const = 0;

    //=========================================================================================================
    /**
     * @brief Convert the entire dataset to a FiffRawData structure.
     *
     * @return FiffRawData populated with info, calibrations, and sample bounds.
     */
    virtual FIFFLIB::FiffRawData toFiffRawData() const = 0;

    //=========================================================================================================
    /**
     * @brief Return a descriptive name for the format (e.g. "EDF", "BrainVision").
     */
    virtual QString formatName() const = 0;

    //=========================================================================================================
    /**
     * @brief Check whether this reader can handle the given file extension.
     *
     * @param[in] sExtension   File extension including dot (e.g. ".edf", ".vhdr").
     *
     * @return True if this reader supports the extension.
     */
    virtual bool supportsExtension(const QString& sExtension) const = 0;

protected:
    AbstractFormatReader() = default;
};

} // namespace BIDSLIB

#endif // BIDS_ABSTRACT_FORMAT_READER_H
