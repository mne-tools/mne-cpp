//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2016
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
* @brief    Example of dipole fit
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>


#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/sphere.h>












#include <stdio.h>
#include <string.h>



//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;



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



//============================= dot.h =============================

#define X 0
#define Y 1
#define Z 2
/*
 * Dot product and length
 */
#define VEC_DOT(x,y) ((x)[X]*(y)[X] + (x)[Y]*(y)[Y] + (x)[Z]*(y)[Z])
#define VEC_LEN(x) sqrt(VEC_DOT(x,x))



//============================= mne_allocs.h =============================

/*
 * Basics...
 */
#define MALLOC(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define FREE(x) if ((char *)(x) != NULL) free((char *)(x))
/*
 * Float, double, and int arrays
 */
#define ALLOC_FLOAT(x) MALLOC(x,float)
#define ALLOC_DOUBLE(x) MALLOC(x,double)
#define ALLOC_INT(x) MALLOC(x,int)
#define REALLOC_FLOAT(x,y) REALLOC(x,y,float)
#define REALLOC_DOUBLE(x,y) REALLOC(x,y,double)
#define REALLOC_INT(x,y) REALLOC(x,y,int)
/*
 * float matrices
 */
#define ALLOC_CMATRIX(x,y) mne_cmatrix((x),(y))
#define FREE_CMATRIX(m) mne_free_cmatrix((m))
#define CMATRIX ALLOC_CMATRIX


//============================= mne_allocs.c =============================

static void matrix_error(int kind, int nr, int nc)

{
  if (kind == 1)
    fprintf(stderr,"Failed to allocate memory pointers for a %d x %d matrix\n",nr,nc);
  else if (kind == 2)
    fprintf(stderr,"Failed to allocate memory for a %d x %d matrix\n",nr,nc);
  else
    fprintf(stderr,"Allocation error for a %d x %d matrix\n",nr,nc);
  if (sizeof(void *) == 4) {
    fprintf(stderr,"This is probably because you seem to be using a computer with 32-bit architecture.\n");
    fprintf(stderr,"Please consider moving to a 64-bit platform.");
  }
  fprintf(stderr,"Cannot continue. Sorry.\n");
  exit(1);
}

float **mne_cmatrix(int nr,int nc)

{
  int i;
  float **m;
  float *whole;

  m = MALLOC(nr,float *);
  if (!m) matrix_error(1,nr,nc);
  whole = MALLOC(nr*nc,float);
  if (!whole) matrix_error(2,nr,nc);

  for(i=0;i<nr;i++)
    m[i] = whole + i*nc;
  return m;
}

void mne_free_cmatrix (float **m)
{
  if (m) {
    FREE(*m);
    FREE(m);
  }
}

/*
 * float matrices
 */

#define FREE_CMATRIX(m) mne_free_cmatrix((m))


//============================= mne_types.h =============================

typedef void (*mneUserFreeFunc)(void *);  /* General purpose */







typedef struct {		/* Matrix specification with a channel list */
  int   nrow;			/* Number of rows */
  int   ncol;			/* Number of columns */
  char  **rowlist;		/* Name list for the rows (may be NULL) */
  char  **collist;		/* Name list for the columns (may be NULL) */
  float **data;                  /* The data itself (dense) */
} *mneNamedMatrix,mneNamedMatrixRec;

//typedef struct {		/* Matrix specification with a channel list */
//  int   nrow;			/* Number of rows (same as in data) */
//  int   ncol;			/* Number of columns (same as in data) */
//  char  **rowlist;		/* Name list for the rows (may be NULL) */
//  char  **collist;		/* Name list for the columns (may be NULL) */
//  mneSparseMatrix data;		/* The data itself (sparse) */
//} *mneSparseNamedMatrix,mneSparseNamedMatrixRec;

//typedef struct {		/* Vector specification with a channel list */
//  int    nvec;			/* Number of elements */
//  char   **names;		/* Name list for the elements */
//  float  *data;			/* The data itself */
//} *mneNamedVector,mneNamedVectorRec;

typedef struct {		/* One linear projection item */
  mneNamedMatrix vecs;          /* The original projection vectors */
  int            nvec;          /* Number of vectors = vecs->nrow */
  char           *desc;	        /* Projection item description */
  int            kind;          /* Projection item kind */
  int            active;	/* Is this item active now? */
  int            active_file;	/* Was this item active when loaded from file? */
  int            has_meg;	/* Does it have MEG channels? */
  int            has_eeg;	/* Does it have EEG channels? */
} *mneProjItem,mneProjItemRec;

typedef struct {		/* Collection of projection items and the projector itself */
  mneProjItem    *items;        /* The projection items */
  int            nitems;        /* Number of items */
  char           **names;	/* Names of the channels in the final projector */
  int            nch;	        /* Number of channels in the final projector */
  int            nvec;          /* Number of vectors in the final projector */
  float          **proj_data;	/* The orthogonalized projection vectors picked and orthogonalized from the original data */
} *mneProjOp,mneProjOpRec;



//============================= misc_util.c =============================

char *mne_strdup(const char *s)
{
  char *res;
  if (s == NULL)
    return NULL;
  res = (char*) malloc(strlen(s)+1);
  strcpy(res,s);
  return res;
}




//============================= fwd_types.h =============================

#define FWD_COIL_UNKNOWN      0

#define FWD_COILC_UNKNOWN     0
#define FWD_COILC_EEG         1000
#define FWD_COILC_MAG         1
#define FWD_COILC_AXIAL_GRAD  2
#define FWD_COILC_PLANAR_GRAD 3
#define FWD_COILC_AXIAL_GRAD2 4

#define FWD_COIL_ACCURACY_POINT    0
#define FWD_COIL_ACCURACY_NORMAL   1
#define FWD_COIL_ACCURACY_ACCURATE 2

#define FWD_IS_MEG_COIL(x) ((x) != FWD_COILC_EEG && (x) != FWD_COILC_UNKNOWN)

typedef void (*fwdUserFreeFunc)(void *);  /* General purpose */

typedef struct {
    char         *chname;       /* Name of this channel */
    int          coord_frame;   /* Which coordinate frame are we in? */
    char         *desc;         /* Description for this type of a coil */
    int          coil_class;    /* Coil class */
    int          type;          /* Coil type */
    int          accuracy;      /* Accuracy */
    float        size;          /* Coil size */
    float        base;          /* Baseline */
    float        r0[3];         /* Coil coordinate system origin */
    float        ex[3];         /* Coil coordinate system unit vectors */
    float        ey[3];         /* This stupid construction needs to be replaced with */
    float        ez[3];         /* a coordinate transformation */
    int          np;            /* Number of integration points */
    float        **rmag;        /* The field point locations */
    float        **cosmag;      /* The corresponding direction cosines */
    float        *w;            /* The weighting coefficients */
} *fwdCoil,fwdCoilRec;


typedef struct {
  fwdCoil *coils;                   /* The coil or electrode positions */
  int     ncoil;
  int     coord_frame;              /* Common coordinate frame */
  void    *user_data;               /* We can put whatever in here */
  fwdUserFreeFunc user_data_free;
} *fwdCoilSet,fwdCoilSetRec;        /* A collection of the above */

/*
 * This is a convenient generic field / potential computation function
 */
typedef int (*fwdFieldFunc)(float *rd,float *Q,fwdCoilSet coils,float *res,void *client);
typedef int (*fwdVecFieldFunc)(float *rd,fwdCoilSet coils,float **res,void *client);
typedef int (*fwdFieldGradFunc)(float *rd,float *Q,fwdCoilSet coils, float *res,
                float *xgrad, float *ygrad, float *zgrad, void *client);


/*
 * This is used in the function which evaluates integrals over triangles
 */

typedef struct {
    double **xyz;                   /* The coordinates */
    double *w;                      /* Weights */
    int    np;                      /* How many */
} *fwdIntPoints,fwdIntPointsRec;    /* Integration points in a triangle */

typedef double (*fwdIntApproxEvalFunc)(fwdIntPoints int_points, double *r0, void *user);

/*
 * Definitions for the EEG sphere model
 */

typedef struct {
    float rad;                              /* The actual rads */
    float rel_rad;                          /* Relative rads */
    float sigma;                            /* Conductivity */
} *fwdEegSphereLayer,fwdEegSphereLayerRec;


typedef struct {
  char  *name;              /* Textual identifier */
  int   nlayer;             /* Number of layers */
  fwdEegSphereLayer layers; /* An array of layers */
  float  r0[3];             /* The origin */

  double *fn;           /* Coefficients saved to speed up the computations */
  int    nterms;        /* How many? */

  float  *mu;           /* The Berg-Scherg equivalence parameters */
  float  *lambda;
  int    nfit;          /* How many? */
  int    scale_pos;     /* Scale the positions to the surface of the sphere? */
} *fwdEegSphereModel,fwdEegSphereModelRec;

typedef struct {
  fwdEegSphereModel *models;    /* Set of EEG sphere model definitions */
  int               nmodel;
} *fwdEegSphereModelSet,fwdEegSphereModelSetRec;

#define FWD_BEM_UNKNOWN           -1
#define FWD_BEM_CONSTANT_COLL     1
#define FWD_BEM_LINEAR_COLL       2

#define FWD_BEM_IP_APPROACH_LIMIT 0.1

#define FWD_BEM_LIN_FIELD_SIMPLE    1
#define FWD_BEM_LIN_FIELD_FERGUSON  2
#define FWD_BEM_LIN_FIELD_URANKAR   3

typedef struct {                    /* Space to store a solution matrix */
    float **solution;               /* The solution matrix */
    int   ncoil;                    /* Number of sensors */
    int   np;                       /* Number of potential solution points */
} *fwdBemSolution,fwdBemSolutionRec;/* Mapping from infinite medium potentials to a particular set of coils or electrodes */









