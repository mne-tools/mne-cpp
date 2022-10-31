//=============================================================================================================
/**
 * @file     fwd_comp_data.cpp
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
 * @brief    Definition of the FwdCompData Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_comp_data.h"

#include <mne/c/mne_ctf_comp_data_set.h>
#include <fiff/fiff_types.h>

#include <iostream>

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

#define X_60 0
#define Y_60 1
#define Z_60 2

#define MALLOC_60(x,t) (t *)malloc((x)*sizeof(t))

#define ALLOC_CMATRIX_60(x,y) mne_cmatrix_60((x),(y))

#define FREE_60(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_60(m) mne_free_cmatrix_60((m))

void mne_free_cmatrix_60 (float **m)
{
    if (m) {
        FREE_60(*m);
        FREE_60(m);
    }
}

static void matrix_error_60 (int kind, int nr, int nc)
{
    if (kind == 1)
        printf("Failed to allocate memory pointers for a %d x %d matrix\n",nr,nc);
    else if (kind == 2)
        printf("Failed to allocate memory for a %d x %d matrix\n",nr,nc);
    else
        printf("Allocation error for a %d x %d matrix\n",nr,nc);
    if (sizeof(void *) == 4) {
        printf("This is probably because you seem to be using a computer with 32-bit architecture.\n");
        printf("Please consider moving to a 64-bit platform.");
    }
    printf("Cannot continue. Sorry.\n");
    exit(1);
}

float **mne_cmatrix_60 (int nr,int nc)
{
    int i;
    float **m;
    float *whole;

    m = MALLOC_60(nr,float *);
    if (!m) matrix_error_60(1,nr,nc);
    whole = MALLOC_60(nr*nc,float);
    if (!whole) matrix_error_60(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdCompData::FwdCompData()
:comp_coils (NULL)
,field      (NULL)
,vec_field  (NULL)
,field_grad (NULL)
,client     (NULL)
,client_free(NULL)
,set        (NULL)
,work       (NULL)
,vec_work   (NULL)
{
}

//=============================================================================================================

FwdCompData::~FwdCompData()
{
//    fwd_free_comp_data((void *)this);
    if(this->comp_coils)
        delete this->comp_coils;
    if(this->set)
        delete this->set;
    FREE_60(this->work);
    FREE_CMATRIX_60(this->vec_work);

    if (this->client_free && this->client)
        this->client_free(this->client);
}

//=============================================================================================================

int FwdCompData::fwd_comp_field(float *rd, float *Q, FwdCoilSet *coils, float *res, void *client)
/*
          * Calculate the compensated field (one dipole component)
          */
{
    FwdCompData* comp = (FwdCompData*)client;

    if (!comp->field) {
        printf("Field computation function is missing in fwd_comp_field_vec");
        return FAIL;
    }
    /*
       * First compute the field in the primary set of coils
       */
    if (comp->field(rd,Q,coils,res,comp->client) == FAIL)
        return FAIL;
    /*
       * Compensation needed?
       */
    if (!comp->comp_coils || comp->comp_coils->ncoil <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
       * Workspace needed?
       */
    if (!comp->work)
        comp->work = MALLOC_60(comp->comp_coils->ncoil,float);
    /*
       * Compute the field in the compensation coils
       */
    if (comp->field(rd,Q,comp->comp_coils,comp->work,comp->client) == FAIL)
        return FAIL;
    /*
       * Compute the compensated field
       */
    return MneCTFCompDataSet::mne_apply_ctf_comp(comp->set,TRUE,res,coils->ncoil,comp->work,comp->comp_coils->ncoil);
}

//=============================================================================================================

void FwdCompData::fwd_free_comp_data(void *d)
{
    FwdCompData* comp = (FwdCompData*)d;

    if (!comp)
        return;

    if (comp->client_free && comp->client)
        comp->client_free(comp->client);

    if(comp)
        delete(comp);
    return;
}

//=============================================================================================================

int FwdCompData::fwd_make_ctf_comp_coils(MneCTFCompDataSet *set,
                                         FwdCoilSet *coils,
                                         FwdCoilSet *comp_coils)   /* The compensation coil set */
/*
 * Call mne_make_ctf_comp using the information in the coil sets
 */
{
    QList<FiffChInfo> chs;
    QList<FiffChInfo> compchs;
    int        nchan   = 0;
    int        ncomp   = 0;
    FwdCoil* coil;
    int k,res;

    if (!coils || coils->ncoil <= 0) {
        printf("Coil data missing in fwd_make_ctf_comp_coils");
        return FAIL;
    }
    /*
       * Create the fake channel info which contain just enough information
       * for mne_make_ctf_comp
       */
    for (k = 0; k < coils->ncoil; k++) {
        chs.append(FiffChInfo());
        coil = coils->coils[k];
        chs[k].ch_name = coil->chname;
        chs[k].chpos.coil_type = coil->type;
        chs[k].kind = (coil->coil_class == FWD_COILC_EEG) ? FIFFV_EEG_CH : FIFFV_MEG_CH;
    }
    nchan = coils->ncoil;
    if (comp_coils && comp_coils->ncoil > 0) {
        for (k = 0; k < comp_coils->ncoil; k++) {
            compchs.append(FiffChInfo());
            coil = comp_coils->coils[k];
            compchs[k].ch_name = coil->chname;
            compchs[k].chpos.coil_type = coil->type;
            compchs[k].kind = (coil->coil_class == FWD_COILC_EEG) ? FIFFV_EEG_CH : FIFFV_MEG_CH;
        }
        ncomp = comp_coils->ncoil;
    }
    res = MneCTFCompDataSet::mne_make_ctf_comp(set,chs,nchan,compchs,ncomp);

    return res;
}

//=============================================================================================================

FwdCompData *FwdCompData::fwd_make_comp_data(MneCTFCompDataSet *set,
                                             FwdCoilSet *coils,
                                             FwdCoilSet *comp_coils,
                                             fwdFieldFunc field,
                                             fwdVecFieldFunc vec_field,
                                             fwdFieldGradFunc field_grad,
                                             void *client,
                                             fwdUserFreeFunc client_free)
/*
 * Compose a compensation data set
 */
{
    FwdCompData* comp = new FwdCompData();

    if(set)
        comp->set = new MneCTFCompDataSet(*set);
    else
        comp->set = NULL;

    if (comp_coils) {
        comp->comp_coils = comp_coils->dup_coil_set(NULL);
    }
    else {
        qWarning("No coils to duplicate");
        comp->comp_coils = NULL;
    }
    comp->field       = field;
    comp->vec_field   = vec_field;
    comp->field_grad  = field_grad;
    comp->client      = client;
    comp->client_free = client_free;

    if (fwd_make_ctf_comp_coils(comp->set,
                                coils,
                                comp->comp_coils) != OK) {
        fwd_free_comp_data(comp);
        return NULL;
    }
    else {
        return comp;
    }
}

//=============================================================================================================

int FwdCompData::fwd_comp_field_vec(float *rd, FwdCoilSet *coils, float **res, void *client)
/*
          * Calculate the compensated field (all dipole components)
          */
{
    FwdCompData* comp = (FwdCompData*)client;
    int k;

    if (!comp->vec_field) {
        printf("Field computation function is missing in fwd_comp_field_vec");
        return FAIL;
    }
    /*
       * First compute the field in the primary set of coils
       */
    if (comp->vec_field(rd,coils,res,comp->client) == FAIL)
        return FAIL;
    /*
       * Compensation needed?
       */
    if (!comp->comp_coils || comp->comp_coils->ncoil <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
       * Need workspace?
       */
    if (!comp->vec_work)
        comp->vec_work = ALLOC_CMATRIX_60(3,comp->comp_coils->ncoil);
    /*
       * Compute the field at the compensation sensors
       */
    if (comp->vec_field(rd,comp->comp_coils,comp->vec_work,comp->client) == FAIL)
        return FAIL;
    /*
       * Compute the compensated field of three orthogonal dipoles
       */
    for (k = 0; k < 3; k++) {
        if (MneCTFCompDataSet::mne_apply_ctf_comp(comp->set,TRUE,res[k],coils->ncoil,comp->vec_work[k],comp->comp_coils->ncoil) == FAIL)
            return FAIL;
    }
    return OK;
}

//=============================================================================================================

int FwdCompData::fwd_comp_field_grad(float *rd, float *Q, FwdCoilSet* coils, float *res, float *xgrad, float *ygrad, float *zgrad, void *client)
/*
 * Calculate the compensated field (one dipole component)
 */
{
    FwdCompData* comp = (FwdCompData*)client;

    if (!comp->field_grad) {
        qCritical("Field and gradient computation function is missing in fwd_comp_field_grad");
        return FAIL;
    }
    /*
     * First compute the field in the primary set of coils
     */
    if (comp->field_grad(rd,Q,coils,res,xgrad,ygrad,zgrad,comp->client) == FAIL)
        return FAIL;
    /*
     * Compensation needed?
     */
    if (!comp->comp_coils || comp->comp_coils->ncoil <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
     * Workspace needed?
     */
    if (!comp->work)
        comp->work = MALLOC_60(comp->comp_coils->ncoil,float);
    if (!comp->vec_work)
        comp->vec_work = ALLOC_CMATRIX_60(3,comp->comp_coils->ncoil);
    /*
     * Compute the field in the compensation coils
     */
    if (comp->field_grad(rd,Q,comp->comp_coils,comp->work,comp->vec_work[0],comp->vec_work[1],comp->vec_work[2],comp->client) == FAIL)
        return FAIL;
    /*
     * Compute the compensated field
     */
    if (MneCTFCompDataSet::mne_apply_ctf_comp(comp->set,TRUE,res,coils->ncoil,comp->work,comp->comp_coils->ncoil) != OK)
        return FAIL;
    if (MneCTFCompDataSet::mne_apply_ctf_comp(comp->set,TRUE,xgrad,coils->ncoil,comp->vec_work[0],comp->comp_coils->ncoil) != OK)
        return FAIL;
    if (MneCTFCompDataSet::mne_apply_ctf_comp(comp->set,TRUE,ygrad,coils->ncoil,comp->vec_work[1],comp->comp_coils->ncoil) != OK)
        return FAIL;
    if (MneCTFCompDataSet::mne_apply_ctf_comp(comp->set,TRUE,zgrad,coils->ncoil,comp->vec_work[2],comp->comp_coils->ncoil) != OK)
        return FAIL;
    return OK;
}
