//=============================================================================================================
/**
* @file     fwd_coil_set.cpp
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
* @brief    Implementation of the FwdCoilSet Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_coil_set.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace INVERSELIB;




#define REALLOC_6(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define FREE_6(x) if ((char *)(x) != NULL) free((char *)(x))

#define FIFFV_COORD_UNKNOWN     0





#define X_6 0
#define Y_6 1
#define Z_6 2



#define VEC_COPY_6(to,from) {\
    (to)[X_6] = (from)[X_6];\
    (to)[Y_6] = (from)[Y_6];\
    (to)[Z_6] = (from)[Z_6];\
    }


char *mne_strdup_6(const char *s)
{
    char *res;
    if (s == NULL)
        return NULL;
    res = (char*) malloc(strlen(s)+1);
    strcpy(res,s);
    return res;
}



void fiff_coord_trans_6 (float r[3],fiffCoordTrans t,int do_move)
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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdCoilSet::FwdCoilSet()
{
    coils = NULL;
    ncoil = 0;
    coord_frame = FIFFV_COORD_UNKNOWN;
    user_data = NULL;
    user_data_free = NULL;
}


//*************************************************************************************************************

//FwdCoilSet::FwdCoilSet(const FwdCoilSet& p_FwdCoilSet)
//{
//}


//*************************************************************************************************************

FwdCoilSet::~FwdCoilSet()
{
    for (int k = 0; k < ncoil; k++)
        delete coils[k];
    FREE_6(coils);

    this->fwd_free_coil_set_user_data();
}


//*************************************************************************************************************

FwdCoil *FwdCoilSet::create_meg_coil(fiffChInfo ch, int acc, fiffCoordTrans t)
{
    int        k,p,c;
    FwdCoil*    def;
    FwdCoil*    res = NULL;

    if (ch->kind != FIFFV_MEG_CH && ch->kind != FIFFV_REF_MEG_CH) {
        printf("%s is not a MEG channel. Cannot create a coil definition.",ch->ch_name);
        goto bad;
    }
    /*
        * Simple linear search from the coil definitions
        */
    for (k = 0, def = NULL; k < this->ncoil; k++) {
        if ((this->coils[k]->type == (ch->chpos.coil_type & 0xFFFF)) &&
                this->coils[k]->accuracy == acc) {
            def = this->coils[k];
        }
    }
    if (!def) {
        printf("Desired coil definition not found (type = %d acc = %d)",ch->chpos.coil_type,acc);
        goto bad;
    }
    /*
        * Create the result
        */
    res = new FwdCoil(def->np);

    res->chname   = mne_strdup_6(ch->ch_name);
    if (def->desc)
        res->desc   = mne_strdup_6(def->desc);
    res->coil_class = def->coil_class;
    res->accuracy   = def->accuracy;
    res->base       = def->base;
    res->size       = def->size;
    res->type       = ch->chpos.coil_type;

    VEC_COPY_6(res->r0,ch->chpos.r0);
    VEC_COPY_6(res->ex,ch->chpos.ex);
    VEC_COPY_6(res->ey,ch->chpos.ey);
    VEC_COPY_6(res->ez,ch->chpos.ez);
    /*
        * Apply a coordinate transformation if so desired
        */
    if (t) {
        fiff_coord_trans_6(res->r0,t,FIFFV_MOVE);
        fiff_coord_trans_6(res->ex,t,FIFFV_NO_MOVE);
        fiff_coord_trans_6(res->ey,t,FIFFV_NO_MOVE);
        fiff_coord_trans_6(res->ez,t,FIFFV_NO_MOVE);
        res->coord_frame = t->to;
    }
    else
        res->coord_frame = FIFFV_COORD_DEVICE;

    for (p = 0; p < res->np; p++) {
        res->w[p] = def->w[p];
        for (c = 0; c < 3; c++) {
            res->rmag[p][c]   = res->r0[c] + def->rmag[p][X_6]*res->ex[c] + def->rmag[p][Y_6]*res->ey[c] + def->rmag[p][Z_6]*res->ez[c];
            res->cosmag[p][c] = def->cosmag[p][X_6]*res->ex[c] + def->cosmag[p][Y_6]*res->ey[c] + def->cosmag[p][Z_6]*res->ez[c];
        }
    }
    return res;

bad : {
        return NULL;
    }
}


//*************************************************************************************************************

FwdCoilSet *FwdCoilSet::create_meg_coils(FIFFLIB::fiffChInfo chs, int nch, int acc, FIFFLIB::fiffCoordTrans t)
{
    FwdCoilSet* res = new FwdCoilSet();
    FwdCoil*    next;
    int        k;

    for (k = 0; k < nch; k++) {
        if ((next = this->create_meg_coil(chs+k,acc,t)) == NULL)
            goto bad;
        res->coils = REALLOC_6(res->coils,res->ncoil+1,FwdCoil*);
        res->coils[res->ncoil++] = next;
    }
    if (t)
        res->coord_frame = t->to;
    return res;

bad : {
        delete res;
        return NULL;
    }
}