typedef struct {
  char       *surf_name;    /* Name of the file where surfaces were loaded from */
  MNESurface *surfs;        /* The interface surfaces from outside towards inside */
  int        *ntri;         /* Number of triangles on each surface */
  int        *np;           /* Number of vertices on each surface */
  int        nsurf;         /* How many */
  float      *sigma;        /* The conductivities */
  float      **gamma;       /* The gamma factors */
  float      *source_mult;  /* These multiply the infinite medium potentials */
  float      *field_mult;   /* Multipliers for the magnetic field */
  int        bem_method;    /* Which approximation method is used */
  char       *sol_name;     /* Name of the file where the solution was loaded from */

  float      **solution;    /* The potential solution matrix */
  float      *v0;           /* Space for the infinite-medium potentials */
  int        nsol;          /* Size of the solution matrix */

  FiffCoordTrans head_mri_t;    /* Coordinate transformation from head to MRI coordinates */

  float      ip_approach_limit; /* Controls whether we need to use the isolated problem approach */
  int        use_ip_approach;   /* Do we need it */
} *fwdBemModel,fwdBemModelRec;  /* Holds the BEM model definition */




//============================= fit_types.h =============================

/*
 * These are the type definitions for dipole fitting
 */
typedef void (*fitUserFreeFunc)(void *);

typedef struct {
  float **rd;       /* Dipole locations */
  int   ndip;       /* How many dipoles */
  float **fwd;      /* The forward solution (projected and whitened) */
  float *scales;    /* Scales applied to the columns of fwd */
  float **uu;       /* The left singular vectors of the forward matrix */
  float **vv;       /* The right singular vectors of the forward matrix */
  float *sing;      /* The singular values */
  int   nch;        /* Number of channels */
} *dipoleForward,dipoleForwardRec;

typedef struct {
    float          **rr;        /* These are the guess dipole locations */
    dipoleForward  *guess_fwd;  /* Forward solutions for the guesses */
    int            nguess;      /* How many sources */
} *guessData,guessDataRec;


typedef struct {
    fwdFieldFunc    meg_field;          /* MEG forward calculation functions */
    fwdVecFieldFunc meg_vec_field;
    void            *meg_client;        /* Client data for MEG field computations */
    mneUserFreeFunc meg_client_free;

    fwdFieldFunc    eeg_pot;            /* EEG forward calculation functions */
    fwdVecFieldFunc eeg_vec_pot;
    void            *eeg_client;        /* Client data for EEG field computations */
    mneUserFreeFunc eeg_client_free;
} *dipoleFitFuncs,dipoleFitFuncsRec;

#define COLUMN_NORM_NONE 0      /* No column normalization requested */
#define COLUMN_NORM_COMP 1      /* Componentwise normalization */
#define COLUMN_NORM_LOC  2      /* Dipole locationwise normalization */

typedef struct {                        /* This structure holds all fitting-related data */
    FiffCoordTrans    mri_head_t;       /* MRI <-> head coordinate transformation */
    FiffCoordTrans    meg_head_t;       /* MEG <-> head coordinate transformation */
    int               coord_frame;      /* Common coordinate frame */
    QList<FiffChInfo> chs;              /* Channels */
    int               nmeg;             /* How many MEG */
    int               neeg;             /* How many EEG */
    char              **ch_names;       /* List of all channel names */
    RowVectorXf       pick;             /* Matrix to pick data from the
                                           full data set which may contain channels
                                           we are not interested in */
    fwdCoilSet        meg_coils;        /* MEG coil definitions */
    fwdCoilSet        eeg_els;          /* EEG electrode definitions */
    float             r0[3];            /* Sphere model origin */
    char              *bemname;         /* Using a BEM? */

    fwdEegSphereModel eeg_model;        /* EEG sphere model definition */
    fwdBemModel       bem_model;        /* BEM model definition */

    dipoleFitFuncs    sphere_funcs;     /* These are the sphere model forward functions */
    dipoleFitFuncs    bem_funcs;        /* These are the BEM forward functions */
    dipoleFitFuncs    funcs;            /* Points to one of the two above */
    dipoleFitFuncs    mag_dipole_funcs; /* Functions to fit a magnetic dipole */

    int               fixed_noise;      /* Were fixed noise values used rather than a noise-covariance
                                         * matrix read from a file */
    FiffCov           noise_orig;       /* Noise covariance matrix (original) */
    FiffCov           noise;            /* Noise covariance matrix (weighted to take the selection into account) */
    int               nave;             /* How many averages does this correspond to? */
    mneProjOp         proj;             /* The projection operator to use */
    int               column_norm;      /* What kind of column normalization to apply to the forward solution */
    int               fit_mag_dipoles;  /* Fit magnetic dipoles? */
    void              *user;            /* User data for anything we need */
    fitUserFreeFunc   user_free;        /* Function to free the above */
} *dipoleFitData,dipoleFitDataRec;



//============================= dipole_forward.c =============================

void free_dipole_forward ( dipoleForward f )
{
    if (!f)
        return;
    FREE_CMATRIX(f->rd);
    FREE_CMATRIX(f->fwd);
    FREE_CMATRIX(f->uu);
    FREE_CMATRIX(f->vv);
    FREE(f->sing);
    FREE(f->scales);
    FREE(f);
    return;
}







//============================= fiff_trans.c =============================


void fiff_coord_trans (float r[3],FiffCoordTrans* t,int do_move)
     /*
      * Apply coordinate transformation
      */
{
  int j,k;
  float res[3];

  for (j = 0; j < 3; j++) {
    res[j] = (do_move ? t->trans(j,3) :  0.0);
    for (k = 0; k < 3; k++)
      res[j] += t->trans(j,k)*r[k];
  }
  for (j = 0; j < 3; j++)
    r[j] = res[j];
}


//============================= fwd_coil_def.c =============================

#define MAXWORD 1000

#define BIG 0.5



/*
 * This is the old interface which should be eventually deleted
 */
static fwdCoil    fwd_new_coil(int np)

{
  fwdCoil res = MALLOC(1,fwdCoilRec);
  int     k;

  res->chname     = NULL;
  res->desc       = NULL;
  res->coil_class = FWD_COILC_UNKNOWN;
  res->accuracy   = FWD_COIL_ACCURACY_POINT;
  res->base       = 0.0;
  res->size       = 0.0;
  res->np         = np;
  res->rmag       = ALLOC_CMATRIX(np,3);
  res->cosmag     = ALLOC_CMATRIX(np,3);
  res->w          = MALLOC(np,float);
  /*
   * Reasonable defaults
   */
  for (k = 0; k < 3; k++) {
    res->r0[k] = 0.0;
    res->ex[k] = 0.0;
    res->ey[k] = 0.0;
    res->ez[k] = 0.0;
  }
  res->ex[0] = 1.0;
  res->ey[1] = 1.0;
  res->ez[2] = 1.0;

  return res;
}

static void fwd_free_coil(fwdCoil coil)

{
  if (!coil)
    return;

  FREE(coil->chname);
  FREE(coil->desc);
  FREE_CMATRIX(coil->rmag);
  FREE_CMATRIX(coil->cosmag);
  FREE(coil->w);
  FREE(coil);
}


fwdCoilSet fwd_new_coil_set()

{
  fwdCoilSet s = MALLOC(1,fwdCoilSetRec);

  s->coils = NULL;
  s->ncoil = 0;
  s->coord_frame = FIFFV_COORD_UNKNOWN;
  s->user_data = NULL;
  s->user_data_free = NULL;
  return s;
}

void fwd_free_coil_set_user_data(fwdCoilSet set)

{
  if (!set)
    return;
  if (set->user_data_free && set->user_data)
    set->user_data_free(set->user_data);
  set->user_data = NULL;
  return;
}

void fwd_free_coil_set(fwdCoilSet set)

{
  int k;

  if (!set)
    return;

  for (k = 0; k < set->ncoil; k++)
    fwd_free_coil(set->coils[k]);
  FREE(set->coils);

  fwd_free_coil_set_user_data(set);

  FREE(set);
  return;
}

int fwd_is_axial_coil(fwdCoil coil)

{
  return (coil->coil_class == FWD_COILC_MAG ||
      coil->coil_class == FWD_COILC_AXIAL_GRAD ||
      coil->coil_class == FWD_COILC_AXIAL_GRAD2);
}

int fwd_is_magnetometer_coil(fwdCoil coil)

{
  return coil->coil_class == FWD_COILC_MAG;
}

int fwd_is_planar_coil(fwdCoil coil)

{
  return coil->coil_class == FWD_COILC_PLANAR_GRAD;
}

int fwd_is_eeg_electrode(fwdCoil coil)

{
  return coil->coil_class == FWD_COILC_EEG;
}

int fwd_is_planar_coil_type(int type,            /* This is the coil type we are interested in */
                fwdCoilSet set)	 /* Set of templates */

{
  int k;

  if (type == FIFFV_COIL_EEG)
    return FALSE;
  if (!set)
    return FALSE;
  for (k = 0; k < set->ncoil; k++)
    if (set->coils[k]->type == type)
      return set->coils[k]->coil_class == FWD_COILC_PLANAR_GRAD;
  return FALSE;
}


int fwd_is_axial_coil_type(int type,             /* This is the coil type we are interested in */
               fwdCoilSet set)	 /* Set of templates */

