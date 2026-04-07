//=============================================================================================================
/**
 * @file     mne_meas_data.h
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
 * @brief    MNEMeasData class declaration.
 *
 */

#ifndef MNE_MEAS_DATA_H
#define MNE_MEAS_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include <fiff/fiff_types.h>

#include "mne_types.h"
#include "mne_raw_data.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB { class FiffCoordTrans; }

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MNEInverseOperator;
class MNEMeasDataSet;
class MNENamedMatrix;

//=============================================================================================================
/**
 * @brief Measurement data container for MNE inverse and dipole-fit computations.
 *
 * Replaces @c *mneMeasData / @c mneMeasDataRec from MNE-C @c mne_types.h.
 * Holds channel information, coordinate transforms, projection operators,
 * and one or more MNEMeasDataSet epochs loaded from a FIFF evoked-response file.
 */
class MNESHARED_EXPORT MNEMeasData
{
public:
    typedef QSharedPointer<MNEMeasData> SPtr;              /**< Shared pointer type for MNEMeasData. */
    typedef QSharedPointer<const MNEMeasData> ConstSPtr;   /**< Const shared pointer type for MNEMeasData. */

    //=========================================================================================================
    /**
     * @brief Constructs an empty measurement data container.
     *
     * Refactored from @c mne_new_meas_data (mne_read_data.c).
     */
    MNEMeasData();

    //=========================================================================================================
    /**
     * @brief Destroys the measurement data and all owned data sets.
     *
     * Refactored from @c mne_free_meas_data (mne_read_data.c).
     */
    ~MNEMeasData();

    //=========================================================================================================
    /**
     * @brief Adjust baseline offset of the current data set.
     *
     * Subtracts the mean over [@p bmin, @p bmax] from every channel and
     * accumulates the offset in MNEMeasDataSet::baselines.
     *
     * Refactored from @c mne_adjust_baselines (mne_apply_baselines.c).
     *
     * @param[in] bmin   Baseline window start (seconds).
     * @param[in] bmax   Baseline window end (seconds).
     */
    void adjust_baselines(float bmin, float bmax);

    //============================= mne_read_data.c =============================

    /**
     * @brief Read an evoked-response data set and append it to an existing container.
     *
     * If @p add_to is @c nullptr a new MNEMeasData is created; otherwise the
     * loaded epoch is appended to @p add_to's set list.
     *
     * Refactored from @c mne_read_meas_data_add (mne_read_data.c).
     *
     * @param[in] name      Path to the FIFF measurement file.
     * @param[in] set       1-based data-set index to load.
     * @param[in] op        Inverse operator for consistency checks (may be @c nullptr).
     * @param[in] fwd       Forward operator for consistency checks (may be @c nullptr).
     * @param[in] namesp    Explicit channel name list (fallback if @p op and @p fwd are @c nullptr).
     * @param[in] nnamesp   Number of entries in @p namesp.
     * @param[in] add_to    Existing container to append to, or @c nullptr to create a new one.
     * @return Pointer to the (new or existing) container, or @c nullptr on failure.
     */
    static MNEMeasData* mne_read_meas_data_add(const QString&       name,
                                       int                  set,
                                       MNEInverseOperator*   op,
                                       MNENamedMatrix*       fwd,
                                       const QStringList&   namesp,
                                       int                  nnamesp,
                                       MNEMeasData*          add_to);

    /**
     * @brief Read an evoked-response data set into a new container.
     *
     * Convenience wrapper around mne_read_meas_data_add() with @p add_to = @c nullptr.
     *
     * @param[in] name      Path to the FIFF measurement file.
     * @param[in] set       1-based data-set index to load.
     * @param[in] op        Inverse operator for consistency checks (may be @c nullptr).
     * @param[in] fwd       Forward operator for consistency checks (may be @c nullptr).
     * @param[in] namesp    Explicit channel name list.
     * @param[in] nnamesp   Number of entries in @p namesp.
     * @return Pointer to the new container, or @c nullptr on failure.
     */
    static MNEMeasData* mne_read_meas_data(const QString&       name,
                                   int                  set,
                                   MNEInverseOperator*  op,
                                   MNENamedMatrix*      fwd,
                                   const QStringList&   namesp,
                                   int                  nnamesp);

public:
    QString                 filename;   /**< Path to the source FIFF file. */
    FIFFLIB::FiffId          meas_id;    /**< Measurement block ID from the FIFF file. */
    FIFFLIB::FiffTime       meas_date;  /**< Measurement date / time stamp. */
    QList<FIFFLIB::FiffChInfo>     chs; /**< Channel information list. */
    std::unique_ptr<FIFFLIB::FiffCoordTrans> meg_head_t; /**< MEG device ↔ head coordinate transform. */
    std::unique_ptr<FIFFLIB::FiffCoordTrans> mri_head_t; /**< MRI ↔ head coordinate transform. */
    float                   sfreq;      /**< Sampling frequency (Hz). */
    int                     nchan;      /**< Number of channels. */
    float                   highpass;   /**< High-pass filter setting (Hz). */
    float                   lowpass;    /**< Low-pass filter setting (Hz). */
    std::unique_ptr<MNEProjOp>  proj; /**< SSP projection operator (may be @c nullptr). */
    std::unique_ptr<MNECTFCompDataSet> comp; /**< Software gradient compensation data. */
    MNEInverseOperator*     op;         /**< Associated inverse operator (not owned). */
    MNENamedMatrix*         fwd; /**< Forward operator for dipole fitting (not owned). */
    std::unique_ptr<MNERawData> raw; /**< Raw-data handle when data originates from a raw file. */
    mneChSelection          chsel; /**< Channel selection for raw-data access. */
    QStringList             badlist;    /**< List of bad channel names. */
    int                     nbad;       /**< Number of bad channels. */
    Eigen::VectorXi          bad;       /**< Per-channel bad flag array (0 = good, 1 = bad). */

    bool                    ch_major;   /**< If true, data rows are channels (not times). */
    QList<MNEMeasDataSet*>  sets;       /**< All loaded data-set epochs. */
    int                     nset;       /**< Number of loaded data sets. */
    MNEMeasDataSet*         current;    /**< Pointer to the currently active data set. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNE_MEAS_DATA_H
