//=============================================================================================================
/**
 * @file     mne_meas_data_set.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
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
 * @brief    Definition of the MNE Meas Data Set (MneMeasDataSet) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_meas_data_set.h"
#include <mne/c/mne_mne_data.h>

#include <fiff/fiff_file.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;
using namespace FIFFLIB;

#define FREE_8(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_8(m) mne_free_cmatrix_8((m))

void mne_free_cmatrix_8 (float **m)
{
    if (m) {
        FREE_8(*m);
        FREE_8(m);
    }
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneMeasDataSet::MneMeasDataSet()
:data(NULL)
,data_filt(NULL)
,data_proj(NULL)
,data_white(NULL)
,stim14(NULL)
,first(0)
,np(0)
,nave(1)
,kind(FIFFV_ASPECT_AVERAGE)
,baselines(NULL)
,mne(NULL)
,user_data(NULL)
,user_data_free(NULL)
{
}

//=============================================================================================================

MneMeasDataSet::~MneMeasDataSet()
{
    FREE_CMATRIX_8(data);
    FREE_CMATRIX_8(data_proj);
    FREE_CMATRIX_8(data_filt);
    FREE_CMATRIX_8(data_white);
    FREE_8(stim14);
    comment.clear();
    FREE_8(baselines);
    if(mne)
        delete mne;
    if (user_data && user_data_free)
        user_data_free(user_data);
}