{
  int k;

  if (type == FIFFV_COIL_EEG)
    return FALSE;
  if (!set)
    return FALSE;
  for (k = 0; k < set->ncoil; k++)
    if (set->coils[k]->type == type)
      return (set->coils[k]->coil_class == FWD_COILC_MAG ||
          set->coils[k]->coil_class == FWD_COILC_AXIAL_GRAD ||
          set->coils[k]->coil_class == FWD_COILC_AXIAL_GRAD2);
  return FALSE;
}

int fwd_is_magnetometer_coil_type(int type,             /* This is the coil type we are interested in */
                  fwdCoilSet set)	/* Set of templates */

{
  int k;

  if (type == FIFFV_COIL_EEG)
    return FALSE;
  if (!set)
    return FALSE;
  for (k = 0; k < set->ncoil; k++)
    if (set->coils[k]->type == type)
      return set->coils[k]->coil_class == FWD_COILC_MAG;
  return FALSE;
}

int fwd_is_eeg_electrode_type(int type,		 /*  */
                  fwdCoilSet set)	 /* Templates are included for symmetry */

{
  return type == FIFFV_COIL_EEG;
}


static void normalize(float *rr)
     /*
      * Scale vector to unit length
      */
{
  float ll = VEC_LEN(rr);
  int k;
  if (ll > 0) {
    for (k = 0; k < 3; k++)
      rr[k] = rr[k]/ll;
  }
  return;
}

static fwdCoil fwd_add_coil_to_set(fwdCoilSet set,
                   int type, int coil_class, int acc, int np, float size, float base, char *desc)

{
  fwdCoil def;

  if (set == NULL) {
    qDebug() << "qCritical (No coil definition set to augment.)";
    return NULL;
  }
  if (np <= 0) {
    qDebug() << "err_printf_set_error(Number of integration points should be positive (type = %d acc = %d),type,acc)";
    return NULL;
  }
  if (! (acc == FWD_COIL_ACCURACY_POINT ||
        acc == FWD_COIL_ACCURACY_NORMAL ||
        acc == FWD_COIL_ACCURACY_ACCURATE) ) {
    qDebug() << "err_printf_set_error(Illegal accuracy (type = %d acc = %d),type,acc)";
    return NULL;
  }
  if (! (coil_class == FWD_COILC_MAG ||
        coil_class == FWD_COILC_AXIAL_GRAD ||
        coil_class == FWD_COILC_PLANAR_GRAD ||
        coil_class == FWD_COILC_AXIAL_GRAD2) ) {
    qDebug() << "err_printf_set_error(Illegal coil class (type = %d acc = %d class = %d),type,acc,coil_class)";
    return NULL;
  }

  set->coils = REALLOC(set->coils,set->ncoil+1,fwdCoil);
  def = set->coils[set->ncoil++] = fwd_new_coil(np);

  def->type       = type;
  def->coil_class = coil_class;
  def->accuracy   = acc;
  def->np         = np;
  def->base       = size;
  def->base       = base;
  if (desc)
    def->desc = mne_strdup(desc);
  return def;
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
  char *next = MALLOC(MAXWORD,char);
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
    FREE(next);
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
    qDebug() << "err_printf_set_error(missing integer)";
    return FAIL;
  }
  else if (sscanf(next,"%d",ival) != 1) {
    qDebug() << "err_printf_set_error(bad integer : %s,next)";
    FREE(next);
    return FAIL;
  }
  FREE(next);
  return OK;
}

static int get_fval(FILE *in, float *fval)

{
  char *next = next_word(in);
  if (next == NULL) {
    qDebug() << "qCritical (bad integer)";
    return FAIL;
  }
  else if (sscanf(next,"%g",fval) != 1) {
    qDebug() << "err_printf_set_error(bad floating point number : %s,next)";
    FREE(next);
    return FAIL;
  }
  FREE(next);
  return OK;
}






fwdCoilSet fwd_read_coil_defs(char *name)
     /*
      * Read a coil definition file
      */
{
  FILE    *in = fopen(name,"r");
  char    *desc = NULL;
  int     type,coil_class,acc,np;
  int     p;
  float   size,base;
  fwdCoilSet res = NULL;
  fwdCoil def;

  if (in == NULL) {
    qDebug() << "err_set_sys_error(name)";
    goto bad;
  }

  res = fwd_new_coil_set();
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
    FREE(desc); desc = NULL;

    for (p = 0; p < def->np; p++) {
      /*
       * Read and verify data for each integration point
       */
      if (get_fval(in,def->w+p) != OK)
    goto bad;
      if (get_fval(in,def->rmag[p]+X) != OK)
    goto bad;
      if (get_fval(in,def->rmag[p]+Y) != OK)
    goto bad;
      if (get_fval(in,def->rmag[p]+Z) != OK)
    goto bad;
      if (get_fval(in,def->cosmag[p]+X) != OK)
    goto bad;
      if (get_fval(in,def->cosmag[p]+Y) != OK)
    goto bad;
      if (get_fval(in,def->cosmag[p]+Z) != OK)
    goto bad;

    if (VEC_LEN(def->rmag[p]) > BIG) {
        qDebug() << "err_printf_set_error(Unreasonable integration point: %f %f %f mm (coil type = %d acc = %d), 1000*def->rmag[p][X],1000*def->rmag[p][Y],1000*def->rmag[p][Z], def->type,def->accuracy)";
    goto bad;
      }
      size = VEC_LEN(def->cosmag[p]);
      if (size <= 0) {
        qDebug() << "err_printf_set_error(Unreasonable normal: %f %f %f (coil type = %d acc = %d), def->cosmag[p][X],def->cosmag[p][Y],def->cosmag[p][Z], def->type,def->accuracy)";
    goto bad;
      }
      normalize(def->cosmag[p]);
    }
  }
  fprintf(stderr,"%d coil definitions read\n",res->ncoil);
  return res;

  bad : {
    fwd_free_coil_set(res);
    FREE(desc);
    return NULL;
  }
}


fwdCoil fwd_create_meg_coil(fwdCoilSet     set,      /* These are the available coil definitions */
                FiffChInfo     ch,       /* Channel information to use */
                int            acc,	     /* Required accuracy */
                FiffCoordTrans* t)	     /* Transform the points using this */
     /*
      * Create a MEG coil definition using a database of templates
      * Change the coordinate frame if so desired
      */
{
  int        k,p,c;
  fwdCoil    def;
  fwdCoil    res = NULL;

  if (ch.kind != FIFFV_MEG_CH && ch.kind != FIFFV_REF_MEG_CH) {
    qDebug() << "err_printf_set_error(%s is not a MEG channel. Cannot create a coil definition.,ch->ch_name)";
    goto bad;
  }
  /*
   * Simple linear search from the coil definitions
   */
  for (k = 0, def = NULL; k < set->ncoil; k++) {
    if ((set->coils[k]->type == (ch.coil_type & 0xFFFF)) &&
    set->coils[k]->accuracy == acc) {
      def = set->coils[k];
    }
  }
  if (!def) {
    qDebug() << "err_printf_set_error(Desired coil definition not found (type = %d acc = %d),ch->chpos.coil_type,acc)";
    goto bad;
  }
  /*
   * Create the result
   */
  res = fwd_new_coil(def->np);

  res->chname   = mne_strdup(ch.ch_name.toLatin1().data());
  if (def->desc)
    res->desc   = mne_strdup(def->desc);
  res->coil_class = def->coil_class;
  res->accuracy   = def->accuracy;
  res->base       = def->base;
  res->size       = def->size;
  res->type       = ch.coil_type;

  res->r0[0] = ch.loc[0];
  res->r0[1] = ch.loc[1];
  res->r0[2] = ch.loc[2];
  res->ex[0] = ch.loc[3];
  res->ex[1] = ch.loc[4];
  res->ex[2] = ch.loc[5];
  res->ey[0] = ch.loc[6];
  res->ey[1] = ch.loc[7];
  res->ey[2] = ch.loc[8];
  res->ez[0] = ch.loc[9];
  res->ez[1] = ch.loc[10];
  res->ez[2] = ch.loc[11];
//  VEC_COPY(res->r0,ch->chpos.r0);
//  VEC_COPY(res->ex,ch->chpos.ex);
//  VEC_COPY(res->ey,ch->chpos.ey);
//  VEC_COPY(res->ez,ch->chpos.ez);
  /*
   * Apply a coordinate transformation if so desired
   */
  if (t) {
    fiff_coord_trans(res->r0,t,FIFFV_MOVE);
    fiff_coord_trans(res->ex,t,FIFFV_NO_MOVE);
    fiff_coord_trans(res->ey,t,FIFFV_NO_MOVE);
    fiff_coord_trans(res->ez,t,FIFFV_NO_MOVE);
    res->coord_frame = t->to;
  }
  else
    res->coord_frame = FIFFV_COORD_DEVICE;

  for (p = 0; p < res->np; p++) {
    res->w[p] = def->w[p];
    for (c = 0; c < 3; c++) {
      res->rmag[p][c]   = res->r0[c] + def->rmag[p][X]*res->ex[c] + def->rmag[p][Y]*res->ey[c] + def->rmag[p][Z]*res->ez[c];
      res->cosmag[p][c] = def->cosmag[p][X]*res->ex[c] + def->cosmag[p][Y]*res->ey[c] + def->cosmag[p][Z]*res->ez[c];
    }
  }
  return res;

  bad : {
    return NULL;
  }
}

fwdCoilSet fwd_create_meg_coils(fwdCoilSet      set,      /* These are the available coil definitions */
                QList<FiffChInfo>   chs,      /* Channel information to use */
                int             nch,
                int             acc,	  /* Required accuracy */
                FiffCoordTrans* t)	  /* Transform the points using this */

