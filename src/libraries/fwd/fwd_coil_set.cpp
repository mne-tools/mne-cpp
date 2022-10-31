//=============================================================================================================
/**
 * @file     fwd_coil_set.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FwdCoilSet Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_coil_set.h"
#include "fwd_coil.h"

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace FWDLIB;

#define MAXWORD 1000
#define BIG 0.5

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

#define MALLOC_6(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_6(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define FREE_6(x) if ((char *)(x) != NULL) free((char *)(x))

#define FIFFV_COORD_UNKNOWN     0

#define X_6 0
#define Y_6 1
#define Z_6 2

#define VEC_DOT_6(x,y) ((x)[X_6]*(y)[X_6] + (x)[Y_6]*(y)[Y_6] + (x)[Z_6]*(y)[Z_6])
#define VEC_LEN_6(x) sqrt(VEC_DOT_6(x,x))

#define VEC_COPY_6(to,from) {\
    (to)[X_6] = (from)[X_6];\
    (to)[Y_6] = (from)[Y_6];\
    (to)[Z_6] = (from)[Z_6];\
    }

static void skip_comments(FILE *in)

{
    int c;

    while (1) {
        c = fgetc(in);
        if (c == '#') {
            for (c = fgetc(in); c != EOF && c != '\n'; c = fgetc(in))
                ;
        }
        else {
            ungetc(c,in);
            return;
        }
    }
}

static int whitespace(int c)

{
    if (c == '\t' || c == '\n' || c == ' ')
        return TRUE;
    else
        return FALSE;
}

static int whitespace_quote(int c, int inquote)

{
    if (inquote)
        return (c == '"');
    else
        return (c == '\t' || c == '\n' || c == ' ');
}

static char *next_word(FILE *in)

{
    char *next = MALLOC_6(MAXWORD,char);
    int c;
    int  p,k;
    int  inquote;

    skip_comments(in);

    inquote = FALSE;
    for (k = 0, p = 0, c = fgetc(in); c != EOF && !whitespace_quote(c,inquote) ; c = fgetc(in), k++) {
        if (k == 0 && c == '"')
            inquote = TRUE;
        else
            next[p++] = c;
    }
    if (c == EOF && k == 0) {
        FREE_6(next);
        return NULL;
    }
    else
        next[p] = '\0';
    if (c != EOF) {
        for (k = 0, c = fgetc(in); whitespace(c) ; c = fgetc(in), k++)
            ;
        if (c != EOF)
            ungetc(c,in);
    }
#ifdef DEBUG
    if (next)
        printf("<%s>\n",next);
#endif
    return next;
}

static int get_ival(FILE *in, int *ival)

{
    char *next = next_word(in);
    if (next == NULL) {
        qWarning("missing integer");
        return FAIL;
    }
    else if (sscanf(next,"%d",ival) != 1) {
        qWarning("bad integer : %s",next);
        FREE_6(next);
        return FAIL;
    }
    FREE_6(next);
    return OK;
}

static int get_fval(FILE *in, float *fval)

{
    char *next = next_word(in);
    setlocale(LC_NUMERIC, "C");
    if (next == NULL) {
        qWarning("bad integer");
        return FAIL;
    }
    else if (sscanf(next,"%g",fval) != 1) {
        qWarning("bad floating point number : %s",next);
        FREE_6(next);
        return FAIL;
    }
    FREE_6(next);
    return OK;
}

static void normalize(float *rr)
/*
      * Scale vector to unit length
      */
{
    float ll = VEC_LEN_6(rr);
    int k;
    if (ll > 0) {
        for (k = 0; k < 3; k++)
            rr[k] = rr[k]/ll;
    }
    return;
}

static FwdCoil* fwd_add_coil_to_set(FwdCoilSet* set,
                                   int type, int coil_class, int acc, int np, float size, float base, const QString& desc)

{
    FwdCoil* def;

    if (set == NULL) {
        qWarning ("No coil definition set to augment.");
        return NULL;
    }
    if (np <= 0) {
        qWarning("Number of integration points should be positive (type = %d acc = %d)",type,acc);
        return NULL;
    }
    if (! (acc == FWD_COIL_ACCURACY_POINT ||
           acc == FWD_COIL_ACCURACY_NORMAL ||
           acc == FWD_COIL_ACCURACY_ACCURATE) ) {
        qWarning("Illegal accuracy (type = %d acc = %d)",type,acc);
        return NULL;
    }
    if (! (coil_class == FWD_COILC_MAG ||
           coil_class == FWD_COILC_AXIAL_GRAD ||
           coil_class == FWD_COILC_PLANAR_GRAD ||
           coil_class == FWD_COILC_AXIAL_GRAD2) ) {
        qWarning("Illegal coil class (type = %d acc = %d class = %d)",type,acc,coil_class);
        return NULL;
    }

    set->coils = REALLOC_6(set->coils,set->ncoil+1,FwdCoil*);
    def = set->coils[set->ncoil++] = new FwdCoil(np);

    def->type       = type;
    def->coil_class = coil_class;
    def->accuracy   = acc;
    def->np         = np;
    def->base       = size;
    def->base       = base;
    if (!desc.isEmpty())
        def->desc = desc;
    return def;
}

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

