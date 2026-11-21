//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_meas_data.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     January 2017
 * @brief    Legacy MNE-C measurement-data container assembling raw/evoked sets and their projection state.
 *
 * @ref mne::MNEMeasData mirrors @c mneMeasDataRec from the original
 * MNE C tooling. It bundles an @ref mne::FiffInfo with a list of
 * @ref mne::MNEMeasDataSet entries (one per averaging condition) and the
 * active projection / CTF compensation state. Kept verbatim so ported
 * C utilities (@c mne_inverse_operator, @c mne_compute_raw_inverse,
 * ...) continue to operate against the same in-memory schema.
 */

#ifndef MNE_MEAS_DATA_H
#define MNE_MEAS_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_raw_data.h"
#include <fiff/fiff_types.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>
#include <memory>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffCoordTrans;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// mne FORWARD DECLARATIONS
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
 *
 * @snippet ex_mne_api/main.cpp mne_meas_data_usage
 */
class MNESHARED_EXPORT MNEMeasData
{
public:
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
     * The loaded epoch is appended to @p add_to's set list; its channel list
     * selects the channels read from the file.
     *
     * Refactored from @c mne_read_meas_data_add (mne_read_data.c).
     *
     * @param[in] name      Path to the FIFF measurement file.
     * @param[in] set       1-based data-set index to load.
     * @param[in] op        Inverse operator for consistency checks (may be @c nullptr).
     * @param[in] fwd       Forward operator for consistency checks (may be @c nullptr).
     * @param[in] namesp    Explicit channel name list (fallback if @p op and @p fwd are @c nullptr).
     * @param[in] nnamesp   Number of entries in @p namesp.
     * @param[in,out] add_to Existing container to append to.
     * @return True if the data set was read and appended.
     */
    static bool mne_read_meas_data_add(const std::filesystem::path& name,
                                       int set,
                                       MNEInverseOperator* op,
                                       MNENamedMatrix* fwd,
                                       const StringList& namesp,
                                       int nnamesp,
                                       MNEMeasData& add_to);

    /**
     * @brief Read an evoked-response data set into a new container.
     *
     *
     * @param[in] name      Path to the FIFF measurement file.
     * @param[in] set       1-based data-set index to load.
     * @param[in] op        Inverse operator for consistency checks (may be @c nullptr).
     * @param[in] fwd       Forward operator for consistency checks (may be @c nullptr).
     * @param[in] namesp    Explicit channel name list.
     * @param[in] nnamesp   Number of entries in @p namesp.
     * @return The new container, or @c nullptr on failure.
     */
    static std::unique_ptr<MNEMeasData> mne_read_meas_data(const std::filesystem::path& name,
                                                           int set,
                                                           MNEInverseOperator* op,
                                                           MNENamedMatrix* fwd,
                                                           const StringList& namesp,
                                                           int nnamesp);

private:
    static bool read_evoked_set(const std::filesystem::path& name,
                                int set,
                                MNEInverseOperator* op,
                                MNENamedMatrix* fwd,
                                const StringList& namesp,
                                int nnamesp,
                                MNEMeasData& target,
                                bool fresh);

public:
    std::string filename;                            /**< Path to the source FIFF file. */
    mne::FiffId meas_id;                             /**< Measurement block ID from the FIFF file. */
    mne::FiffTime meas_date;                         /**< Measurement date / time stamp. */
    std::vector<mne::FiffChInfo> chs;                /**< Channel information list. */
    std::unique_ptr<mne::FiffCoordTrans> meg_head_t; /**< MEG device ↔ head coordinate transform. */
    std::unique_ptr<mne::FiffCoordTrans> mri_head_t; /**< MRI ↔ head coordinate transform. */
    float sfreq;                                     /**< Sampling frequency (Hz). */
    int nchan;                                       /**< Number of channels. */
    float highpass;                                  /**< High-pass filter setting (Hz). */
    float lowpass;                                   /**< Low-pass filter setting (Hz). */
    std::unique_ptr<MNEProjOp> proj;                 /**< SSP projection operator (may be @c nullptr). */
    std::unique_ptr<MNECTFCompDataSet> comp;         /**< Software gradient compensation data. */
    MNEInverseOperator* op;                          /**< Associated inverse operator (not owned). */
    MNENamedMatrix* fwd;                             /**< Forward operator for dipole fitting (not owned). */
    std::unique_ptr<MNERawData> raw;                 /**< Raw-data handle when data originates from a raw file. */
    StringList badlist;                              /**< List of bad channel names. */
    Eigen::VectorXi bad;                             /**< Per-channel bad flag array (0 = good, 1 = bad). */

    bool ch_major;                                     /**< If true, data rows are channels (not times). */
    std::vector<std::unique_ptr<MNEMeasDataSet>> sets; /**< All loaded data-set epochs. */
    int nset;                                          /**< Number of loaded data sets. */
    MNEMeasDataSet* current;                           /**< Currently active data set (owned by @ref sets). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNE_MEAS_DATA_H