{

    fwdCoilSet res = fwd_new_coil_set();
    fwdCoil    next;
    int        k;

    for (k = 0; k < nch; k++) {
        if ((next = fwd_create_meg_coil(set,chs[k],acc,t)) == NULL)
            goto bad;

        res->coils = REALLOC(res->coils,res->ncoil+1,fwdCoil);
        res->coils[res->ncoil++] = next;
    }
    if (t)
        res->coord_frame = t->to;
    return res;

    bad : {
        fwd_free_coil_set(res);
        return NULL;
    }
}



fwdCoil fwd_create_eeg_el(  FiffChInfo*     ch,         /* Channel information to use */
                            FiffCoordTrans* t)     /* Transform the points using this */
/*
* Create an electrode definition. Transform coordinate frame if so desired.
*/
{
    fwdCoil    res = NULL;
    int        c;

    if (ch->kind != FIFFV_EEG_CH) {
        qDebug() << "(%s is not an EEG channel. Cannot create an electrode definition.,ch->ch_name)" << ch->ch_name;
        goto bad;
    }
    if (t && t->from != FIFFV_COORD_HEAD) {
        qDebug() << "err_printf_set_error(Inappropriate coordinate transformation in fwd_create_eeg_el)";
        goto bad;
    }

    float vec_len = sqrt(ch->loc[3]*ch->loc[3] + ch->loc[4]*ch->loc[4] + ch->loc[5]*ch->loc[5]); //VEC_LEN(ch->chpos.ex)
    if ( vec_len < 1e-4)
        res = fwd_new_coil(1);          /* No reference electrode */
    else
        res = fwd_new_coil(2);          /* Reference electrode present */

    res->chname     = mne_strdup(ch->ch_name.toLatin1().data());
    res->desc       = mne_strdup("EEG electrode");
    res->coil_class = FWD_COILC_EEG;
    res->accuracy   = FWD_COIL_ACCURACY_NORMAL;
    res->type       = ch->coil_type;//ch->chpos.coil_type;
//    VEC_COPY(res->r0,ch->chpos.r0);
    res->r0[0] = ch->loc(0);
    res->r0[1] = ch->loc(1);
    res->r0[2] = ch->loc(2);
//    VEC_COPY(res->ex,ch->chpos.ex);
    res->ex[0] = ch->loc(3);
    res->ex[1] = ch->loc(4);
    res->ex[2] = ch->loc(5);
    /*
    * Optional coordinate transformation
    */
    if (t) {
        fiff_coord_trans(res->r0,t,FIFFV_MOVE);
        fiff_coord_trans(res->ex,t,FIFFV_MOVE);
        res->coord_frame = t->to;
    }
    else
        res->coord_frame = FIFFV_COORD_HEAD;
    /*
    * The electrode location
    */
    for (c = 0; c < 3; c++)
        res->rmag[0][c] = res->cosmag[0][c] = res->r0[c];
    normalize(res->cosmag[0]);
    res->w[0] = 1.0;
    /*
    * Add the reference electrode, if appropriate
    */
    if (res->np == 2) {
        for (c = 0; c < 3; c++)
            res->rmag[1][c] = res->cosmag[1][c] = res->ex[c];
        normalize(res->cosmag[1]);
        res->w[1] = -1.0;
    }
    return res;

    bad : {
        return NULL;
    }
}


fwdCoilSet fwd_create_eeg_els(  QList<FiffChInfo> chs,      /* Channel information to use */
                                int start,
                                int             nch,
                                FiffCoordTrans* t )     /* Transform the points using this */

{
    fwdCoilSet res = fwd_new_coil_set();
    fwdCoil    next;
    int        k;

    for (k = start; k < start+nch; k++) {
        if ((next = fwd_create_eeg_el(&chs[k],t)) == NULL)
            goto bad;
        res->coils = REALLOC(res->coils,res->ncoil+1,fwdCoil);
        res->coils[res->ncoil++] = next;
    }
    if (t)
        res->coord_frame = t->to;
    return res;

    bad : {
        fwd_free_coil_set(res);
        return NULL;
    }
}



























//============================= setup.c =============================


guessData new_guess_data()
{
    guessData res = MALLOC(1,guessDataRec);

    res->rr        = NULL;
    res->guess_fwd = NULL;
    res->nguess    = 0;
    return res;
}

static void free_guess_data(guessData g)

{
    int k;
    if (!g)
        return;

    FREE_CMATRIX(g->rr);
    if (g->guess_fwd) {
    for (k = 0; k < g->nguess; k++)
        free_dipole_forward(g->guess_fwd[k]);
    FREE(g->guess_fwd);
    }
    FREE(g);
    return;
}

static dipoleFitFuncs new_dipole_fit_funcs()
{
    dipoleFitFuncs f = MALLOC(1,dipoleFitFuncsRec);

    f->meg_field     = NULL;
    f->eeg_pot       = NULL;
    f->meg_vec_field = NULL;
    f->eeg_vec_pot   = NULL;
    f->meg_client      = NULL;
    f->meg_client_free = NULL;
    f->eeg_client      = NULL;
    f->eeg_client_free = NULL;

    return f;
}


static void free_dipole_fit_funcs(dipoleFitFuncs f)
{
    if (!f)
        return;

    if (f->meg_client_free && f->meg_client)
        f->meg_client_free(f->meg_client);
    if (f->eeg_client_free && f->eeg_client)
        f->eeg_client_free(f->eeg_client);

    FREE(f);
    return;
}


dipoleFitData new_dipole_fit_data()
{
  dipoleFitData res = new dipoleFitDataRec();//MALLOC(1,dipoleFitDataRec);

//  res->mri_head_t    = NULL;
//  res->meg_head_t    = NULL;
//  res->chs           = NULL;
    res->meg_coils     = NULL;
    res->eeg_els       = NULL;
    res->nmeg          = 0;
    res->neeg          = 0;
    res->r0[0]         = 0.0;
    res->r0[1]         = 0.0;
    res->r0[2]         = 0.0;
    res->bemname       = NULL;
    res->bem_model     = NULL;
    res->eeg_model     = NULL;
//  res->noise         = NULL;
    res->nave          = 1;
    res->user          = NULL;
    res->user_free     = NULL;
    res->proj          = NULL;

    res->sphere_funcs     = NULL;
    res->bem_funcs        = NULL;
    res->mag_dipole_funcs = NULL;
    res->funcs            = NULL;
    res->column_norm      = COLUMN_NORM_NONE;
    res->fit_mag_dipoles  = FALSE;

    return res;
}


//void free_dipole_fit_data(dipoleFitData d)
//{
//  if (!d)
//    return;

//  FREE(d->mri_head_t);
//  FREE(d->meg_head_t);
//  FREE(d->chs);
//  fwd_free_coil_set(d->meg_coils);
//  fwd_free_coil_set(d->eeg_els);
//  FREE(d->bemname);
//  mne_free_cov(d->noise);
//  mne_free_name_list(d->ch_names,d->nmeg+d->neeg);
//  fwd_bem_free_model(d->bem_model);
//  fwd_free_eeg_sphere_model(d->eeg_model);
//  if (d->user_free)
//    d->user_free(d->user);

//  mne_free_proj_op(d->proj);

//  free_dipole_fit_funcs(d->sphere_funcs);
//  free_dipole_fit_funcs(d->bem_funcs);
//  free_dipole_fit_funcs(d->mag_dipole_funcs);

//  FREE(d);
//  return;
//}






















//fwdEegSphereModel setup_eeg_sphere_model(char  *eeg_model_file,   /* Contains the model specifications */
//					 char  *eeg_model_name,	  /* Name of the model to use */
//					 float eeg_sphere_rad)    /* Outer surface radius */
//     /*
//      * Set up the desired sphere model for EEG
//      */
//{
//  fwdEegSphereModelSet eeg_models = NULL;
//  fwdEegSphereModel    eeg_model  = NULL;

//  if (!eeg_model_name)
//    eeg_model_name = mne_strdup("Default");
//  else
//    eeg_model_name = mne_strdup(eeg_model_name);

//  eeg_models = fwd_load_eeg_sphere_models(eeg_model_file,NULL);
//  fwd_list_eeg_sphere_models(stderr,eeg_models);

//  if ((eeg_model = fwd_select_eeg_sphere_model(eeg_model_name,eeg_models)) == NULL)
//    goto bad;
//  if (fwd_setup_eeg_sphere_model(eeg_model,eeg_sphere_rad,TRUE,3) == FAIL)
//    goto bad;
//  fprintf(stderr,"Using EEG sphere model \"%s\" with scalp radius %7.1f mm\n",
//	  eeg_model->name,1000*eeg_sphere_rad);
//  fprintf(stderr,"\n");
//  FREE(eeg_model_name);
//  fwd_free_eeg_sphere_model_set(eeg_models);
//  return eeg_model;

//  bad : {
//    fwd_free_eeg_sphere_model_set(eeg_models);
//    fwd_free_eeg_sphere_model(eeg_model);
//    FREE(eeg_model_name);
//    return NULL;
//  }
//}




