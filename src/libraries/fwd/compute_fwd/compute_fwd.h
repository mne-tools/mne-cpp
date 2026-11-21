//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     compute_fwd.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February 2017
 * @brief    Top-level driver that assembles the MEG/EEG lead-field matrix G from a source space, sensor array and volume-conductor model.
 *
 * ComputeFwd is the C++ analogue of the @c mne_forward_solution
 * command-line tool: given a ComputeFwdSettings (source-space file, BEM
 * or sphere model, raw/measurement file, coil-definition file,
 * coordinate transforms, accuracy level), it loads every piece of input
 * data, sets up the requested volume-conductor model (BEM via
 * FwdBemModel or analytic sphere via FwdEegSphereModel), wraps the
 * resulting field/grad callbacks in a CTF/4D compensation layer when
 * needed, and runs the parallel dipole loop that fills G column by
 * column. The output @c MNEForwardSolution carries G plus all metadata
 * (source orientations, channel info, projection operators) that the
 * downstream inverse stage requires.
 *
 * @c updateHeadPos() supports head-motion-corrected averaging: it
 * re-evaluates only the MEG block of G after a new device-to-head
 * transform without re-touching the source space or BEM solution,
 * matching the behaviour of MNE-C @c mne_forward_solution -- update_head_pos.
 */

#ifndef COMPUTE_FWD_H
#define COMPUTE_FWD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fwd_global.h"
#include "compute_fwd_settings.h"
#include "../fwd_coil_set.h"
#include "../fwd_eeg_sphere_model_set.h"
#include "../fwd_bem_model.h"
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_types.h>
#include <fiff/fiff_info_base.h>
#include <mne/mne_ctf_comp_data_set.h>
#include <mne/mne_source_space.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <vector>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffNamedMatrix;
}

namespace mne
{
class MNEForwardSolution;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Implements the forward-solution computation driver.
 *
 * ComputeFwd is the worker/factory that takes a ComputeFwdSettings,
 * initialises all required data structures (coil definitions, BEM or
 * sphere model, source spaces, compensation data, coordinate transforms)
 * and assembles the MEG/EEG lead-field matrix @c G one dipole column at
 * a time. The computed MNEForwardSolution is returned by
 * @ref calculateFwd; @ref updateHeadPos rebuilds only the MEG block
 * after a new device-to-head transform.
 *
 * @brief Top-level driver for MEG/EEG forward-solution computation.
 *
 * @snippet ex_fwd_api/main.cpp compute_fwd_usage
 */
class FWDSHARED_EXPORT ComputeFwd
{
public:
    //=========================================================================================================
    /**
     * Constructs a ComputeFwd and initialises all data structures needed for the
     * forward computation according to @a pSettings.
     *
     * @param[in] pSettings   Shared pointer to the forward computation settings.
     */
    explicit ComputeFwd(std::shared_ptr<ComputeFwdSettings> pSettings);

    //=========================================================================================================
    /**
     * Destructor.
     */
    virtual ~ComputeFwd();

    //=========================================================================================================
    /**
     * Perform the forward calculation.
     *
     * @return The computed forward solution, or nullptr on error.
     */
    std::unique_ptr<mne::MNEForwardSolution> calculateFwd();

    //=========================================================================================================
    /**
     * Update the head position with a new device-to-head transform and
     * recompute the MEG portion of the forward solution.
     *
     * @param[in] transDevHead   The updated device-to-head coordinate transform.
     * @param[in,out] fwd        The forward solution to update in place.
     * @return True on success, false on error.
     */
    bool updateHeadPos(const mne::FiffCoordTrans& transDevHead, mne::MNEForwardSolution& fwd);

private:
    //=========================================================================================================
    /**
     * Read source spaces, coordinate transforms, channel information, coil
     * definitions, compensation data, and BEM model.  Populates all
     * computation state members and the public fields of fwdSolution.
     */
    void initFwd();

    //=========================================================================================================
    /**
     * Populate metadata fields of a forward solution from the current
     * computation state (settings, source spaces, channel info, etc.).
     */
    void populateMetadata(mne::MNEForwardSolution& fwd);

    //=========================================================================================================
    // Computation state
    //=========================================================================================================

    std::vector<std::unique_ptr<mne::MNESourceSpace>> m_spaces; /**< Source spaces. */
    int m_iNSource = 0;                                         /**< Number of active source points. */
    std::unique_ptr<FwdCoilSet> m_templates;                    /**< Template coil set. */
    std::unique_ptr<FwdCoilSet> m_megcoils;                     /**< MEG coil set. */
    std::unique_ptr<FwdCoilSet> m_compcoils;                    /**< Compensator coil set. */
    std::unique_ptr<FwdCoilSet> m_eegels;                       /**< EEG electrode set. */
    std::unique_ptr<mne::MNECTFCompDataSet> m_compData;         /**< CTF compensation data. */
    std::unique_ptr<FwdEegSphereModelSet> m_eegModels;          /**< EEG sphere model set. */
    std::unique_ptr<FwdEegSphereModel> m_eegModel;              /**< Active EEG sphere model. */
    std::unique_ptr<FwdBemModel> m_bemModel;                    /**< BEM model. */
    bool m_bInitialized = false;                                /**< Whether initFwd completed. */

    std::vector<mne::FiffChInfo> m_listMegChs;  /**< MEG channel information. */
    std::vector<mne::FiffChInfo> m_listEegChs;  /**< EEG channel information. */
    std::vector<mne::FiffChInfo> m_listCompChs; /**< Compensator channel list. */
    int m_iNChan = 0;                           /**< Number of channels. */

    mne::FiffId m_mri_id;             /**< MRI file ID. */
    mne::FiffId m_meas_id;            /**< Measurement ID. */
    mne::FiffCoordTrans m_mri_head_t; /**< MRI-to-head transform. */
    mne::FiffCoordTrans m_meg_head_t; /**< MEG-to-head transform. */

    std::shared_ptr<mne::FiffInfoBase> m_pInfoBase;  /**< Measurement info. */
    std::shared_ptr<ComputeFwdSettings> m_pSettings; /**< Forward computation settings. */

    mne::FiffNamedMatrix m_meg_forward;      /**< MEG forward matrix. */
    mne::FiffNamedMatrix m_meg_forward_grad; /**< MEG gradient forward matrix. */
    mne::FiffNamedMatrix m_eeg_forward;      /**< EEG forward matrix. */
    mne::FiffNamedMatrix m_eeg_forward_grad; /**< EEG gradient forward matrix. */

    std::filesystem::path m_qPath; /**< Coil definition file path. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace mne

#endif // COMPUTE_FWD_H
