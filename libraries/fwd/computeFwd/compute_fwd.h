//=============================================================================================================
/**
 * @file     compute_fwd.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
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
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================

/**
 * The struct containing the forward solution.
 */
struct FwdResult {
    MNELIB::MneSourceSpaceOld **spaces;             /* Source spaces */
    int nspace;                                     /* How many? */
    FwdCoilSet *megCoils;
    FwdCoilSet *comp_coils;
    MNELIB::MneCTFCompDataSet *comp_data;
    bool fixed_ori;                                 /* Use fixed-orientation dipoles */
    FwdBemModel *bem_model;                         /* BEM model definition */
    Eigen::Vector3f *r0;                            /* Sphere model origin */
    bool use_threads;                               /* Parallelize with threads? */
    MNELIB::MneNamedMatrix **megForward;            /* The results */
    MNELIB::MneNamedMatrix **megForwardGrad;
};

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
     * @param [in]  p_settings        The pointer that contains the setting information
     */
    explicit ComputeFwd(ComputeFwdSettings* p_settings);

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
     * Update the heaposition with meg_head_tand recalculate the forward solution for meg
     * @param [in] meg_head_t        The meg_head_t to use for updating head position
     */
    void updateHeadPos(FIFFLIB::FiffCoordTransOld* meg_head_t);

    //=========================================================================================================
    /**
     * store Forward solution
     */
    void storeFwd();

    QString qPath;
    QFile file;

private:
    //=========================================================================================================
    /**
     * init the result
     */
    void initFwd();

    MNELIB::MneSourceSpaceOld **spaces;             /* Source spaces */
    int nspace;                                     /* How many? */
    FwdCoilSet *megcoils;                           /* The MEG coil set */
    FwdCoilSet *compcoils;                          /* The compensator coil set */
    FwdCoilSet* eegels;                             /* The eeg eceltrode set */
    MNELIB::MneCTFCompDataSet *comp_data;           /* The compensator data */
    FwdEegSphereModelSet* eeg_models;               /* The EEG model set */
    FwdEegSphereModel* eeg_model;                   /* The EEG model */
    FwdBemModel *bem_model;                         /* BEM model definition */
    Eigen::Vector3f *r0;                            /* Sphere model origin */

    MNELIB::MneNamedMatrix *meg_forward;            /* The meg forward  */
    MNELIB::MneNamedMatrix *meg_forward_grad;
    MNELIB::MneNamedMatrix *eeg_forward;
    MNELIB::MneNamedMatrix *eeg_forward_grad;

    QList<FIFFLIB::FiffChInfo> megchs;              /* The MEG channel information */
    QList<FIFFLIB::FiffChInfo> eegchs;              /* The EEG channel information */

    FIFFLIB::fiffId mri_id;
    FIFFLIB::FiffId meas_id;
    FIFFLIB::FiffCoordTransOld* mri_head_t;         /* MRI <-> head coordinate transformation */
    FIFFLIB::FiffCoordTransOld* meg_head_t;         /* MEG <-> head coordinate transformation */

    ComputeFwdSettings* settings;                   /* The settings for the forward calculation */

    //=========================================================================================================

    int readChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                     QList<FIFFLIB::FiffChInfo>& meg,	 /* MEG channels */
                     int& nmeg,
                     QList<FIFFLIB::FiffChInfo>& meg_comp,
                     int& nmeg_comp,
                     QList<FIFFLIB::FiffChInfo>& eeg,	 /* EEG channels */
                     int& neeg,
                     FIFFLIB::FiffCoordTransOld** meg_head_t,
                     FIFFLIB::FiffId& id);	         /* The measurement ID */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // COMPUTEFWDSETTINGS_H