dipoleFitData setup_dipole_fit_data(char  *mriname,		 /* This gives the MRI/head transform */
                    char  *measname,		 /* This gives the MEG/head transform and
                                  * sensor locations */
                    char  *bemname,		 /* BEM model */
                    float *r0,			 /* Sphere model origin in head coordinates (optional) */
                    fwdEegSphereModel eeg_model, /* EEG sphere model definition */
                    int   accurate_coils,	 /* Use accurate coil definitions? */
                    char  *badname,		 /* Bad channels list */
                    char  *noisename,		 /* Noise covariance matrix */
                    float grad_std,              /* Standard deviations for the ad-hoc noise cov (planar gradiometers) */
                    float mag_std,               /* Ditto for magnetometers */
                    float eeg_std,               /* Ditto for EEG */
                    float mag_reg,               /* Noise-covariance regularization factors */
                    float grad_reg,
                    float eeg_reg,
                    int   diagnoise,		 /* Use only the diagonal elements of the noise-covariance matrix */
                    char  **projnames,           /* SSP file names */
                    int   nproj,                 /* How many of them */
                    int   include_meg,           /* Include MEG in the fitting? */
                    int   include_eeg)           /* Include EEG in the fitting? */
     /*
      * Background work for modelling
      */
{
    dipoleFitData  res = new_dipole_fit_data();
    int            k;
    char           **badlist = NULL;
    int            nbad      = 0;
    char           **file_bads;
    int            file_nbad;
    int            coord_frame = FIFFV_COORD_HEAD;
//    mneCovMatrix cov;
    fwdCoilSet     templates = NULL;
//    mneCTFcompDataSet comp_data  = NULL;
//    fwdCoilSet        comp_coils = NULL;
//    /*
//    * Read the coordinate transformations
//    */
//    if (mriname) {
//    if ((res->mri_head_t = mne_read_mri_transform(mriname)) == NULL)
//    goto bad;
//    }
//    else if (bemname) {
//    qCritical ("Source of MRI / head transform required for the BEM model is missing");
//    goto bad;
//    }
//    else {
//    float move[] = { 0.0, 0.0, 0.0 };
//    float rot[3][3] = { { 1.0, 0.0, 0.0 },
//        { 0.0, 1.0, 0.0 },
//        { 0.0, 0.0, 1.0 } };
//    res->mri_head_t = fiff_make_transform(FIFFV_COORD_MRI,FIFFV_COORD_HEAD,rot,move);
//    }

//    mne_print_coord_transform(stderr,res->mri_head_t);
//    if ((res->meg_head_t = mne_read_meas_transform(measname)) == NULL)
//    goto bad;
//    mne_print_coord_transform(stderr,res->meg_head_t);
//    /*
//    * Read the bad channel lists
//    */
//    if (badname) {
//    if (mne_read_bad_channels(badname,&badlist,&nbad) != OK)
//    goto bad;
//    fprintf(stderr,"%d bad channels read from %s.\n",nbad,badname);
//    }
//    if (mne_read_bad_channel_list(measname,&file_bads,&file_nbad) == OK && file_nbad > 0) {
//    if (!badlist)
//    nbad = 0;
//    badlist = REALLOC(badlist,nbad+file_nbad,char *);
//    for (k = 0; k < file_nbad; k++)
//    badlist[nbad++] = file_bads[k];
//    FREE(file_bads);
//    fprintf(stderr,"%d bad channels read from the data file.\n",file_nbad);
//    }
//    fprintf(stderr,"%d bad channels total.\n",nbad);
//    /*
//    * Read the channel information
//    */
//    if (read_meg_eeg_ch_info(measname,include_meg,include_eeg,badlist,nbad,
//           &res->chs,&res->nmeg,&res->neeg) != OK)
//    goto bad;

//    if (res->nmeg > 0)
//        fprintf(stderr,"Will use %3d MEG channels from %s\n",res->nmeg,measname);
//    if (res->neeg > 0)
//        fprintf(stderr,"Will use %3d EEG channels from %s\n",res->neeg,measname);
//    {
//        char *s = mne_channel_names_to_string(res->chs,res->nmeg+res->neeg);
//        int  n;
//        mne_string_to_name_list(s,&res->ch_names,&n);
//    }
//    /*
//    * Make coil definitions
//    */
//    res->coord_frame = coord_frame;
//    if (coord_frame == FIFFV_COORD_HEAD) {
//        #ifdef USE_SHARE_PATH
//            char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
//        #else
//            char *coilfile = mne_compose_mne_name("setup/mne","coil_def.dat");
//        #endif

//        if (!coilfile)
//            goto bad;
//        if ((templates = fwd_read_coil_defs(coilfile)) == NULL) {
//            FREE(coilfile);
//            goto bad;
//        }

//        if ((res->meg_coils = fwd_create_meg_coils(templates,res->chs,res->nmeg,
//                           accurate_coils ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
//                           res->meg_head_t)) == NULL)
//            goto bad;
//        if ((res->eeg_els = fwd_create_eeg_els(res->chs+res->nmeg,res->neeg,NULL)) == NULL)
//            goto bad;
//        fprintf(stderr,"Head coordinate coil definitions created.\n");
//    }
//    else {
//        err_printf_set_error("Cannot handle computations in %s coordinates",mne_coord_frame_name(coord_frame));
//        goto bad;
//    }
//    /*
//    * Forward model setup
//    */
//    res->bemname   = mne_strdup(bemname);
//    if (r0) {
//    res->r0[0]     = r0[0];
//    res->r0[1]     = r0[1];
//    res->r0[2]     = r0[2];
//    }
//    res->eeg_model = eeg_model;
//    /*
//    * Compensation data
//    */
//    if ((comp_data = mne_read_ctf_comp_data(measname)) == NULL)
//        goto bad;
//    if (comp_data->ncomp > 0) {	/* Compensation channel information may be needed */
//        fiffChInfo comp_chs = NULL;
//        int        ncomp    = 0;

//        fprintf(stderr,"%d compensation data sets in %s\n",comp_data->ncomp,measname);
//        if (mne_read_meg_comp_eeg_ch_info(measname,NULL,0,&comp_chs,&ncomp,NULL,NULL,NULL,NULL) == FAIL)
//            goto bad;
//        if (ncomp > 0) {
//            if ((comp_coils = fwd_create_meg_coils(templates,comp_chs,ncomp,
//                     FWD_COIL_ACCURACY_NORMAL,res->meg_head_t)) == NULL) {
//                FREE(comp_chs);
//                goto bad;
//            }
//            fprintf(stderr,"%d compensation channels in %s\n",comp_coils->ncoil,measname);
//        }
//        FREE(comp_chs);
//    }
//    else {			/* Get rid of the empty data set */
//        mne_free_ctf_comp_data_set(comp_data);
//        comp_data = NULL;
//    }
//    /*
//    * Ready to set up the forward model
//    */
//    if (setup_forward_model(res,comp_data,comp_coils) == FAIL)
//        goto bad;
//    res->column_norm = COLUMN_NORM_LOC;
//    /*
//    * Projection data should go here
//    */
//    if (make_projection(projnames,nproj,res->chs,res->nmeg+res->neeg,&res->proj) == FAIL)
//        goto bad;
//    if (res->proj && res->proj->nitems > 0) {
//        fprintf(stderr,"Final projection operator is:\n");
//        mne_proj_op_report(stderr,"\t",res->proj);

//        if (mne_proj_op_chs(res->proj,res->ch_names,res->nmeg+res->neeg) == FAIL)
//            goto bad;
//        if (mne_proj_op_make_proj(res->proj) == FAIL)
//            goto bad;
//    }
//    /*
//    * Noise covariance
//    */
//    if (noisename) {
//    if ((cov = mne_read_cov(noisename,FIFFV_MNE_SENSOR_COV)) == NULL)
//    goto bad;
//    fprintf(stderr,"Read a %s noise-covariance matrix from %s\n",
//    cov->cov_diag ? "diagonal" : "full", noisename);
//    }
//    else {
//    if ((cov = ad_hoc_noise(res->meg_coils,res->eeg_els,grad_std,mag_std,eeg_std)) == NULL)
//    goto bad;
//    }
//    res->noise = mne_pick_chs_cov_omit(cov,res->ch_names,res->nmeg+res->neeg,TRUE,res->chs);
//    if (res->noise == NULL) {
//    mne_free_cov(cov);
//    goto bad;
//    }
//    fprintf(stderr,"Picked appropriate channels from the noise-covariance matrix.\n");
//    mne_free_cov(cov);
//    /*
//    * Apply the projection operator to the noise-covariance matrix
//    */
//    if (res->proj && res->proj->nitems > 0 && res->proj->nvec > 0) {
//    if (mne_proj_op_apply_cov(res->proj,res->noise) == FAIL)
//    goto bad;
//    fprintf(stderr,"Projection applied to the covariance matrix.\n");
//    }
//    /*
//    * Force diagonal noise covariance?
//    */
//    if (diagnoise) {
//    mne_revert_to_diag_cov(res->noise);
//    fprintf(stderr,"Using only the main diagonal of the noise-covariance matrix.\n");
//    }
//    /*
//    * Regularize the possibly deficient noise-covariance matrix
//    */
//    if (res->noise->cov) {
//    float regs[3];
//    int   do_it;

//    regs[MNE_COV_CH_MEG_MAG]  = mag_reg;
//    regs[MNE_COV_CH_MEG_GRAD] = grad_reg;
//    regs[MNE_COV_CH_EEG]      = eeg_reg;
//    /*
//    * Classify the channels
//    */
//    if (mne_classify_channels_cov(res->noise,res->chs,res->nmeg+res->neeg) == FAIL)
//    goto bad;
//    /*
//    * Do we need to do anything?
//    */
//    for (k = 0, do_it = 0; k < res->noise->ncov; k++) {
//    if (res->noise->ch_class[k] != MNE_COV_CH_UNKNOWN &&
//    regs[res->noise->ch_class[k]] > 0.0)
//    do_it++;
//    }
//    /*
//    * Apply regularization if necessary
//    */
//    if (do_it > 0)
//    mne_regularize_cov(res->noise,regs);
//    else
//    fprintf(stderr,"No regularization applied to the noise-covariance matrix\n");
//    }
//    /*
//    * Do the decomposition and check that the matrix is positive definite
//    */
//    fprintf(stderr,"Decomposing the noise covariance...\n");
//    if (res->noise->cov) {
//    if (mne_decompose_eigen_cov(res->noise) == FAIL)
//    goto bad;
//    fprintf(stderr,"Eigenvalue decomposition done.\n");
//    for (k = 0; k < res->noise->ncov; k++) {
//    if (res->noise->lambda[k] < 0.0)
//    res->noise->lambda[k] = 0.0;
//    }
//    }
//    else {
//    fprintf(stderr,"Decomposition not needed for a diagonal covariance matrix.\n");
//    if (mne_add_inv_cov(res->noise) == FAIL)
//    goto bad;
//    }
//    mne_free_name_list(badlist,nbad);
//    fwd_free_coil_set(templates);
//    fwd_free_coil_set(comp_coils);
//    mne_free_ctf_comp_data_set(comp_data);
    return res;


//    bad : {
//    mne_free_name_list(badlist,nbad);
//    fwd_free_coil_set(templates);
//    fwd_free_coil_set(comp_coils);
//    mne_free_ctf_comp_data_set(comp_data);
//    free_dipole_fit_data(res);
//    return NULL;
//    }
}




























