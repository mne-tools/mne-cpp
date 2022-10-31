//=============================================================================================================
/**
 * @file     fwd_thread_arg.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the Forward Thread Argument (FwdThreadArg) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_thread_arg.h"
#include <mne/c/mne_source_space_old.h>
#include "fwd_coil_set.h"
#include "fwd_bem_model.h"
#include "fwd_comp_data.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#define FREE_80(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_80(m) mne_free_cmatrix_80((m))

void mne_free_cmatrix_80 (float **m)
{
    if (m) {
        FREE_80(*m);
        FREE_80(m);
    }
}

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FWDLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdThreadArg::FwdThreadArg()
:res           (NULL)
,res_grad      (NULL)
,off           (0)
,field_pot     (NULL)
,vec_field_pot (NULL)
,field_pot_grad(NULL)
,coils_els     (NULL)
,client        (NULL)
,s             (NULL)
,fixed_ori     (FALSE)
,stat          (FAIL)
,comp          (-1)
{
}

//=============================================================================================================

FwdThreadArg::~FwdThreadArg()
{
}

//=============================================================================================================

FwdThreadArg *FwdThreadArg::create_eeg_multi_thread_duplicate(FwdThreadArg *one, bool bem_model)
/*
          * Create a duplicate to make the data structure thread safe
          * Do not duplicate read-only parts of the relevant structures
          */
{
    FwdThreadArg* res  = new FwdThreadArg;

     *res = *one;
    if (bem_model) {
        FwdBemModel*   new_bem  = new FwdBemModel;
        FwdBemModel*   orig_bem = (FwdBemModel*)res->client;

        *new_bem    = *orig_bem;
        new_bem->v0 = NULL;
        res->client = new_bem;
    }
    return res;
}

//=============================================================================================================

void FwdThreadArg::free_eeg_multi_thread_duplicate(FwdThreadArg *one, bool bem_model)
{
    if (!one){
        qDebug("Pointer passed is null. Returning early.");
        return;
    }
    if (bem_model) {
        FwdBemModel*    bem = (FwdBemModel*) one->client;
        FREE_80(bem->v0);
        FREE_80(bem);
    }
    one->client = NULL;
    if(one)
        delete one;
}

//=============================================================================================================

FwdThreadArg *FwdThreadArg::create_meg_multi_thread_duplicate(FwdThreadArg* one, bool bem_model)
/*
 * Create a duplicate to make the data structure thread safe
 * Do not duplicate read-only parts of the relevant structures
 */
{
    FwdThreadArg* res  = new FwdThreadArg;
    FwdCompData*  orig = (FwdCompData*)one->client;
    FwdCompData*  comp = NULL;

     *res = *one;
    res->client = comp = new FwdCompData;
     *comp = *orig;
    comp->work     = NULL;
    comp->vec_work = NULL;
    comp->set      = orig->set ? new MneCTFCompDataSet(*(orig->set)) : NULL;

    if (bem_model) {
        FwdBemModel*   new_bem  = new FwdBemModel();
        FwdBemModel*   orig_bem = (FwdBemModel*)comp->client;

        *new_bem     = *orig_bem;
        new_bem->v0  = NULL;
        comp->client = new_bem;
    }
    return res;
}

//=============================================================================================================

void FwdThreadArg::free_meg_multi_thread_duplicate(FwdThreadArg *one, bool bem_model)

{
    if (!one){
        qDebug("Pointer passed is null. Returning early.");
        return;
    }

    FwdCompData* comp = (FwdCompData*)one->client;

    FREE_80(comp->work);
    FREE_CMATRIX_80(comp->vec_work);
    if(comp->set)
        delete comp->set;

    if (bem_model) {
        FwdBemModel*    bem = (FwdBemModel*)comp->client;
        FREE_80(bem->v0);
        FREE_80(bem);
    }
    FREE_80(comp);
    one->client = NULL;
    if(one)
        delete one;
}
