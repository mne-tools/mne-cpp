//=============================================================================================================
/**
 * @file     compute_fwd.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Ruben Dörfel <ruben.deorfel@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2017
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
 * @brief    ComputeFwd class declaration.
 *
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

#include <QSharedPointer>
#include <QSharedDataPointer>
#include <QString>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffNamedMatrix;
}

namespace FWDLIB {
    class FwdForwardSolution;
}

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * Implements the forward solution computation.
 *
 * ComputeFwd is a worker/factory that takes ComputeFwdSettings, initialises
 * all required data structures (coil definitions, BEM model, source spaces, …)
 * and performs the actual forward calculation.  The computed
 * FwdForwardSolution is returned by calculateFwd() and updateHeadPos().
 *
 * @brief Forward solution computation worker.
 */
class FWDSHARED_EXPORT ComputeFwd
{
public:
    typedef QSharedPointer<ComputeFwd> SPtr;             /**< Shared pointer type for ComputeFwd. */
    typedef QSharedPointer<const ComputeFwd> ConstSPtr;  /**< Const shared pointer type for ComputeFwd. */

    //=========================================================================================================
    /**
     * Constructs a ComputeFwd and initialises all data structures needed for the
     * forward computation according to @a pSettings.
     *
     * @param[in] pSettings   Shared pointer to the forward computation settings.
     */
    explicit ComputeFwd(ComputeFwdSettings::SPtr pSettings);

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
    std::unique_ptr<FwdForwardSolution> calculateFwd();

    //=========================================================================================================
    /**
     * Update the head position with a new device-to-head transform and
     * recompute the MEG portion of the forward solution.
     *
     * @param[in] transDevHead   The updated device-to-head coordinate transform.
     * @param[in,out] fwd        The forward solution to update in place.
     * @return True on success, false on error.
     */
    bool updateHeadPos(const FIFFLIB::FiffCoordTrans& transDevHead, FwdForwardSolution& fwd);

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
    void populateMetadata(FwdForwardSolution& fwd);

    //=========================================================================================================
    // Computation state
    //=========================================================================================================

    std::vector<std::unique_ptr<MNELIB::MNESourceSpace>> m_spaces;  /**< Source spaces. */
    int m_iNSource = 0;                                 /**< Number of active source points. */
    std::unique_ptr<FwdCoilSet> m_templates;            /**< Template coil set. */
    std::unique_ptr<FwdCoilSet> m_megcoils;             /**< MEG coil set. */
    std::unique_ptr<FwdCoilSet> m_compcoils;            /**< Compensator coil set. */
    std::unique_ptr<FwdCoilSet> m_eegels;               /**< EEG electrode set. */
    std::unique_ptr<MNELIB::MNECTFCompDataSet> m_compData; /**< CTF compensation data. */
    std::unique_ptr<FwdEegSphereModelSet> m_eegModels;  /**< EEG sphere model set. */
    std::unique_ptr<FwdEegSphereModel> m_eegModel;      /**< Active EEG sphere model. */
    std::unique_ptr<FwdBemModel> m_bemModel;            /**< BEM model. */

    QList<FIFFLIB::FiffChInfo> m_listMegChs;             /**< MEG channel information. */
    QList<FIFFLIB::FiffChInfo> m_listEegChs;             /**< EEG channel information. */
    QList<FIFFLIB::FiffChInfo> m_listCompChs;            /**< Compensator channel list. */
    int m_iNChan = 0;                                    /**< Number of channels. */

    FIFFLIB::FiffId m_mri_id;                            /**< MRI file ID. */
    FIFFLIB::FiffId m_meas_id;                           /**< Measurement ID. */
    FIFFLIB::FiffCoordTrans m_mri_head_t;                /**< MRI-to-head transform. */
    FIFFLIB::FiffCoordTrans m_meg_head_t;                /**< MEG-to-head transform. */

    QSharedPointer<FIFFLIB::FiffInfoBase> m_pInfoBase;   /**< Measurement info. */
    ComputeFwdSettings::SPtr m_pSettings;                /**< Forward computation settings. */

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