//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION     "1.12"
#endif


#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif



#ifdef _POSIX_SOURCE
extern char *strdup(const char *);
#endif

#define BIG_TIME 1e6

static char  *bemname     = NULL;		 /* Boundary-element model */
static float r0[]         = { 0.0,0.0,0.04 };    /* Sphere model origin  */
static int   accurate     = FALSE;		 /* Use accurate coil definitions? */
static char  *mriname     = NULL;		 /* Gives the MRI <-> head transform */

static char  *guessname   = NULL;		 /* Initial guess grid (if not present, the values below
                          * will be employed to generate the grid) */
static char  *guess_surfname = NULL;		 /* Load the inner skull surface from this BEM file */
static float guess_mindist = 0.010;		 /* Minimum allowed distance to the surface */
static float guess_exclude = 0.020;		 /* Exclude points closer than this to the origin */
static float guess_grid    = 0.010;		 /* Grid spacing */

static char  *noisename   = NULL;		 /* Noise-covariance matrix */
static float grad_std     = 5e-13;               /* Standard deviations to be used if noise covariance is not specified */
static float mag_std      = 20e-15;
static float eeg_std      = 0.2e-6;
static int   diagnoise    = FALSE;		 /* Use only the diagonals of the noise-covariance matrix */

static char  *measname    = NULL;		 /* Data file */
static int   is_raw       = FALSE;		 /* Is this a raw data file */
static char  *badname     = NULL;		 /* Bad channels */
static int   include_meg  = FALSE;		 /* Use MEG? */
static int   include_eeg  = FALSE;		 /* Use EEG? */
static float tmin         = -2*BIG_TIME;	 /* Possibility to set these from the command line */
static float tmax         = 2*BIG_TIME;
static float tstep        = -1.0;		 /* Step between fits */
static float integ        = 0.0;
static float bmin         = BIG_TIME;	         /* Possibility to set these from the command line */
static float bmax         = BIG_TIME;
static int   do_baseline  = FALSE;	         /* Are both baseline limits set? */
static int   setno        = 1;		         /* Which data set */
static int   verbose      = FALSE;
static mneFilterDefRec filter = { TRUE,		 /* Filter on? */
                  4096,		 /* size */
                  2048,		 /* taper_size */
                  0.0, 0.0,	 /* highpass corner and width */
                  40.0, 5.0,	 /* lowpass corner and width */
                  0.0, 0.0,	 /* EOG highpass corner and width */
                  40.0, 5.0 };	 /* EOG Lowpass corner and width */
static char **projnames   = NULL;                /* Projection file names */
static int  nproj         = 0;
static int  omit_data_proj = FALSE;

static char   *eeg_model_file = NULL;            /* File of EEG sphere model specifications */
static char   *eeg_model_name = NULL;		 /* Name of the EEG model to use */
static float  eeg_sphere_rad = 0.09;		 /* Scalp radius to use in EEG sphere model */
static int    scale_eeg_pos  = FALSE;	         /* Scale the electrode locations to scalp in the sphere model */
static float  mag_reg      = 0.1;                /* Noise-covariance matrix regularization for MEG (magnetometers and axial gradiometers)  */
static int   fit_mag_dipoles = FALSE;

static float  grad_reg     = 0.1;               /* Noise-covariance matrix regularization for EEG (planar gradiometers) */
static float  eeg_reg      = 0.1;               /* Noise-covariance matrix regularization for EEG  */
static char   *dipname     = NULL;		/* Output file in dip format */
static char   *bdipname    = NULL;		/* Output file in bdip format */

static void usage(char *name)

{
  fprintf(stderr,"usage: %s [options]\n",name);
  fprintf(stderr,"This is a program for sequential single dipole fitting.\n");
  fprintf(stderr,"\nInput data:\n\n");
  fprintf(stderr,"\t--meas name       specify an evoked-response data file\n");
  fprintf(stderr,"\t--set   no        evoked data set number to use (default: 1)\n");
  fprintf(stderr,"\t--bad name        take bad channel list from here\n");

  fprintf(stderr,"\nModality selection:\n\n");
  fprintf(stderr,"\t--meg             employ MEG data in fitting\n");
  fprintf(stderr,"\t--eeg             employ EEG data in fitting\n");

  fprintf(stderr,"\nTime scale selection:\n\n");
  fprintf(stderr,"\t--tmin  time/ms   specify the starting analysis time\n");
  fprintf(stderr,"\t--tmax  time/ms   specify the ending analysis time\n");
  fprintf(stderr,"\t--tstep time/ms   specify the time step between frames (default 1/(sampling frequency))\n");
  fprintf(stderr,"\t--integ time/ms   specify the time integration for each frame (default 0)\n");

  fprintf(stderr,"\nPreprocessing:\n\n");
  fprintf(stderr,"\t--bmin  time/ms   specify the baseline starting time (evoked data only)\n");
  fprintf(stderr,"\t--bmax  time/ms   specify the baseline ending time (evoked data only)\n");
  fprintf(stderr,"\t--proj name       Load the linear projection from here\n");
  fprintf(stderr,"\t                  Multiple projections can be specified.\n");
  fprintf(stderr,"\t                  The data file will be automatically included, unless --noproj is present.\n");
  fprintf(stderr,"\t--noproj          Do not load the projection from the data file, just those given with the --proj option.\n");
  fprintf(stderr,"\n\tFiltering (raw data only):\n\n");
  fprintf(stderr,"\t--filtersize size desired filter length (default = %d)\n",filter.size);
  fprintf(stderr,"\t--highpass val/Hz highpass corner (default = %6.1f Hz)\n",filter.highpass);
  fprintf(stderr,"\t--lowpass  val/Hz lowpass  corner (default = %6.1f Hz)\n",filter.lowpass);
  fprintf(stderr,"\t--lowpassw val/Hz lowpass transition width (default = %6.1f Hz)\n",filter.lowpass_width);
  fprintf(stderr,"\t--filteroff       do not filter the data\n");

  fprintf(stderr,"\nNoise specification:\n\n");
  fprintf(stderr,"\t--noise name      take the noise-covariance matrix from here\n");
  fprintf(stderr,"\t--gradnoise val   specify a gradiometer noise value in fT/cm\n");
  fprintf(stderr,"\t--magnoise val    specify a gradiometer noise value in fT\n");
  fprintf(stderr,"\t--eegnoise val    specify an EEG value in uV\n");
  fprintf(stderr,"\t                  NOTE: The above will be used only if --noise is missing\n");
  fprintf(stderr,"\t--diagnoise       omit off-diagonal terms from the noise-covariance matrix\n");
  fprintf(stderr,"\t--reg amount      Apply regularization to the noise-covariance matrix (same fraction for all channels).\n");
  fprintf(stderr,"\t--gradreg amount  Apply regularization to the MEG noise-covariance matrix (planar gradiometers, default = %6.2f).\n",grad_reg);
  fprintf(stderr,"\t--magreg amount   Apply regularization to the EEG noise-covariance matrix (axial gradiometers and magnetometers, default = %6.2f).\n",mag_reg);
  fprintf(stderr,"\t--eegreg amount   Apply regularization to the EEG noise-covariance matrix (default = %6.2f).\n",eeg_reg);


  fprintf(stderr,"\nForward model:\n\n");
  fprintf(stderr,"\t--mri name        take head/MRI coordinate transform from here (Neuromag MRI description file)\n");
  fprintf(stderr,"\t--bem  name       BEM model name\n");
  fprintf(stderr,"\t--origin x:y:z/mm use a sphere model with this origin (head coordinates/mm)\n");
  fprintf(stderr,"\t--eegscalp        scale the electrode locations to the surface of the scalp when using a sphere model\n");
  fprintf(stderr,"\t--eegmodels name  read EEG sphere model specifications from here.\n");
  fprintf(stderr,"\t--eegmodel  name  name of the EEG sphere model to use (default : Default)\n");
  fprintf(stderr,"\t--eegrad val      radius of the scalp surface to use in EEG sphere model (default : %7.1f mm)\n",1000*eeg_sphere_rad);
  fprintf(stderr,"\t--accurate        use accurate coil definitions in MEG forward computation\n");

  fprintf(stderr,"\nFitting parameters:\n\n");
  fprintf(stderr,"\t--guess name      The source space of initial guesses.\n");
  fprintf(stderr,"\t                  If not present, the values below are used to generate the guess grid.\n");
  fprintf(stderr,"\t--gsurf   name    Read the inner skull surface from this fif file to generate the guesses.\n");
  fprintf(stderr,"\t--exclude dist/mm Exclude points which are closer than this distance from the CM of the inner skull surface (default =  %6.1f mm).\n",1000*guess_exclude);
  fprintf(stderr,"\t--mindist dist/mm Exclude points which are closer than this distance from the inner skull surface  (default = %6.1f mm).\n",1000*guess_mindist);
  fprintf(stderr,"\t--grid    dist/mm Source space grid size (default = %6.1f mm).\n",1000*guess_grid);
  fprintf(stderr,"\t--magdip          Fit magnetic dipoles instead of current dipoles.\n");
  fprintf(stderr,"\nOutput:\n\n");
  fprintf(stderr,"\t--dip     name    xfit dip format output file name\n");
  fprintf(stderr,"\t--bdip    name    xfit bdip format output file name\n");
  fprintf(stderr,"\nGeneral:\n\n");
  fprintf(stderr,"\t--help            print this info.\n");
  fprintf(stderr,"\t--version         print version info.\n\n");
  return;
}

