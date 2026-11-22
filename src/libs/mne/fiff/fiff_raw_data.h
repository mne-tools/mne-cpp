//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_raw_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September 2012
 * @brief    FIFF continuous raw recording: FiffInfo plus a directory of FIFF_DATA_BUFFER tags for random-access sample reads.
 *
 * @ref mne::FiffRawData represents a continuous ("raw") Neuromag recording
 * as stored under @c FIFFB_RAW_DATA / @c FIFFB_CONTINUOUS_DATA: a
 * @ref mne::FiffInfo describing the channels and acquisition setup, a list of
 * @ref mne::FiffRawDir entries pointing at each @c FIFF_DATA_BUFFER tag, the
 * first and last sample indices, the per-channel calibration vector and a
 * @ref mne::FiffStream handle for on-demand reads. The
 * @c FiffStream::read_raw_segment family uses that directory to seek
 * directly to the buffers that cover a requested sample range, decode
 * them through the channel cals and projectors, and return a contiguous
 * Eigen matrix. Drop-in counterpart of @c mne.io.Raw in MNE-Python.
 */

#ifndef FIFF_RAW_DATA_H
#define FIFF_RAW_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_info.h"
#include "fiff_raw_dir.h"
#include "fiff_stream.h"
#include <mne/base/error.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

class FiffRawData;

//=============================================================================================================
/**
 * @brief Continuous FIFF raw recording: @ref FiffInfo plus a random-access directory of @c FIFF_DATA_BUFFER tags.
 *
 * The directory built into @ref FiffRawDir lets segment reads jump
 * straight to the buffers covering the requested sample window, decode
 * them through the channel cals and active projectors and return a
 * contiguous channel × sample Eigen matrix without rescanning the file.
 *
 * Open a recording and pick the MEG channels plus the trigger channel:
 *
 * @snippet ex_read_raw/main.cpp fiff_raw_data_open
 *
 * Read a window of the picked channels, in seconds or in samples:
 *
 * @snippet ex_read_raw/main.cpp fiff_raw_data_read_segment
 */
class FIFFSHARED_EXPORT FiffRawData
{
public:
    /** Constructs empty raw data. */
    FiffRawData() = default;

    // Raw data owns the open file, so it moves but does not copy
    FiffRawData(const FiffRawData&) = delete;
    FiffRawData& operator=(const FiffRawData&) = delete;
    FiffRawData(FiffRawData&&) noexcept = default;
    FiffRawData& operator=(FiffRawData&&) noexcept = default;
    ~FiffRawData();

