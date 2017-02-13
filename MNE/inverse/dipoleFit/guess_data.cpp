//=============================================================================================================
/**
* @file     guess_data.cpp
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
* @brief    Implementation of the GuessData Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "guess_data.h"
#include "dipole_fit_data.h"

#include "dipole_forward.h"

#include "mne_surface_or_volume.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace INVERSELIB;


//ToDo remove later on
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






#define X_16 0
#define Y_16 1
#define Z_16 2


#define VEC_COPY_16(to,from) {\
    (to)[X_16] = (from)[X_16];\
    (to)[Y_16] = (from)[Y_16];\
    (to)[Z_16] = (from)[Z_16];\
    }



#define MALLOC_16(x,t) (t *)malloc((x)*sizeof(t))

#define REALLOC_16(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define ALLOC_CMATRIX_16(x,y) mne_cmatrix_16((x),(y))



static void matrix_error_16(int kind, int nr, int nc)

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


float **mne_cmatrix_16(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_16(nr,float *);
    if (!m) matrix_error_16(1,nr,nc);
    whole = MALLOC_16(nr*nc,float);
    if (!whole) matrix_error_16(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}




#define FREE_16(x) if ((char *)(x) != NULL) free((char *)(x))
#define FREE_CMATRIX_16(m) mne_free_cmatrix_16((m))

void mne_free_cmatrix_16 (float **m)
{
    if (m) {
        FREE_16(*m);
        FREE_16(m);
    }
}








void fromFloatEigenMatrix_16(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_16(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_16(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}


//int
void fromIntEigenMatrix_16(const Eigen::MatrixXi& from_mat, int **&to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromIntEigenMatrix_16(const Eigen::MatrixXi& from_mat, int **&to_mat)
{
    fromIntEigenMatrix_16(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}








//============================= mne_coord_transforms.c =============================




typedef struct {
    int frame;
    const char *name;
} frameNameRec_16;


const char *mne_coord_frame_name_16(int frame)

{
    static frameNameRec_16 frames[] = {
        {FIFFV_COORD_UNKNOWN,"unknown"},
        {FIFFV_COORD_DEVICE,"MEG device"},
        {FIFFV_COORD_ISOTRAK,"isotrak"},
        {FIFFV_COORD_HPI,"hpi"},
        {FIFFV_COORD_HEAD,"head"},
        {FIFFV_COORD_MRI,"MRI (surface RAS)"},
        {FIFFV_MNE_COORD_MRI_VOXEL, "MRI voxel"},
        {FIFFV_COORD_MRI_SLICE,"MRI slice"},
        {FIFFV_COORD_MRI_DISPLAY,"MRI display"},
        {FIFFV_MNE_COORD_CTF_DEVICE,"CTF MEG device"},
        {FIFFV_MNE_COORD_CTF_HEAD,"CTF/4D/KIT head"},
        {FIFFV_MNE_COORD_RAS,"RAS (non-zero origin)"},
        {FIFFV_MNE_COORD_MNI_TAL,"MNI Talairach"},
        {FIFFV_MNE_COORD_FS_TAL_GTZ,"Talairach (MNI z > 0)"},
        {FIFFV_MNE_COORD_FS_TAL_LTZ,"Talairach (MNI z < 0)"},
        {-1,"unknown"}
    };
    int k;
    for (k = 0; frames[k].frame != -1; k++) {
        if (frame == frames[k].frame)
            return frames[k].name;
    }
    return frames[k].name;
}






//============================= fiff_trans.c =============================

void fiff_coord_trans_16 (float r[3],FiffCoordTransOld* t,int do_move)
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


void fiff_coord_trans_inv_16 (float r[3],FiffCoordTransOld* t,int do_move)
/*
      * Apply inverse coordinate transformation
      */
{
    int j,k;
    float res[3];

    for (j = 0; j < 3; j++) {
        res[j] = (do_move ? t->invmove[j] :  0.0);
        for (k = 0; k < 3; k++)
            res[j] += t->invrot[j][k]*r[k];
    }
    for (j = 0; j < 3; j++)
        r[j] = res[j];
}









