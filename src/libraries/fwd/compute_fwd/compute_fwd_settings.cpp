//=============================================================================================================
/**
 * @file     compute_fwd_settings.cpp
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
 * @brief    ComputeFwdSettings class definition.
 *
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
