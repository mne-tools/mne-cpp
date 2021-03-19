//=============================================================================================================
/**
 * @file     fiff_raw_data.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    FiffRawData class declaration.
 *
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

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

class FiffRawData;

//=============================================================================================================
/**
 *Provides fiff raw measurement data, including I/O routines.
 *
 * @brief FIFF raw measurement data
 */
class FIFFSHARED_EXPORT FiffRawData
{
public:
    typedef QSharedPointer<FiffRawData> SPtr;               /**< Shared pointer type for FiffRawData. */
    typedef QSharedPointer<const FiffRawData> ConstSPtr;    /**< Const shared pointer type for FiffRawData. */

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
     * ### MNE toolbox root function ###: Definition of the fiff_read_raw_segment function
     *
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
     * ### MNE toolbox root function ###: Definition of the fiff_read_raw_segment function
     *
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
     * ### MNE toolbox root function ###: Definition of the fiff_read_raw_segment function
     *
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

public:
    FiffStream::SPtr file;      /**< replaces fid. */
    FiffInfo info;              /**< Fiff measurement information. */
    fiff_int_t first_samp;      /**< Do we have a skip ToDo... */
    fiff_int_t last_samp;       /**< Do we have a skip ToDo... */
    Eigen::RowVectorXd cals;    /**< Calibration values. ToDo: Check if RowVectorXd is enough. */
    QList<FiffRawDir> rawdir;   /**< Special fiff diretory entry for raw data. */
    Eigen::MatrixXd proj;       /**< SSP operator to apply to the data. */
    FiffCtfComp comp;           /**< Compensator. */


};
} // NAMESPACE

#endif // FIFF_RAW_DATA_H
