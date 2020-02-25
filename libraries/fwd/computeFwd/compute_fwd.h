//=============================================================================================================
/**
 * @file     compute_fwd.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
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
    void calculateFwd() const;

    QString qPath;
    QFile file;

private:
    ComputeFwdSettings* settings;

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} //NAMESPACE

#endif // COMPUTEFWDSETTINGS_H
