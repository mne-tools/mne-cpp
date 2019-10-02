//=============================================================================================================
/**
* @file     compute_fwd.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
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


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

#include <QCoreApplication>
#include <QFile>
#include <QDir>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


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
    */
    explicit ComputeFwd(ComputeFwdSettings* p_settings);

    //=========================================================================================================
    /**
    * Destructs the Compute Forward solution class
    */
    virtual ~ComputeFwd();

    //ToDo split this function into init (with settings as parameter) and the actual fit function
    void calculateFwd();

    // TODO: This only temporary until we have the fwd dlibrary refactored. This is only done in order to provide easy testing in test_forward_solution.
    bool                res = false;
    MNELIB::MneSourceSpaceOld*  *spaces = NULL;  /* The source spaces */
    int                 nspace  = 0;
    int                 nsource = 0;     /* Number of source space points */

    FIFFLIB::FiffCoordTransOld* mri_head_t = NULL;   /* MRI <-> head coordinate transformation */
    FIFFLIB::FiffCoordTransOld* meg_head_t = NULL;   /* MEG <-> head coordinate transformation */

    FIFFLIB::fiffChInfo     megchs   = NULL; /* The MEG channel information */
    int            nmeg     = 0;
    FIFFLIB::fiffChInfo     eegchs   = NULL; /* The EEG channel information */
    int            neeg     = 0;
    FIFFLIB::fiffChInfo     compchs = NULL;
    int            ncomp    = 0;

    FwdCoilSet*             megcoils = NULL;     /* The coil descriptions */
    FwdCoilSet*             compcoils = NULL;    /* MEG compensation coils */
    MNELIB::MneCTFCompDataSet*      comp_data  = NULL;
    FwdCoilSet*             eegels = NULL;
    FwdEegSphereModelSet*   eeg_models = NULL;

    MNELIB::MneNamedMatrix* meg_forward      = NULL;    /* Result of the MEG forward calculation */
    MNELIB::MneNamedMatrix* eeg_forward      = NULL;    /* Result of the EEG forward calculation */
    MNELIB::MneNamedMatrix* meg_forward_grad = NULL;    /* Result of the MEG forward gradient calculation */
    MNELIB::MneNamedMatrix* eeg_forward_grad = NULL;    /* Result of the EEG forward gradient calculation */
    int            k;
    FIFFLIB::fiffId         mri_id  = NULL;
    FIFFLIB::fiffId         meas_id = NULL;
    FILE           *out = NULL;     /* Output filtered points here */

    FwdCoilSet*       templates = NULL;
    FwdEegSphereModel* eeg_model = NULL;
    FwdBemModel*       bem_model = NULL;

    QString qPath;
    QFile file;

private:
    ComputeFwdSettings* settings;

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} //NAMESPACE

#endif // COMPUTEFWDSETTINGS_H
