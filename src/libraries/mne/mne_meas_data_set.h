//=============================================================================================================
/**
 * @file     mne_meas_data_set.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNEMeasDataSet class declaration.
 *
 */

#ifndef MNE_MEAS_DATA_SET_H
#define MNE_MEAS_DATA_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Single measurement epoch or average within MNEMeasData.
 *
 * Replaces @c *mneMeasDataSet / @c mneMeasDataSetRec from MNE-C @c mne_types.h.
 * Holds the measured data matrix, optional projector / whitened / filtered copies,
 * baseline values, and per-epoch metadata (number of averages, time range, etc.).
 */
class MNESHARED_EXPORT MNEMeasDataSet
{
public:
    typedef QSharedPointer<MNEMeasDataSet> SPtr;              /**< Shared pointer type for MNEMeasDataSet. */
    typedef QSharedPointer<const MNEMeasDataSet> ConstSPtr;   /**< Const shared pointer type for MNEMeasDataSet. */

    //=========================================================================================================
    /**
     * @brief Constructs an empty measurement data set.
     *
     * Refactored from @c mne_new_meas_data_set (mne_read_data.c).
     */
    MNEMeasDataSet();

    //=========================================================================================================
    /**
     * @brief Destroys the measurement data set and frees all owned C-style arrays.
     *
     * Refactored from @c mne_free_meas_data_set (mne_read_data.c).
     */
    ~MNEMeasDataSet();

    //=========================================================================================================
    /**
     * Pick signal values at a specified time point using linear interpolation.
     *
     * Reads from the data matrix (time-by-time layout: data[sample][channel])
     * and writes interpolated values into the output array.
     *
     * Refactored: mne_get_values_from_data (mne_get_values.c)
     *
     * @param[in]  time     Target time point (seconds).
     * @param[in]  integ    Integration window width (seconds).
     * @param[in]  nch      Number of channels to pick.
     * @param[in]  use_abs  If true, take absolute values before averaging.
     * @param[out] value    Output array of picked values (nch elements).
     * @return 0 on success, -1 on error.
     */
    int getValuesAtTime(float time, float integ, int nch, bool use_abs, float *value) const;

    //=========================================================================================================
    /**
     * Pick signal values at a specified time point using linear interpolation (channel-major layout).
     *
     * Reads from a channel-by-time data matrix (data[channel][sample]).
     *
     * Refactored: mne_get_values_from_data_ch (mne_get_values.c)
     *
     * @param[in]  time     Target time point (seconds).
     * @param[in]  integ    Integration window width (seconds).
     * @param[in]  data     Data matrix (channel-by-time layout).
     * @param[in]  nsamp    Number of time samples.
     * @param[in]  nch      Number of channels.
     * @param[in]  tmin     Time of first sample (seconds).
     * @param[in]  sfreq    Sampling frequency (Hz).
     * @param[in]  use_abs  If true, take absolute values before averaging.
     * @param[out] value    Output array of picked values (nch elements).
     * @return 0 on success, -1 on error.
     */
    static int getValuesFromChannelData(float time, float integ, float **data, int nsamp, int nch,
                                        float tmin, float sfreq, bool use_abs, float *value);

public:
    QString              comment;       /**< Comment / description associated with this data set. */
    Eigen::MatrixXf      data;          /**< Measured data matrix [np x nchan] (time-major layout). */
    Eigen::MatrixXf      data_proj;     /**< Data after SSP projection (kept separately for some programs). */
    Eigen::MatrixXf      data_filt;     /**< Optionally filtered copy of the data. */
    Eigen::MatrixXf      data_white;    /**< Whitened data (noise-normalised). */
    Eigen::VectorXf      stim14;        /**< Samples from the digital stimulus / trigger channel. */
    int                  first;         /**< First sample index (for raw-data processing). */
    int                  np;            /**< Number of time samples. */
    int                  nave;          /**< Number of averaged responses. */
    int                  kind;          /**< FIFF aspect kind (e.g. FIFFV_ASPECT_AVERAGE). */
    float                tmin;          /**< Start time of the epoch (seconds). */
    float                tstep;         /**< Sampling interval (seconds). */
    Eigen::VectorXf      baselines;     /**< Per-channel baseline offsets currently applied. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNE_MEAS_DATA_SET_H