//=============================================================================================================

//FwdCoilSet::FwdCoilSet(const FwdCoilSet& p_FwdCoilSet)
//{
//}

//=============================================================================================================

FwdCoilSet::~FwdCoilSet()
{
    for (int k = 0; k < ncoil; k++)
        delete coils[k];
    FREE_6(coils);

    this->fwd_free_coil_set_user_data();
}

//=============================================================================================================

FwdCoil *FwdCoilSet::create_meg_coil(const FiffChInfo& ch, int acc, const FiffCoordTransOld* t)
{
    int        k,p,c;
    FwdCoil*    def;
    FwdCoil*    res = NULL;

    if (ch.kind != FIFFV_MEG_CH && ch.kind != FIFFV_REF_MEG_CH) {
        qWarning() << ch.ch_name << "is not a MEG channel. Cannot create a coil definition.";
        goto bad;
    }
    /*
        * Simple linear search from the coil definitions
        */
    for (k = 0, def = NULL; k < this->ncoil; k++) {
        if ((this->coils[k]->type == (ch.chpos.coil_type & 0xFFFF)) &&
                this->coils[k]->accuracy == acc) {
            def = this->coils[k];
        }
    }
    if (!def) {
        printf("Desired coil definition not found (type = %d acc = %d)",ch.chpos.coil_type,acc);
        goto bad;
    }
    /*
        * Create the result
        */
    res = new FwdCoil(def->np);

    res->chname   = ch.ch_name;
    if (!def->desc.isEmpty())
        res->desc   = def->desc;
    res->coil_class = def->coil_class;
    res->accuracy   = def->accuracy;
    res->base       = def->base;
    res->size       = def->size;
    res->type       = ch.chpos.coil_type;

    VEC_COPY_6(res->r0,ch.chpos.r0);
    VEC_COPY_6(res->ex,ch.chpos.ex);
    VEC_COPY_6(res->ey,ch.chpos.ey);
    VEC_COPY_6(res->ez,ch.chpos.ez);
    /*
        * Apply a coordinate transformation if so desired
        */
    if (t) {
        FiffCoordTransOld::fiff_coord_trans(res->r0,t,FIFFV_MOVE);
        FiffCoordTransOld::fiff_coord_trans(res->ex,t,FIFFV_NO_MOVE);
        FiffCoordTransOld::fiff_coord_trans(res->ey,t,FIFFV_NO_MOVE);
        FiffCoordTransOld::fiff_coord_trans(res->ez,t,FIFFV_NO_MOVE);
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

//=============================================================================================================

FwdCoilSet *FwdCoilSet::create_meg_coils(const QList<FIFFLIB::FiffChInfo>& chs,
                                         int nch,
                                         int acc,
                                         const FiffCoordTransOld* t)
{
    FwdCoilSet* res = new FwdCoilSet();
    FwdCoil*    next;
    int        k;

    for (k = 0; k < nch; k++) {
        if ((next = this->create_meg_coil(chs.at(k),acc,t)) == Q_NULLPTR)
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

//=============================================================================================================

FwdCoilSet *FwdCoilSet::create_eeg_els(const QList<FIFFLIB::FiffChInfo>& chs,
                                       int nch,
                                       const FiffCoordTransOld* t)
{
    FwdCoilSet* res = new FwdCoilSet();
    FwdCoil*    next;
    int        k;

    for (k = 0; k < nch; k++) {
        if ((next = FwdCoil::create_eeg_el(chs.at(k),t)) == Q_NULLPTR)
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

//=============================================================================================================

FwdCoilSet *FwdCoilSet::read_coil_defs(const QString &name)
/*
          * Read a coil definition file
          */
{
    FILE    *in = fopen(name.toUtf8().constData(),"r");
    char    *desc = NULL;
    int     type,coil_class,acc,np;
    int     p;
    float   size,base;
    FwdCoilSet* res = NULL;
    FwdCoil* def;

    if (in == NULL) {
        qWarning() << "FwdCoilSet::read_coil_defs - File is NULL" << name;
        goto bad;
    }

    res = new FwdCoilSet();
    while (1) {
        /*
         * Read basic info
         */
        if (get_ival(in,&coil_class) != OK)
            break;
        if (get_ival(in,&type) != OK)
            goto bad;
        if (get_ival(in,&acc) != OK)
            goto bad;
        if (get_ival(in,&np) != OK)
            goto bad;
        if (get_fval(in,&size) != OK)
            goto bad;
        if (get_fval(in,&base) != OK)
            goto bad;
        desc = next_word(in);
        if (!desc)
            goto bad;

        def = fwd_add_coil_to_set(res,type,coil_class,acc,np,size,base,desc);
        if (!def)
            goto bad;
        FREE_6(desc); desc = NULL;

        for (p = 0; p < def->np; p++) {
            /*
           * Read and verify data for each integration point
           */
            if (get_fval(in,def->w+p) != OK)
                goto bad;
            if (get_fval(in,def->rmag[p]+X_6) != OK)
                goto bad;
            if (get_fval(in,def->rmag[p]+Y_6) != OK)
                goto bad;
            if (get_fval(in,def->rmag[p]+Z_6) != OK)
                goto bad;
            if (get_fval(in,def->cosmag[p]+X_6) != OK)
                goto bad;
            if (get_fval(in,def->cosmag[p]+Y_6) != OK)
                goto bad;
            if (get_fval(in,def->cosmag[p]+Z_6) != OK)
                goto bad;

            if (VEC_LEN_6(def->rmag[p]) > BIG) {
                qWarning("Unreasonable integration point: %f %f %f mm (coil type = %d acc = %d)", 1000*def->rmag[p][X_6],1000*def->rmag[p][Y_6],1000*def->rmag[p][Z_6], def->type,def->accuracy);
                goto bad;
            }
            size = VEC_LEN_6(def->cosmag[p]);
            if (size <= 0) {
                qWarning("Unreasonable normal: %f %f %f (coil type = %d acc = %d)", def->cosmag[p][X_6],def->cosmag[p][Y_6],def->cosmag[p][Z_6], def->type,def->accuracy);
                goto bad;
            }
            normalize(def->cosmag[p]);
        }
    }

    fclose(in);

    printf("%d coil definitions read\n",res->ncoil);
    return res;

bad : {
        delete res;
        FREE_6(desc);
        return NULL;
    }
}

//=============================================================================================================

FwdCoilSet* FwdCoilSet::dup_coil_set(const FiffCoordTransOld* t) const
{
    FwdCoilSet* res;
    FwdCoil*    coil;

    if (t) {
        if (this->coord_frame != t->from) {
            qWarning("Coordinate frame of the transformation does not match the coil set in fwd_dup_coil_set");
            return NULL;
        }
    }
    res = new FwdCoilSet();
    if (t)
        res->coord_frame = t->to;
    else
        res->coord_frame = this->coord_frame;

    res->coils = MALLOC_6(this->ncoil,FwdCoil*);
    res->ncoil = this->ncoil;

    for (int k = 0; k < this->ncoil; k++) {
        coil = res->coils[k] = new FwdCoil(*(this->coils[k]));
        /*
     * Optional coordinate transformation
     */
        if (t) {
            FiffCoordTransOld::fiff_coord_trans(coil->r0,t,FIFFV_MOVE);
            FiffCoordTransOld::fiff_coord_trans(coil->ex,t,FIFFV_NO_MOVE);
            FiffCoordTransOld::fiff_coord_trans(coil->ey,t,FIFFV_NO_MOVE);
            FiffCoordTransOld::fiff_coord_trans(coil->ez,t,FIFFV_NO_MOVE);

            for (int p = 0; p < coil->np; p++) {
                FiffCoordTransOld::fiff_coord_trans(coil->rmag[p],t,FIFFV_MOVE);
                FiffCoordTransOld::fiff_coord_trans(coil->cosmag[p],t,FIFFV_NO_MOVE);
            }
            coil->coord_frame = t->to;
        }
    }
    return res;
}

//=============================================================================================================

bool FwdCoilSet::is_planar_coil_type(int type) const
{
    if (type == FIFFV_COIL_EEG)
        return false;
    for (int k = 0; k < this->ncoil; k++)
        if (this->coils[k]->type == type)
            return this->coils[k]->coil_class == FWD_COILC_PLANAR_GRAD;
    return false;
}

//=============================================================================================================

bool FwdCoilSet::is_axial_coil_type(int type) const
{
    if (type == FIFFV_COIL_EEG)
        return false;
    for (int k = 0; k < this->ncoil; k++)
        if (this->coils[k]->type == type)
            return (this->coils[k]->coil_class == FWD_COILC_MAG ||
                    this->coils[k]->coil_class == FWD_COILC_AXIAL_GRAD ||
                    this->coils[k]->coil_class == FWD_COILC_AXIAL_GRAD2);
    return false;
}

//=============================================================================================================

bool FwdCoilSet::is_magnetometer_coil_type(int type) const
{
    if (type == FIFFV_COIL_EEG)
        return false;
    for (int k = 0; k < this->ncoil; k++)
        if (this->coils[k]->type == type)
            return this->coils[k]->coil_class == FWD_COILC_MAG;
    return false;
}

//=============================================================================================================

bool FwdCoilSet::is_eeg_electrode_type(int type) const
{
    return type == FIFFV_COIL_EEG;
}