static int check_unrecognized_args(int argc, char **argv)

{
  int k;

  if (argc > 1) {
    fprintf(stderr,"Unrecognized arguments : ");
    for (k = 1; k < argc; k++)
      fprintf(stderr,"%s ",argv[k]);
    fprintf(stderr,"\n");
    qCritical ("Check the command line.");
    return FAIL;
  }
  return OK;
}

static int check_args (int *argc,char **argv)

{
  int k;
  int p;
  int found;
  float fval;
  int   ival,filter_size;

  for (k = 0; k < *argc; k++) {
    found = 0;
    if (strcmp(argv[k],"--version") == 0) {
      mne_print_version_info(stderr,argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
      exit(0);
    }
    else if (strcmp(argv[k],"--help") == 0) {
      usage(argv[0]);
      exit(1);
    }
    else if (strcmp(argv[k],"--guess") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--guess: argument required.");
    return FAIL;
      }
      guessname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--gsurf") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--gsurf: argument required.");
    return FAIL;
      }
      guess_surfname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--mindist") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--mindist: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%f",&fval) != 1) {
    qCritical ("Could not interpret the distance.");
    return FAIL;
      }
      guess_mindist = fval/1000.0;
      if (guess_mindist <= 0.0)
    guess_mindist = 0.0;
    }
    else if (strcmp(argv[k],"--exclude") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--exclude: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%f",&fval) != 1) {
    qCritical ("Could not interpret the distance.");
    return FAIL;
      }
      guess_exclude = fval/1000.0;
      if (guess_exclude <= 0.0)
    guess_exclude = 0.0;
    }
    else if (strcmp(argv[k],"--grid") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--grid: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%f",&fval) != 1) {
    qCritical ("Could not interpret the distance.");
    return FAIL;
      }
      if (fval <= 0.0) {
    qCritical ("Grid spacing should be positive");
    return FAIL;
      }
      guess_grid = guess_grid/1000.0;
    }
    else if (strcmp(argv[k],"--mri") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--mri: argument required.");
    return FAIL;
      }
      mriname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--bem") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bem: argument required.");
    return FAIL;
      }
      bemname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--accurate") == 0) {
      found = 1;
      accurate = TRUE;
    }
    else if (strcmp(argv[k],"--meg") == 0) {
      found = 1;
      include_meg = TRUE;
    }
    else if (strcmp(argv[k],"--eeg") == 0) {
      found = 1;
      include_eeg = TRUE;
    }
    else if (strcmp(argv[k],"--origin") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--origin: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%f:%f:%f",r0+X,r0+Y,r0+Z) != 3) {
    qCritical ("Could not interpret the origin.");
    return FAIL;
      }
      r0[X] = r0[X]/1000.0;
      r0[Y] = r0[Y]/1000.0;
      r0[Z] = r0[Z]/1000.0;
    }
    else if (strcmp(argv[k],"--eegrad") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegrad: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&eeg_sphere_rad) != 1) {
    qCritical () << "Incomprehensible radius:" << argv[k+1];
    return FAIL;
      }
      if (eeg_sphere_rad <= 0) {
    qCritical ("Radius must be positive");
    return FAIL;
      }
      eeg_sphere_rad = eeg_sphere_rad/1000.0;
    }
    else if (strcmp(argv[k],"--eegmodels") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegmodels: argument required.");
    return FAIL;
      }
      eeg_model_file = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--eegmodel") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegmodel: argument required.");
    return FAIL;
      }
      eeg_model_name = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--eegscalp") == 0) {
      found         = 1;
      scale_eeg_pos = TRUE;
    }
    else if (strcmp(argv[k],"--meas") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--meas: argument required.");
    return FAIL;
      }
      measname = strdup(argv[k+1]);
      is_raw = FALSE;
    }
    else if (strcmp(argv[k],"--raw") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--raw: argument required.");
    return FAIL;
      }
      measname = strdup(argv[k+1]);
      is_raw = TRUE;
    }
    else if (strcmp(argv[k],"--proj") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--proj: argument required.");
    return FAIL;
      }
      projnames = REALLOC(projnames,nproj+1,char *);
      projnames[nproj++] = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--noproj") == 0) {
      found = 1;
      omit_data_proj = TRUE;
    }
    else if (strcmp(argv[k],"--bad") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bad: argument required.");
    return FAIL;
      }
      badname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--noise") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--noise: argument required.");
    return FAIL;
      }
      noisename = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--gradnoise") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--gradnoise: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0.0) {
    qCritical ("Value should be positive");
    return FAIL;
      }
      grad_std = 1e-13*fval;
    }
    else if (strcmp(argv[k],"--magnoise") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--magnoise: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0.0) {
    qCritical ("Value should be positive");
    return FAIL;
      }
      mag_std = 1e-15*fval;
    }
    else if (strcmp(argv[k],"--eegnoise") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegnoise: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical () << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0.0) {
    qCritical ("Value should be positive");
    return FAIL;
      }
      eeg_std = 1e-6*fval;
    }
    else if (strcmp(argv[k],"--diagnoise") == 0) {
      found = 1;
      diagnoise = TRUE;
    }
    else if (strcmp(argv[k],"--eegreg") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegreg: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical () << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0 || fval > 1) {
    qCritical ("Regularization value should be positive and smaller than one.");
    return FAIL;
      }
      eeg_reg = fval;
    }
    else if (strcmp(argv[k],"--magreg") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--magreg: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical () << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0 || fval > 1) {
    qCritical ("Regularization value should be positive and smaller than one.");
    return FAIL;
      }
      mag_reg = fval;
    }
    else if (strcmp(argv[k],"--gradreg") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--gradreg: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical () << "Incomprehensible value: %s",argv[k+1]);
    return FAIL;
      }
      if (fval < 0 || fval > 1) {
    qCritical ("Regularization value should be positive and smaller than one.");
    return FAIL;
      }
      grad_reg = fval;
    }
    else if (strcmp(argv[k],"--reg") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--reg: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical () << "Incomprehensible value: %s",argv[k+1]);
    return FAIL;
      }
      if (fval < 0 || fval > 1) {
    qCritical ("Regularization value should be positive and smaller than one.");
    return FAIL;
      }
      grad_reg = fval;
      mag_reg = fval;
      eeg_reg = fval;
    }
    else if (strcmp(argv[k],"--tstep") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--tstep: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible tstep:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0.0) {
    qCritical ("Time step should be positive");
    return FAIL;
      }
      tstep = fval/1000.0;
    }
    else if (strcmp(argv[k],"--integ") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--integ: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible integration time:" << argv[k+1];
    return FAIL;
      }
      if (fval <= 0.0) {
    qCritical ("Integration time should be positive.");
    return FAIL;
      }
      integ = fval/1000.0;
    }
    else if (strcmp(argv[k],"--tmin") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--tmin: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible tmin:" << argv[k+1];
    return FAIL;
      }
      tmin = fval/1000.0;
    }
    else if (strcmp(argv[k],"--tmax") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--tmax: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible tmax:" << argv[k+1];
    return FAIL;
      }
      tmax = fval/1000.0;
    }
    else if (strcmp(argv[k],"--bmin") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bmin: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible bmin:" << argv[k+1];
    return FAIL;
      }
      bmin = fval/1000.0;
    }
    else if (strcmp(argv[k],"--bmax") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bmax: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible bmax:" << argv[k+1];
    return FAIL;
      }
      bmax = fval/1000.0;
    }
    else if (strcmp(argv[k],"--set") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--set: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%d",&setno) != 1) {
    qCritical() << "Incomprehensible data set number:" << argv[k+1];
    return FAIL;
      }
      if (setno <= 0) {
    qCritical ("Data set number must be > 0");
    return FAIL;
      }
    }
    else if (strcmp(argv[k],"--filteroff") == 0) {
      found = 1;
      filter.filter_on = FALSE;
    }
    else if (strcmp(argv[k],"--lowpass") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--lowpass: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Illegal number:" << argv[k+1];
    return FAIL;
      }
      if (fval <= 0) {
    qCritical ("Lowpass corner must be positive");
    return FAIL;
      }
      filter.lowpass = fval;
    }
    else if (strcmp(argv[k],"--lowpassw") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--lowpassw: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Illegal number:" << argv[k+1];
    return FAIL;
      }
      if (fval <= 0) {
    qCritical ("Lowpass width must be positive");
    return FAIL;
      }
      filter.lowpass_width = fval;
    }
    else if (strcmp(argv[k],"--highpass") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--highpass: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Illegal number:" << argv[k+1];
    return FAIL;
      }
        if (fval <= 0) {
            qCritical ("Highpass corner must be positive");
            return FAIL;
        }
        filter.highpass = fval;
    }
    else if (strcmp(argv[k],"--filtersize") == 0) {
        found = 2;
    if (k == *argc - 1) {
        qCritical ("--filtersize: argument required.");
        return FAIL;
    }
    if (sscanf(argv[k+1],"%d",&ival) != 1) {
        qCritical() << "Illegal number:" << argv[k+1];
        return FAIL;
    }
    if (ival < 1024) {
        qCritical ("Filtersize should be at least 1024.");
        return FAIL;
    }
    for (filter_size = 1024; filter_size < ival; filter_size = 2*filter_size)
    ;
        filter.size       = filter_size;
        filter.taper_size = filter_size/2;
    }
    else if (strcmp(argv[k],"--magdip") == 0) {
        found = 1;
        fit_mag_dipoles = TRUE;
    }
    else if (strcmp(argv[k],"--dip") == 0) {
        found = 2;
        if (k == *argc - 1) {
            qCritical ("--dip: argument required.");
            return FAIL;
    }
    dipname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--bdip") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bdip: argument required.");
    return FAIL;
      }
      bdipname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--verbose") == 0) {
      found = 1;
      verbose = TRUE;
    }
    if (found) {
      for (p = k; p < *argc-found; p++)
    argv[p] = argv[p+found];
      *argc = *argc - found;
      k = k - found;
    }
  }
  return check_unrecognized_args(*argc,argv);
}

