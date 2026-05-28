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
 * @ref FiffRawData represents a continuous (``raw'') Neuromag recording
 * as stored under @c FIFFB_RAW_DATA / @c FIFFB_CONTINUOUS_DATA: a
 * @ref FiffInfo describing the channels and acquisition setup, a list of
 * @ref FiffRawDir entries pointing at each @c FIFF_DATA_BUFFER tag, the
 * first and last sample indices, the per-channel calibration vector and a
 * @ref FiffStream handle for on-demand reads. The
 * @ref FiffStream::read_raw_segment family uses that directory to seek
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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QSharedPointer>
#include <QString>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
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
 */
class FIFFSHARED_EXPORT FiffRawData
{
public:
    using SPtr = QSharedPointer<FiffRawData>;            /**< Shared pointer type for FiffRawData. */
    using ConstSPtr = QSharedPointer<const FiffRawData>; /**< Const shared pointer type for FiffRawData. */
    using UPtr = std::unique_ptr<FiffRawData>;             /**< Unique pointer type for FiffRawData. */
    using ConstUPtr = std::unique_ptr<const FiffRawData>;  /**< Const unique pointer type for FiffRawData. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    FiffRawData();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffRawData  FIFF raw measurement which should be copied.
     */
    FiffRawData(const FiffRawData &p_FiffRawData);

    //=========================================================================================================
    /**
     * Constructs fiff raw data, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read the raw data from .
     */
    FiffRawData(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Constructs fiff raw data, by reading from a IO device with specified endianness.
     *
     * @param[in] p_IODevice       IO device to read the raw data from.
     * @param[in] b_littleEndian   If true, read data as little-endian.
     */
    FiffRawData(QIODevice &p_IODevice, bool b_littleEndian);

    //=========================================================================================================
    /**
     * Destroys the FiffInfo.
     */
    ~FiffRawData();

    //=========================================================================================================
    /**
     * Initializes the fiff raw measurement data.
     */
    void clear();

    //=========================================================================================================
    /**
     * True if fiff raw data are empty.
     *
     * @return true if fiff raw data are empty.
     */
    inline bool isEmpty() const
    {
        return first_samp == -1 && info.isEmpty();
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
     * @param[in] p_IODevice Output device to write to.
     * @param[in] picks     Channel indices to include (empty = all channels).
     * @param[in] decim     Decimation factor (1 = no decimation).
     * @param[in] from      First sample to save (-1 = from start of raw data).
     * @param[in] to        Last sample to save (-1 = to end of raw data).
     *
     * @return true on success.
     */
    bool save(QIODevice &p_IODevice,
              const Eigen::RowVectorXi &picks = Eigen::RowVectorXi(),
              int decim = 1,
              int from = -1,
              int to = -1) const;

public:
    FiffStream::SPtr file;      /**< replaces fid. */
    FiffInfo info;              /**< Fiff measurement information. */
    fiff_int_t first_samp;      /**< First sample number. */
    fiff_int_t last_samp;       /**< Last sample number. */
    Eigen::RowVectorXd cals;    /**< Calibration values. */
    QList<FiffRawDir> rawdir;   /**< Special fiff directory entry for raw data. */
    Eigen::MatrixXd proj;       /**< SSP operator to apply to the data. */
    FiffCtfComp comp;           /**< Compensator. */

};
} // NAMESPACE

#endif // FIFF_RAW_DATA_H
