//=============================================================================================================
/**
* @file     fwd_coil.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the FwdCoil Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_coil.h"
#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace INVERSELIB;


#define MALLOC_5(x,t) (t *)malloc((x)*sizeof(t))


#define FREE_5(x) if ((char *)(x) != NULL) free((char *)(x))
#define FREE_CMATRIX_5(m) mne_free_cmatrix_5((m))


#define ALLOC_CMATRIX_5(x,y) mne_cmatrix_5((x),(y))

#define X_5 0
#define Y_5 1
#define Z_5 2


#define VEC_DOT_5(x,y) ((x)[X_5]*(y)[X_5] + (x)[Y_5]*(y)[Y_5] + (x)[Z_5]*(y)[Z_5])
#define VEC_LEN_5(x) sqrt(VEC_DOT_5(x,x))


#define VEC_COPY_5(to,from) {\
    (to)[X_5] = (from)[X_5];\
    (to)[Y_5] = (from)[Y_5];\
    (to)[Z_5] = (from)[Z_5];\
    }




char *mne_strdup_5(const char *s)
{
    char *res;
    if (s == NULL)
        return NULL;
    res = (char*) malloc(strlen(s)+1);
    strcpy(res,s);
    return res;
}



static void matrix_error_5(int kind, int nr, int nc)

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


float **mne_cmatrix_5(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_5(nr,float *);
    if (!m) matrix_error_5(1,nr,nc);
    whole = MALLOC_5(nr*nc,float);
    if (!whole) matrix_error_5(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

void mne_free_cmatrix_5 (float **m)
{
    if (m) {
        FREE_5(*m);
        FREE_5(m);
    }
}


void fiff_coord_trans_5 (float r[3],fiffCoordTrans t,int do_move)
/*
      * Apply coordinate transformation
      */
{
    int j,k;
    float res[3];

    for (j = 0; j < 3; j++) {
        res[j] = (do_move ? t->move[j] :  0.0);
        for (k = 0; k < 3; k++)
            res[j] += t->rot[j][k]*r[k];
    }
    for (j = 0; j < 3; j++)
        r[j] = res[j];
}


static void normalize_5(float *rr)
/*
      * Scale vector to unit length
      */
{
    float ll = VEC_LEN_5(rr);
    int k;
    if (ll > 0) {
        for (k = 0; k < 3; k++)
            rr[k] = rr[k]/ll;
    }
    return;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdCoil::FwdCoil(int p_np)
{
    chname     = NULL;
    desc       = NULL;
    coil_class = FWD_COILC_UNKNOWN;
    accuracy   = FWD_COIL_ACCURACY_POINT;
    base       = 0.0;
    size       = 0.0;
    np         = p_np;
    rmag       = ALLOC_CMATRIX_5(np,3);
    cosmag     = ALLOC_CMATRIX_5(np,3);
    w          = MALLOC_5(np,float);
    /*
   * Reasonable defaults
   */
    for (int k = 0; k < 3; k++) {
        r0[k] = 0.0;
        ex[k] = 0.0;
        ey[k] = 0.0;
        ez[k] = 0.0;
    }
    ex[0] = 1.0;
    ey[1] = 1.0;
    ez[2] = 1.0;
}


//*************************************************************************************************************

FwdCoil::FwdCoil(const FwdCoil& p_FwdCoil)
{
    if (p_FwdCoil.chname)
        this->chname   = mne_strdup_5(p_FwdCoil.chname);
    if (p_FwdCoil.desc)
        this->desc   = mne_strdup_5(p_FwdCoil.desc);
    this->coil_class = p_FwdCoil.coil_class;
    this->accuracy   = p_FwdCoil.accuracy;
    this->base       = p_FwdCoil.base;
    this->size       = p_FwdCoil.size;
    this->type       = p_FwdCoil.type;

    VEC_COPY_5(this->r0,p_FwdCoil.r0);
    VEC_COPY_5(this->ex,p_FwdCoil.ex);
    VEC_COPY_5(this->ey,p_FwdCoil.ey);
    VEC_COPY_5(this->ez,p_FwdCoil.ez);

    for (int p = 0; p < p_FwdCoil.np; p++) {
        this->w[p] = p_FwdCoil.w[p];
        VEC_COPY_5(this->rmag[p],p_FwdCoil.rmag[p]);
        VEC_COPY_5(this->cosmag[p],p_FwdCoil.cosmag[p]);
    }
    this->coord_frame = p_FwdCoil.coord_frame;
}


//*************************************************************************************************************

FwdCoil::~FwdCoil()
{
    FREE_5(chname);
    FREE_5(desc);
    FREE_CMATRIX_5(rmag);
    FREE_CMATRIX_5(cosmag);
    FREE_5(w);
}


//*************************************************************************************************************

FwdCoil *FwdCoil::create_eeg_el(FIFFLIB::fiffChInfo ch, FIFFLIB::fiffCoordTrans t)
{
    FwdCoil*    res = NULL;
    int        c;

    if (ch->kind != FIFFV_EEG_CH) {
        printf("%s is not an EEG channel. Cannot create an electrode definition.",ch->ch_name);
        goto bad;
    }
    if (t && t->from != FIFFV_COORD_HEAD) {
        printf("Inappropriate coordinate transformation in fwd_create_eeg_el");
        goto bad;
    }

    if (VEC_LEN_5(ch->chpos.ex) < 1e-4)
        res = new FwdCoil(1);	             /* No reference electrode */
    else
        res = new FwdCoil(2);		     /* Reference electrode present */

    res->chname     = mne_strdup_5(ch->ch_name);
    res->desc       = mne_strdup_5("EEG electrode");
    res->coil_class = FWD_COILC_EEG;
    res->accuracy   = FWD_COIL_ACCURACY_NORMAL;
    res->type       = ch->chpos.coil_type;
    VEC_COPY_5(res->r0,ch->chpos.r0);
    VEC_COPY_5(res->ex,ch->chpos.ex);
    /*
       * Optional coordinate transformation
       */
    if (t) {
        fiff_coord_trans_5(res->r0,t,FIFFV_MOVE);
        fiff_coord_trans_5(res->ex,t,FIFFV_MOVE);
        res->coord_frame = t->to;
    }
    else
        res->coord_frame = FIFFV_COORD_HEAD;
    /*
       * The electrode location
       */
    for (c = 0; c < 3; c++)
        res->rmag[0][c] = res->cosmag[0][c] = res->r0[c];
    normalize_5(res->cosmag[0]);
    res->w[0] = 1.0;
    /*
       * Add the reference electrode, if appropriate
       */
    if (res->np == 2) {
        for (c = 0; c < 3; c++)
            res->rmag[1][c] = res->cosmag[1][c] = res->ex[c];
        normalize_5(res->cosmag[1]);
        res->w[1] = -1.0;
    }
    return res;

bad : {
        return NULL;
    }
}
