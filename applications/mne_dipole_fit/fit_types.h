#ifndef _fit_types_h
#define _fit_types_h

#include "fwd_types.h"

/*
 * These are the type definitions for dipole fitting
 */
typedef void (*fitUserFreeFunc)(void *);

typedef struct {
  float **rd;			    /* Dipole locations */
  int   ndip;			    /* How many dipoles */
  float **fwd;			    /* The forward solution (projected and whitened) */
  float *scales;		    /* Scales applied to the columns of fwd */
  float **uu;			    /* The left singular vectors of the forward matrix */
  float **vv;			    /* The right singular vectors of the forward matrix */
  float *sing;			    /* The singular values */
  int   nch;			    /* Number of channels */
} *dipoleForward,dipoleForwardRec;

typedef struct {
  float          **rr;		    /* These are the guess dipole locations */
  dipoleForward  *guess_fwd;	    /* Forward solutions for the guesses */
  int            nguess;	    /* How many sources */
} *guessData,guessDataRec;

typedef struct {
  fwdFieldFunc    meg_field;	    /* MEG forward calculation functions */
  fwdVecFieldFunc meg_vec_field;
  void            *meg_client;	    /* Client data for MEG field computations */
  mneUserFreeFunc meg_client_free;

  fwdFieldFunc    eeg_pot;	    /* EEG forward calculation functions */
  fwdVecFieldFunc eeg_vec_pot;
  void            *eeg_client;	    /* Client data for EEG field computations */
  mneUserFreeFunc eeg_client_free;
} *dipoleFitFuncs,dipoleFitFuncsRec;

#define COLUMN_NORM_NONE 0	    /* No column normalization requested */
#define COLUMN_NORM_COMP 1	    /* Componentwise normalization */
#define COLUMN_NORM_LOC  2	    /* Dipole locationwise normalization */

typedef struct {		      /* This structure holds all fitting-related data */
  fiffCoordTrans    mri_head_t;	      /* MRI <-> head coordinate transformation */
  fiffCoordTrans    meg_head_t;	      /* MEG <-> head coordinate transformation */
  int               coord_frame;      /* Common coordinate frame */
  fiffChInfo        chs;              /* Channels */
  int               nmeg;	      /* How many MEG */
  int               neeg;	      /* How many EEG */
  char              **ch_names;	      /* List of all channel names */
  mneSparseMatrix   pick;	      /* Matrix to pick data from the
                     full data set which may contain channels
                     we are not interested in */
  fwdCoilSet        meg_coils;	      /* MEG coil definitions */
  fwdCoilSet        eeg_els;	      /* EEG electrode definitions */
  float             r0[3];	      /* Sphere model origin */
  char              *bemname;	      /* Using a BEM? */

  fwdEegSphereModel eeg_model;	      /* EEG sphere model definition */
  fwdBemModel       bem_model;	      /* BEM model definition */

  dipoleFitFuncs    sphere_funcs;     /* These are the sphere model forward functions */
  dipoleFitFuncs    bem_funcs;	      /* These are the BEM forward functions */
  dipoleFitFuncs    funcs;	      /* Points to one of the two above */
  dipoleFitFuncs    mag_dipole_funcs; /* Functions to fit a magnetic dipole */

  int               fixed_noise;      /* Were fixed noise values used rather than a noise-covariance
                       * matrix read from a file */
  mneCovMatrix      noise_orig;	      /* Noise covariance matrix (original) */
  mneCovMatrix      noise;	      /* Noise covariance matrix (weighted to take the selection into account) */
  int               nave;	      /* How many averages does this correspond to? */
  mneProjOp         proj;	      /* The projection operator to use */
  int               column_norm;      /* What kind of column normalization to apply to the forward solution */
  int               fit_mag_dipoles;  /* Fit magnetic dipoles? */
  void              *user;	      /* User data for anything we need */
  fitUserFreeFunc   user_free;	      /* Function to free the above */
} *dipoleFitData,dipoleFitDataRec;

//typedef struct {
//  int   valid;			/* Is this dipole valid */
//  float time;			/* Fitting time */
//  float begin;			/* Begin time if this is a time span */
//  float end;			/* End time if this is a time span */
//  float rd[3];			/* Dipole location */
//  float Q[3];			/* Dipole moment */
//  float good;			/* Goodness of fit */
//  float khi2;			/* khi^2 value */
//  int   nfree;			/* Degrees of freedom for the above */
//  int   neval;			/* Number of function evaluations required for this fit */
//  float color[4];		/* Paint with this color */
//} *ECD,ECDRec;			/* One ECD */




typedef struct {
  int   valid;			/* Is this dipole valid */
  float time;			/* Time point */
  float rd[3];			/* Dipole location */
  float Q[3];			/* Dipole moment */
  float good;			/* Goodness of fit */
  float khi2;			/* khi^2 value */
  int   nfree;			/* Degrees of freedom for the above */
  int   neval;			/* Number of function evaluations required for this fit */
} *ecd,ecdRec;			/* One ECD */

typedef struct {
  char *dataname;		/* The associated data file */
  int  ndip;			/* How many dipoles */
  ecd  *dips;			/* The dipoles themselves */
} *ecdSet,ecdSetRec;		/* A set of ECDs */


#define DIPOLE_MODEL_UNKNOWN    -1
#define DIPOLE_MODEL_SINGLE      1
#define DIPOLE_MODEL_SEQUENTIAL  2   /* AKA moving dipole fit */
#define DIPOLE_MODEL_COLLECTION  3   /* Collection of dipoles transferred via drag and drop */
#define DIPOLE_MODEL_MULTI       4   /* Multiple dipoles, single time point */
#define DIPOLE_MODEL_MULTI_RANGE 5   /* Multiple dipoles, time range */

///*
// * These are used for bookkeeping of dipole models
// */
//typedef struct {
//  ECD             *ECDs;	     /* The current dipoles */
//  int             ndip;		     /* How many? */
//  int             model_kind;	     /* One of the above */
//  void            *pars;             /* Opaque parameter data */
//  mneUserFreeFunc pars_free;         /* Frees the above */
//} *dipoleModel,dipoleModelRec;

//typedef void (*dipoleFunc)(Widget,dipoleModel);


#endif

