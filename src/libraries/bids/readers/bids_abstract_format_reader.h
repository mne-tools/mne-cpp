//=============================================================================================================
/**
 * @file     bids_abstract_format_reader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the declaration of the AbstractFormatReader class.
 *
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