    //=========================================================================================================
    /**
     * Opens a raw FIFF file like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The raw data file.
     */
    explicit FiffRawData(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Opens raw FIFF data held in bytes that are not a file, e.g. in memory, like read(); throws
     * mne::Error if it cannot.
     *
     * @param[in] bytes   The raw data; kept for reading segments.
     */
    explicit FiffRawData(std::unique_ptr<mne::ByteStream> bytes);

    //=========================================================================================================
    /**
     * Opens a raw FIFF file, like mne.io.read_raw_fif(path, preload=False): reads the measurement info
     * and the directory of data buffers, and keeps the file for reading segments.
     *
     * @param[in] path              The raw data file.
     * @param[in] allow_maxshield   Accept unprocessed Internal Active Shielding (MaxShield) data.
     *
     * @return The raw data, or why the file cannot be read: Io if it cannot be opened, NotFound if it
     *         holds no raw data, Format if it is damaged or holds MaxShield data that is not allowed.
     */
    [[nodiscard]] static mne::Expected<FiffRawData> read(const std::filesystem::path& path, bool allow_maxshield = false);

    //=========================================================================================================
    /**
     * Opens raw FIFF data held in bytes that are not a file, e.g. in memory, like read(path).
     *
     * @param[in] bytes             The raw data; kept for reading segments.
     * @param[in] allow_maxshield   Accept unprocessed Internal Active Shielding (MaxShield) data.
     *
     * @return The raw data, or why it cannot be read.
     */
    [[nodiscard]] static mne::Expected<FiffRawData> read(std::unique_ptr<mne::ByteStream> bytes, bool allow_maxshield = false);

    //=========================================================================================================
    /**
     * Resets to empty raw data.
     */
    void clear();

    /** @return Whether there is no recording. */
    bool empty() const
    {
        return first_samp == -1 && info.empty();
    }

    //=========================================================================================================
    /**
     * Read a specific raw data segment
     *
     * @param[out] data      returns the data matrix (channels x samples).
     * @param[out] times     returns the time values corresponding to the samples.
     * @param[in] from       first sample to include. If omitted, defaults to the first sample in data (optional).
     * @param[in] to         last sample to include. If omitted, defaults to the last sample in data (optional).
     * @param[in] sel        channel selection vector (optional).
     * @param[in] do_debug Print diagnostic output while reading.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_raw_segment(Eigen::MatrixXd& data,
                          Eigen::MatrixXd& times,
                          fiff_int_t from = -1,
                          fiff_int_t to = -1,
                          const Eigen::RowVectorXi& sel = defaultRowVectorXi,
                          bool do_debug = false) const;

    //=========================================================================================================
    /**
     * Read a specific raw data segment
     *
     * @param[out] data      returns the data matrix (channels x samples).
     * @param[out] times     returns the time values corresponding to the samples.
     * @param[out] multSegment used multiplication matrix (compensator,projection,calibration).
     * @param[in] from       first sample to include. If omitted, defaults to the first sample in data (optional).
     * @param[in] to         last sample to include. If omitted, defaults to the last sample in data (optional).
     * @param[in] sel        channel selection vector (optional).
     * @param[in] do_debug Print diagnostic output while reading.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_raw_segment(Eigen::MatrixXd& data,
                          Eigen::MatrixXd& times,
                          Eigen::SparseMatrix<double>& multSegment,
                          fiff_int_t from = -1,
                          fiff_int_t to = -1,
                          const Eigen::RowVectorXi& sel = defaultRowVectorXi,
                          bool do_debug = false) const;

    //=========================================================================================================
    /**
     * Read a specific raw data segment
     *
     * @param[out] data      returns the data matrix (channels x samples).
     * @param[out] times     returns the time values corresponding to the samples.
     * @param[in] from       starting time of the segment in seconds.
     * @param[in] to         end time of the segment in seconds.
     * @param[in] sel        optional channel selection vector.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_raw_segment_times(Eigen::MatrixXd& data,
                                Eigen::MatrixXd& times,
                                float from,
                                float to,
                                const Eigen::RowVectorXi& sel = defaultRowVectorXi) const;

    //=========================================================================================================
    /**
     * Save raw data to a FIFF file, optionally with decimation and channel picking.
     * Ported from save.c (MNE-C).
     *
     * @param[in] path      The file to write.
     * @param[in] picks     Channel indices to include (empty = all channels).
     * @param[in] decim     Decimation factor (1 = no decimation).
     * @param[in] from      First sample to save (-1 = from start of raw data).
     * @param[in] to        Last sample to save (-1 = to end of raw data).
     *
     * @return true on success.
     */
    bool save(const std::filesystem::path& path,
              const Eigen::RowVectorXi& picks = Eigen::RowVectorXi(),
              int decim = 1,
              int from = -1,
              int to = -1) const;

    //=========================================================================================================
    /**
     * Activates the SSP projectors of the measurement info and builds the operator read() applies, as
     * mne.io.Raw.apply_proj prepares it.
     *
     * @return The rank of the operator; 0 when there are no projectors or none applies to these channels.
     */
    int activate_projections();

    //=========================================================================================================
    /**
     * Sets up the CTF/4D compensator that read() applies to bring the data to another gradient compensation
     * grade, as mne.io.Raw.apply_gradient_compensation does.
     *
     * @param[in] grade   The compensation grade wanted (0 for none).
     *
     * @return Nothing, or why the compensator cannot be made (Format).
     */
    [[nodiscard]] mne::Expected<void> apply_gradient_compensation(int grade);

public:
    std::unique_ptr<FiffStream> file; /**< replaces fid. */
    FiffInfo info;                    /**< Fiff measurement information. */
    fiff_int_t first_samp = -1;       /**< First sample number. */
    fiff_int_t last_samp = -1;        /**< Last sample number. */
    Eigen::RowVectorXd cals;          /**< Calibration values. */
    std::vector<FiffRawDir> rawdir;   /**< The data buffers and skips, in sample order. */
    Eigen::MatrixXd proj;             /**< SSP operator to apply to the data. */
    FiffCtfComp comp;                 /**< Compensator. */

private:
    static mne::Expected<FiffRawData> read(std::unique_ptr<FiffStream> stream, bool allow_maxshield);
};
} // NAMESPACE

#endif // FIFF_RAW_DATA_H