//============================= mne_source_space.c =============================



int mne_transform_source_space(MneSurfaceOrVolume::MneCSourceSpace* ss, FiffCoordTransOld* t)
/*
* Transform source space data into another coordinate frame
*/
{
    int k;
    if (ss == NULL)
        return OK;
    if (ss->coord_frame == t->to)
        return OK;
    if (ss->coord_frame != t->from) {
        printf("Coordinate transformation does not match with the source space coordinate system.");
        return FAIL;
    }
    for (k = 0; k < ss->np; k++) {
        fiff_coord_trans_16(ss->rr[k],t,FIFFV_MOVE);
        fiff_coord_trans_16(ss->nn[k],t,FIFFV_NO_MOVE);
    }
    if (ss->tris) {
        for (k = 0; k < ss->ntri; k++)
            fiff_coord_trans_16(ss->tris[k].nn,t,FIFFV_NO_MOVE);
    }
    ss->coord_frame = t->to;
    return OK;
}



int mne_transform_source_spaces_to(int            coord_frame,   /* Which coord frame do we want? */
                                   FiffCoordTransOld* t,             /* The coordinate transformation */
                                   MneSurfaceOrVolume::MneCSourceSpace* *spaces,       /* A list of source spaces */
                                   int            nspace)
/*
      * Facilitate the transformation of the source spaces
      */
{
    MneSurfaceOrVolume::MneCSourceSpace* s;
    int k;
    FiffCoordTransOld* my_t;

    for (k = 0; k < nspace; k++) {
        s = spaces[k];
        if (s->coord_frame != coord_frame) {
            if (t) {
                if (s->coord_frame == t->from && t->to == coord_frame) {
                    if (mne_transform_source_space(s,t) != OK)
                        return FAIL;
                }
                else if (s->coord_frame == t->to && t->from == coord_frame) {
                    my_t = t->fiff_invert_transform();
                    if (mne_transform_source_space(s,my_t) != OK) {
                        FREE_16(my_t);
                        return FAIL;
                    }
                    FREE_16(my_t);
                }
                else {
                    printf("Could not transform a source space because of transformation incompatibility.");
                    return FAIL;
                }
            }
            else {
                printf("Could not transform a source space because of missing coordinate transformation.");
                return FAIL;
            }
        }
    }
    return OK;
}






//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GuessData::GuessData()
: rr(NULL)
, guess_fwd(NULL)
, nguess(0)
{

}


//*************************************************************************************************************

//GuessData::GuessData(const GuessData& p_GuessData)
//{
//}


//*************************************************************************************************************