void print_cov(FILE *out, mneCovMatrix cov)
{
    int k;
    mne_print_dvector(out,NULL,cov->lambda,cov->ncov);
    if (cov->eigen) {
        for (k = 0; k < cov->ncov; k++)
            mne_print_vector(out,NULL,cov->eigen[k],cov->ncov);
    }
}












//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int            res      = FAIL;
    dipoleFitData  fit_data = NULL;
    mneMeasData    data     = NULL;
    mneRawData     raw      = NULL;
    guessData      guess    = NULL;
    mneChSelection sel      = NULL;
    ecdSet         set      = NULL;

    fwdEegSphereModel    eeg_model = NULL;
    int                  k;

    if (check_args(&argc,argv) == FAIL)
        goto out;

    do_baseline = (bmin < BIG_TIME && bmax < BIG_TIME);

    if (!measname) {
        qCritical ("Data file name missing. Please specify one using the --meas option.");
        goto out;
    }
    if (!dipname && !bdipname) {
        qCritical ("Output file name missing. Please use the --dip or --bdip options to do this.");
        goto out;
    }
    if (!guessname) {
        if (!bemname && guess_surfname && !mriname) {
            qCritical ("Please specify the MRI/head coordinate transformation with the --mri option");
            goto out;
        }
    }
    if (!include_meg && !include_eeg) {
        qCritical ("Specify one or both of the --eeg and --meg options");
        goto out;
    }
    if (!omit_data_proj) {
        projnames = REALLOC(projnames,nproj+1,char *);
        nproj++;
        for (k = 1; k < nproj; k++)
            projnames[k] = projnames[k-1];
        projnames[0] = mne_strdup(measname);
    }
    fprintf(stderr,"\n");
    mne_print_version_info(stderr,argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
    fprintf(stderr,"\n");
    if (bemname)
        fprintf(stderr,"BEM              : %s\n",bemname);
    else {
        fprintf(stderr,"Sphere model     : origin at (% 7.2f % 7.2f % 7.2f) mm\n",
        1000*r0[X],1000*r0[Y],1000*r0[Z]);
    }
    fprintf(stderr,"Using %s MEG coil definitions.\n",accurate ? "accurate" : "standard");
    if (mriname)
        fprintf(stderr,"MRI transform    : %s\n",mriname);
    if (guessname)
        fprintf(stderr,"Guesses          : %s\n",guessname);
    else {
        fprintf(stderr,"Guess grid       : %6.1f mm\n",1000*guess_grid);
        if (guess_mindist > 0.0)
            fprintf(stderr,"Guess mindist    : %6.1f mm\n",1000*guess_mindist);
        if (guess_exclude > 0)
            fprintf(stderr,"Guess exclude    : %6.1f mm\n",1000*guess_exclude);
    }
    fprintf(stderr,"Data             : %s\n",measname);
    if (nproj > 0) {
        fprintf(stderr,"SSP sources      :\n");
    for (k = 0; k < nproj; k++)
        fprintf(stderr,"\t%s\n",projnames[k]);
    }
    if (badname)
        fprintf(stderr,"Bad channels     : %s\n",badname);
    if (do_baseline)
        fprintf(stderr,"Baseline         : %10.2f ... %10.2f ms\n",
    1000*bmin,1000*bmax);
    if (noisename) {
        fprintf(stderr,"Noise covariance : %s\n",noisename);
        if (include_meg) {
            if (mag_reg > 0.0)
                fprintf(stderr,"\tNoise-covariange regularization (mag)     : %-5.2f\n",mag_reg);
            if (grad_reg > 0.0)
                fprintf(stderr,"\tNoise-covariange regularization (grad)    : %-5.2f\n",grad_reg);
        }
        if (include_eeg && eeg_reg > 0.0)
            fprintf(stderr,"\tNoise-covariange regularization (EEG)     : %-5.2f\n",eeg_reg);
    }
    if (fit_mag_dipoles)
        fprintf(stderr,"Fit data with magnetic dipoles\n");
    if (dipname)
        fprintf(stderr,"dip output      : %s\n",dipname);
    if (bdipname)
        fprintf(stderr,"bdip output     : %s\n",bdipname);
    fprintf(stderr,"\n");
    fprintf(stderr,"---- Setting up...\n\n");

    if (include_eeg) {
        if ((eeg_model = setup_eeg_sphere_model(eeg_model_file,eeg_model_name,eeg_sphere_rad)) == NULL)
            goto out;
    }
    if ((fit_data = setup_dipole_fit_data(mriname,measname,bemname,r0,eeg_model,accurate,
        badname,noisename,grad_std,mag_std,eeg_std,
        mag_reg,grad_reg,eeg_reg,
        diagnoise,projnames,nproj,include_meg,include_eeg)) == NULL)
        goto out;
    fit_data->fit_mag_dipoles = fit_mag_dipoles;
    if (is_raw) {
        int c;
        float t1,t2;

        fprintf(stderr,"\n---- Opening a raw data file...\n\n");
        if ((raw = mne_raw_open_file(measname,TRUE,FALSE,&filter)) == NULL)
            goto out;
        /*
        * A channel selection is needed to access the data
        */
        sel = mne_ch_selection_these("fit",fit_data->ch_names,fit_data->nmeg+fit_data->neeg);
        mne_ch_selection_assign_chs(sel,raw);
        for (c = 0; c < sel->nchan; c++)
            if (sel->pick[c] < 0) {
                qCritical ("All desired channels were not available");
                goto out;
            }
            fprintf(stderr,"\tChannel selection created.\n");
            /*
            * Let's be a little generous here
            */
            t1 = raw->first_samp/raw->info->sfreq;
            t2 = (raw->first_samp+raw->nsamp-1)/raw->info->sfreq;
            if (tmin < t1 + integ)
            tmin = t1 + integ;
            if (tmax > t2 - integ)
            tmax =  t2 - integ;
            if (tstep < 0)
            tstep = 1.0/raw->info->sfreq;

            fprintf(stderr,"\tOpened raw data file %s : %d MEG and %d EEG \n",
            measname,fit_data->nmeg,fit_data->neeg);
        }
    else {
        fprintf(stderr,"\n---- Reading data...\n\n");
        if ((data = mne_read_meas_data(measname,setno,NULL,NULL,
            fit_data->ch_names,fit_data->nmeg+fit_data->neeg)) == NULL)
            goto out;
        if (do_baseline)
            mne_adjust_baselines(data,bmin,bmax);
        else
            fprintf(stderr,"\tNo baseline setting in effect.\n");
        if (tmin < data->current->tmin + integ/2.0)
            tmin = data->current->tmin + integ/2.0;
        if (tmax > data->current->tmin + (data->current->np-1)*data->current->tstep - integ/2.0)
            tmax =  data->current->tmin + (data->current->np-1)*data->current->tstep - integ/2.0;
        if (tstep < 0)
            tstep = data->current->tstep;

        fprintf(stderr,"\tRead data set %d from %s : %d MEG and %d EEG \n",
        setno,measname,fit_data->nmeg,fit_data->neeg);
        if (noisename) {
            fprintf(stderr,"\nScaling the noise covariance...\n");
            if (scale_noise_cov(fit_data,data->current->nave) == FAIL)
                goto out;
        }
    }
    /*
    * Proceed to computing the fits
    */
    fprintf(stderr,"\n---- Computing the forward solution for the guesses...\n\n");
    if ((guess = make_guess_data(guessname,
    guess_surfname,
    guess_mindist,guess_exclude,guess_grid,fit_data)) == NULL)
    goto out;
    fprintf (stderr,"\n---- Fitting : %7.1f ... %7.1f ms (step: %6.1f ms integ: %6.1f ms)\n\n",
    1000*tmin,1000*tmax,1000*tstep,1000*integ);
    if (raw) {
        if (fit_dipoles_raw(measname,raw,sel,fit_data,guess,tmin,tmax,tstep,integ,verbose,NULL) == FAIL)
            goto out;
    }
    else {
        if (fit_dipoles(measname,data,fit_data,guess,tmin,tmax,tstep,integ,verbose,&set) == FAIL)
            goto out;
    }
    printf("%d dipoles fitted\n",set->ndip);
    /*
    * Saving...
    */
    if (save_dipoles_dip(dipname,set) == FAIL)
    goto out;
    if (save_dipoles_bdip(bdipname,set) == FAIL)
    goto out;
    free_ecd_set(set);
    res = OK;

    out : {
        if (res == FAIL) {
//            err_print_error();
            exit(1);
        }
        else
            exit (0);
    }


//    return app.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
