//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file compute_fwd.h
 * @since 2026
 * @date  March 2026
 * @brief Top-level driver that assembles the MEG/EEG lead-field matrix G from a source space, sensor array and volume-conductor model.
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

#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_types.h>
#include <fiff/fiff_info_base.h>

#include "../fwd_coil_set.h"
#include "../fwd_eeg_sphere_model_set.h"
#include "../fwd_bem_model.h"

#include <mne/mne_ctf_comp_data_set.h>
#include <mne/mne_source_space.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedDataPointer>
#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffNamedMatrix;
}

namespace MNELIB {
    class MNEForwardSolution;
}

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
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
    std::unique_ptr<MNELIB::MNEForwardSolution> calculateFwd();

    //=========================================================================================================
    /**
     * Update the head position with a new device-to-head transform and
     * recompute the MEG portion of the forward solution.
     *
     * @param[in] transDevHead   The updated device-to-head coordinate transform.
     * @param[in,out] fwd        The forward solution to update in place.
     * @return True on success, false on error.
     */
    bool updateHeadPos(const FIFFLIB::FiffCoordTrans& transDevHead, MNELIB::MNEForwardSolution& fwd);

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
    void populateMetadata(MNELIB::MNEForwardSolution& fwd);

    //=========================================================================================================
    // Computation state
    //=========================================================================================================

    std::vector<MNELIB::MNESourceSpace::UPtr> m_spaces;     /**< Source spaces. */
    int m_iNSource = 0;                                 /**< Number of active source points. */
    FwdCoilSet::UPtr m_templates;                       /**< Template coil set. */
    FwdCoilSet::UPtr m_megcoils;                        /**< MEG coil set. */
    FwdCoilSet::UPtr m_compcoils;                       /**< Compensator coil set. */
    FwdCoilSet::UPtr m_eegels;                          /**< EEG electrode set. */
    MNELIB::MNECTFCompDataSet::UPtr m_compData;         /**< CTF compensation data. */
    FwdEegSphereModelSet::UPtr m_eegModels;             /**< EEG sphere model set. */
    FwdEegSphereModel::UPtr m_eegModel;                 /**< Active EEG sphere model. */
    FwdBemModel::UPtr m_bemModel;                       /**< BEM model. */

    QList<FIFFLIB::FiffChInfo> m_listMegChs;             /**< MEG channel information. */
    QList<FIFFLIB::FiffChInfo> m_listEegChs;             /**< EEG channel information. */
    QList<FIFFLIB::FiffChInfo> m_listCompChs;            /**< Compensator channel list. */
    int m_iNChan = 0;                                    /**< Number of channels. */

    FIFFLIB::FiffId m_mri_id;                            /**< MRI file ID. */
    FIFFLIB::FiffId m_meas_id;                           /**< Measurement ID. */
    FIFFLIB::FiffCoordTrans m_mri_head_t;                /**< MRI-to-head transform. */
    FIFFLIB::FiffCoordTrans m_meg_head_t;                /**< MEG-to-head transform. */

    QSharedPointer<FIFFLIB::FiffInfoBase> m_pInfoBase;   /**< Measurement info. */
    std::shared_ptr<ComputeFwdSettings> m_pSettings;     /**< Forward computation settings. */

    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> m_meg_forward;        /**< MEG forward matrix. */
    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> m_meg_forward_grad;   /**< MEG gradient forward matrix. */
    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> m_eeg_forward;        /**< EEG forward matrix. */
    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> m_eeg_forward_grad;   /**< EEG gradient forward matrix. */

    QString m_qPath;                                     /**< Coil definition file path. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace FWDLIB

#endif // COMPUTE_FWD_H