GuessData::GuessData(const QString &guessname, const QString &guess_surfname, float mindist, float exclude, float grid, DipoleFitData *f)
{
    MneSurfaceOrVolume::MneCSourceSpace* *sp = NULL;
    int            nsp = 0;
//    GuessData*      res = new GuessData();
    int            k,p;
    float          guessrad = 0.080;
    MneSurfaceOrVolume::MneCSourceSpace* guesses = NULL;
    dipoleFitFuncs orig;

    if (!guessname.isEmpty()) {
        /*
            * Read the guesses and transform to the appropriate coordinate frame
            */
        if (MneSurfaceOrVolume::mne_read_source_spaces(guessname,&sp,&nsp) == FAIL)
            goto bad;
        if (nsp != 1) {
            printf("Incorrect number of source spaces in guess file");
            for (k = 0; k < nsp; k++)
                delete sp[k];
            FREE_16(sp);
            goto bad;
        }
        fprintf(stderr,"Read guesses from %s\n",guessname.toLatin1().constData());
        guesses = sp[0]; FREE_16(sp);
    }
    else {
        MneSurfaceOrVolume::MneCSurface*    inner_skull = NULL;
        int            free_inner_skull = FALSE;
        float          r0[3];

        VEC_COPY_16(r0,f->r0);
        fiff_coord_trans_inv_16(r0,f->mri_head_t,TRUE);
        if (f->bem_model) {
            fprintf(stderr,"Using inner skull surface from the BEM (%s)...\n",f->bemname);
            if ((inner_skull = f->bem_model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN)) == NULL)
                goto bad;
        }
        else if (!guess_surfname.isEmpty()) {
            fprintf(stderr,"Reading inner skull surface from %s...\n",guess_surfname.toLatin1().data());
            if ((inner_skull = MneSurfaceOrVolume::read_bem_surface(guess_surfname,FIFFV_BEM_SURF_ID_BRAIN,TRUE,NULL)) == NULL)
                goto bad;
            free_inner_skull = TRUE;
        }
        if ((guesses = MneSurfaceOrVolume::MneCSurface::make_guesses(inner_skull,guessrad,r0,grid,exclude,mindist)) == NULL)
            goto bad;
        if (free_inner_skull)
            delete inner_skull;
    }
    if (mne_transform_source_spaces_to(f->coord_frame,f->mri_head_t,&guesses,1) != OK)
        goto bad;
    fprintf(stderr,"Guess locations are now in %s coordinates.\n",mne_coord_frame_name_16(f->coord_frame));
    this->nguess  = guesses->nuse;
    this->rr      = ALLOC_CMATRIX_16(guesses->nuse,3);
    for (k = 0, p = 0; k < guesses->np; k++)
        if (guesses->inuse[k]) {
            VEC_COPY_16(this->rr[p],guesses->rr[k]);
            p++;
        }
    delete guesses; guesses = NULL;

    fprintf(stderr,"Go through all guess source locations...");
    this->guess_fwd = MALLOC_16(this->nguess,DipoleForward*);
    for (k = 0; k < this->nguess; k++)
        this->guess_fwd[k] = NULL;
    /*
        * Compute the guesses using the sphere model for speed
        */
    orig = f->funcs;
    if (f->fit_mag_dipoles)
        f->funcs = f->mag_dipole_funcs;
    else
        f->funcs = f->sphere_funcs;

    for (k = 0; k < this->nguess; k++) {
        if ((this->guess_fwd[k] = DipoleFitData::dipole_forward_one(f,this->rr[k],NULL)) == NULL)
            goto bad;
#ifdef DEBUG
        sing = this->guess_fwd[k]->sing;
        printf("%f %f %f\n",sing[0],sing[1],sing[2]);
#endif
    }
    f->funcs = orig;

    fprintf(stderr,"[done %d sources]\n",p);

    return;
//    return res;

bad : {
        if(guesses)
            delete guesses;

        return;
//        return NULL;
    }
}


//*************************************************************************************************************

