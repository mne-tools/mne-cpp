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
 * @brief    Compute Forward Setting class declaration.
 *
 */

#ifndef COMPUTEFWD_H
#define COMPUTEFWD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fwd_global.h"
#include "compute_fwd_settings.h"

#include <fiff/c/fiff_coord_trans_old.h>
#include "../fwd_coil_set.h"
#include <mne/c/mne_ctf_comp_data_set.h>
#include "../fwd_eeg_sphere_model_set.h"
#include "../fwd_bem_model.h"

#include <mne/c/mne_named_matrix.h>
#include <mne/c/mne_nearest.h>
#include <mne/c/mne_source_space_old.h>
#include <mne/mne_forwardsolution.h>

#include <fiff/c/fiff_sparse_matrix.h>

#include <fiff/fiff_types.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

#include <QCoreApplication>
#include <QFile>
#include <QDir>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
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
 * Implements the compute forward solution
 *
 * @brief Compute Forward implementation
 */
class FWDSHARED_EXPORT ComputeFwd
{
public:
    typedef QSharedPointer<ComputeFwd> SPtr;             /**< Shared pointer type for ComputeFwd. */
    typedef QSharedPointer<const ComputeFwd> ConstSPtr;  /**< Const shared pointer type for ComputeFwd. */

    //=========================================================================================================
    /**
     * Default Constructor
     * @param[in] p_settings        The pointer that contains the setting information.
     */
    explicit ComputeFwd(ComputeFwdSettings::SPtr pSettings);

    //=========================================================================================================
    /**
     * Destructs the Compute Forward solution class
     */
    virtual ~ComputeFwd();

    //=========================================================================================================
    /**
     * calculate Forward solution
     */
    void calculateFwd();

    //=========================================================================================================
    /**
     * Update the heaposition with meg_head_t and recalculate the forward solution for meg
     * @param[in] transDevHeadOld        The meg <-> head transformation to use for updating head position.
     */
    void updateHeadPos(FIFFLIB::FiffCoordTransOld* transDevHeadOld);

    //=========================================================================================================
    /**
     * Store Forward solution with given name. It defaults the name specified in
     * ComputeFwdSettings::settings->solname.
     * @param[in] sSolName        The file name to store the currnt forward solution.
     *
     */
    void storeFwd(const QString& sSolName = "default");

    // ToDo: make MNEForwardSolution the main output for the solution
    // QSharedPointer<MNELIB::MNEForwardSolution> fwdSolution;  /**< MNE Forward solution that contains all results. */

    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> sol;           /**< Forward solution (will be part of fwdSolution once rafactored). */
    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> sol_grad;      /**< Forward solution (Grad) (will be part of fwdSolution once rafactored). */

    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> m_meg_forward;            /**< The MEG forward calculation. */
    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> m_meg_forward_grad;       /**< The MEG gradient forward calculation*/
    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> m_eeg_forward;            /**< The EEG forward calculation. */
    QSharedDataPointer<FIFFLIB::FiffNamedMatrix> m_eeg_forward_grad;       /**< The EEG gradient forward calculation*/

    QString qPath;
    QFile file;

private:
    //=========================================================================================================
    /**
     * init the member variables
     */
    void initFwd();

    MNELIB::MneSourceSpaceOld **m_spaces;           /**< Source spaces. */
    int m_iNSpace;                                  /**< The number of source spaces. */
    int m_iNSource;                                 /**< Number of source space points. */
    FwdCoilSet* m_templates;                        /**< The template coil set. */
    FwdCoilSet* m_megcoils;                         /**< The MEG coil set. */
    FwdCoilSet* m_compcoils;                        /**< The compensator coil set. */
    FwdCoilSet* m_eegels;                           /**< The EEG eceltrode set. */
    MNELIB::MneCTFCompDataSet *m_compData;          /**< The compensator data. */
    FwdEegSphereModelSet* m_eegModels;              /**< The EEG model set. */
    FwdEegSphereModel* m_eegModel;                  /**< The EEG model. */
    FwdBemModel *m_bemModel;                        /**< BEM model definition. */
    Eigen::Vector3f *m_r0;                          /**< The Sphere model origin. */

    QList<FIFFLIB::FiffChInfo> m_listMegChs;        /**< The MEG channel information. */
    QList<FIFFLIB::FiffChInfo> m_listEegChs;        /**< The EEG channel information. */
    QList<FIFFLIB::FiffChInfo> m_listCompChs;       /**< The Compensator Channel List. */
    int m_iNChan;                                   /**< The number of channels. */

    FIFFLIB::fiffId m_mri_id;                       /**< The MRI ID. */
    FIFFLIB::FiffId m_meas_id;                      /**< The Measurement ID. */
    FIFFLIB::FiffCoordTransOld* m_mri_head_t;       /**< The MRI->head coordinate transformation. */
    FIFFLIB::FiffCoordTransOld* m_meg_head_t;       /**< The MEG->head coordinate transformation. */

    QSharedPointer<FIFFLIB::FiffInfoBase> m_pInfoBase;

    ComputeFwdSettings::SPtr m_pSettings;                /**< The settings for the forward calculation. */

    //=========================================================================================================
    /**
     * Read channelinformation and split into lists for meg/eeg/comp + read m_meg_head_t
     * @param[in] pFiffInfo            The FiffInfo to read from.
     * @param[in, out] listMegCh           The MEG channel list.
     * @param[in, out] iNMeg               The number of MEG channels.
     * @param[in, out] listMegComp         The compensator channel list.
     * @param[in, out] iNMegCmp            The number of compensator channels.
     * @param[in, out] listEegCh           The EEG channel list.
     * @param[in, out] iNEeg               The number of EEG channels.
     * @param[in, out] transDevHeadOld     The meg <-> head transformation.
     * @param[in, out] id                  The FiffID.
     *
     */
    int mne_read_meg_comp_eeg_ch_info_41(FIFFLIB::FiffInfoBase::SPtr pFiffInfoBase,
                                         QList<FIFFLIB::FiffChInfo>& listMegCh,
                                         int& iNMeg,
                                         QList<FIFFLIB::FiffChInfo>& listMegComp,
                                         int& iNMegCmp,
                                         QList<FIFFLIB::FiffChInfo>& listEegCh,
                                         int& iNEeg,
                                         FIFFLIB::FiffCoordTransOld** transDevHeadOld,
                                         FIFFLIB::FiffId& id);

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // COMPUTEFWDSETTINGS_H
