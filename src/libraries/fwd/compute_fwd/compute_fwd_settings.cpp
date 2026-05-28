//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     compute_fwd_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    ComputeFwdSettings implementation — default initialisation and @c checkIntegrity validation of the @c mne_forward_solution parameter set consumed by ComputeFwd.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "compute_fwd_settings.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace FWDLIB;

//=============================================================================================================
// DEFINES
//=============================================================================================================

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

/**
 * Constructs a ComputeFwdSettings object with default member values.
 */
ComputeFwdSettings::ComputeFwdSettings()
{
    initMembers();
}

//=============================================================================================================

/**
 * Destroys the ComputeFwdSettings object.
 */
ComputeFwdSettings::~ComputeFwdSettings()
{
    //ToDo Garbage collection
}

//=============================================================================================================

/**
 * Validate that all required settings have been specified.
 *
 * Checks for source space name, MRI-to-head transform, measurement file,
 * solution output name, and that at least MEG or EEG is selected.
 */
void ComputeFwdSettings::checkIntegrity()
{
    if (srcname.isEmpty()) {
        qCritical("Source space name is missing. Use the --src option to specify it.");
        return;
    }
    if (!mri_head_ident) {
        if (mriname.isEmpty() && transname.isEmpty()) {
            qCritical("MRI <-> head coordinate transformation is missing. Use the --mri or --trans option to specify it.");
            return;
        }
    }
    if (measname.isEmpty()) {
        qCritical("Source of coil and electrode locations is missing. Use the --meas option to specify it.");
        return;
    }
    if (solname.isEmpty()) {
        qCritical("Solution name is missing. Use the --fwd option to specify it.");
        return;
    }
    if (! (include_meg || include_eeg)) {
        qCritical("Employ the --meg and --eeg options to select MEG and/or EEG");
        return;
    }
}

//=============================================================================================================

/**
 * Initialize all data members to their default values.
 */
void ComputeFwdSettings::initMembers()
{
    // Init origin
    r0 << 0.0f,0.0f,0.04f;

    mri_head_ident = false;
    filter_spaces = true;  
    accurate = false;      
    fixed_ori = false;     
    include_meg = false;
    include_eeg = false;
    compute_grad = false;
    mindist = 0.0f;        
    coord_frame = FIFFV_COORD_HEAD;
    do_all = false;
    nlabel = 0;

    eeg_sphere_rad = 0.09f;   
    scale_eeg_pos = false;    
    use_equiv_eeg = true;     
    use_threads = true;

    pFiffInfo = nullptr;
    meg_head_t = FiffCoordTrans();
}