GuessData::GuessData(const QString &guessname, const QString &guess_surfname, float mindist, float exclude, float grid, DipoleFitData *f, char *guess_save_name)
{
    MneSurfaceOrVolume::MneCSourceSpace* *sp = NULL;
    int             nsp = 0;
    GuessData*      res = NULL;
    int             k,p;
    float           guessrad = 0.080f;
    MneSurfaceOrVolume::MneCSourceSpace*  guesses = NULL;

    if (!guessname.isEmpty()) {
        /*
         * Read the guesses and transform to the appropriate coordinate frame
         */
        if (MneSurfaceOrVolume::mne_read_source_spaces(guessname,&sp,&nsp) == FIFF_FAIL)
            goto bad;
        if (nsp != 1) {
            qCritical("Incorrect number of source spaces in guess file");
            for (k = 0; k < nsp; k++)
                delete sp[k];
            FREE_16(sp);
            goto bad;
        }
        printf("Read guesses from %s\n",guessname.toLatin1().constData());
        guesses = sp[0]; FREE_16(sp);
    }
    else {
        MneSurfaceOrVolume::MneCSurface*     inner_skull = NULL;
        int            free_inner_skull = FALSE;
        float          r0[3];

        VEC_COPY_16(r0,f->r0);
        fiff_coord_trans_inv_16(r0,f->mri_head_t,TRUE);
        if (f->bem_model) {
            printf("Using inner skull surface from the BEM (%s)...\n",f->bemname);
            if ((inner_skull = f->bem_model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN)) == NULL)
                goto bad;
        }
        else if (!guess_surfname.isEmpty()) {
            printf("Reading inner skull surface from %s...\n",guess_surfname.toLatin1().data());
            if ((inner_skull = MneSurfaceOrVolume::read_bem_surface(guess_surfname,FIFFV_BEM_SURF_ID_BRAIN,TRUE,NULL)) == NULL)
                goto bad;
            free_inner_skull = TRUE;
        }
        if ((guesses = MneSurfaceOrVolume::MneCSurface::make_guesses(inner_skull,guessrad,r0,grid,exclude,mindist)) == NULL)
            goto bad;
        if (free_inner_skull)
            delete inner_skull;
    }
    /*
       * Save the guesses for future use
       */
    if (guesses->nuse == 0) {
        qCritical("No active guess locations remaining.");
        goto bad;
    }
    if (guess_save_name) {
        printf("###################DEBUG writing source spaces not yet implemented.");
        //    if (mne_write_source_spaces(guess_save_name,&guesses,1,FALSE) != OK)
        //      goto bad;
        //    printf("Wrote guess locations to %s\n",guess_save_name);
    }
    /*
       * Transform the guess locations to the appropriate coordinate frame
       */
    if (mne_transform_source_spaces_to(f->coord_frame,f->mri_head_t,&guesses,1) != OK)
        goto bad;
    printf("Guess locations are now in %s coordinates.\n",mne_coord_frame_name_16(f->coord_frame));

    res = new GuessData();
    this->nguess  = guesses->nuse;
    this->rr      = ALLOC_CMATRIX_16(guesses->nuse,3);
    for (k = 0, p = 0; k < guesses->np; k++)
        if (guesses->inuse[k]) {
            VEC_COPY_16(this->rr[p],guesses->rr[k]);
            p++;
        }
    if(guesses)
        delete guesses;
    guesses = NULL;

    this->guess_fwd = MALLOC_16(this->nguess,DipoleForward*);
    for (k = 0; k < this->nguess; k++)
        this->guess_fwd[k] = NULL;
    /*
        * Compute the guesses using the sphere model for speed
        */
    if (!this->compute_guess_fields(f))
        goto bad;

    return;
//    return res;

bad : {
        if(guesses)
            delete guesses;
        delete res;
        return;
//        return NULL;
    }
}


//*************************************************************************************************************

GuessData::~GuessData()
{
    FREE_CMATRIX_16(rr);
    if (guess_fwd) {
        for (int k = 0; k < nguess; k++)
            delete guess_fwd[k];
        FREE_16(guess_fwd);
    }
    return;
}


//*************************************************************************************************************

bool GuessData::compute_guess_fields(DipoleFitData* f)
{
    dipoleFitFuncs orig = NULL;

    if (!f) {
        qCritical("Data missing in compute_guess_fields");
        return false;
    }
    if (!f->noise) {
        qCritical("Noise covariance missing in compute_guess_fields");
        return false;
    }
    printf("Go through all guess source locations...");
    orig = f->funcs;
    if (f->fit_mag_dipoles)
        f->funcs = f->mag_dipole_funcs;
    else
        f->funcs = f->sphere_funcs;
    for (int k = 0; k < this->nguess; k++) {
        if ((this->guess_fwd[k] = DipoleFitData::dipole_forward_one(f,this->rr[k],this->guess_fwd[k])) == NULL){
            if (orig)
                f->funcs = orig;
            return false;
        }
#ifdef DEBUG
        sing = this->guess_fwd[k]->sing;
        printf("%f %f %f\n",sing[0],sing[1],sing[2]);
#endif
    }
    f->funcs = orig;
    printf("[done %d sources]\n",this->nguess);

    return true;
}
