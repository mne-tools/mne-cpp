#ifndef _fwd_fwd_types_h
#define _fwd_fwd_types_h

/*
 * Type definitions for forward calculations
 */

#include <fiff/fiff_types.h>
#include "mne_types.h"

#include "fwd_coil_set.h"

using namespace FIFFLIB;


#if defined(__cplusplus) 
extern "C" {
#endif

//#define FWD_COIL_UNKNOWN      0

//#define FWD_COILC_UNKNOWN     0
//#define FWD_COILC_EEG         1000
//#define FWD_COILC_MAG         1
//#define FWD_COILC_AXIAL_GRAD  2
//#define FWD_COILC_PLANAR_GRAD 3
//#define FWD_COILC_AXIAL_GRAD2 4

//#define FWD_COIL_ACCURACY_POINT    0
//#define FWD_COIL_ACCURACY_NORMAL   1
//#define FWD_COIL_ACCURACY_ACCURATE 2

//#define FWD_IS_MEG_COIL(x) ((x) != FWD_COILC_EEG && (x) != FWD_COILC_UNKNOWN)

typedef void (*fwdUserFreeFunc)(void *);  /* General purpose */

//typedef struct {
//  char         *chname;		/* Name of this channel */
//  int          coord_frame;	/* Which coordinate frame are we in? */
//  char         *desc;	        /* Description for this type of a coil */
//  int          coil_class;	/* Coil class */
//  int          type;		/* Coil type */
//  int          accuracy;	/* Accuracy */
//  float        size;		/* Coil size */
//  float        base;		/* Baseline */
//  float        r0[3];		/* Coil coordinate system origin */
//  float        ex[3];		/* Coil coordinate system unit vectors */
//  float        ey[3];		/* This stupid construction needs to be replaced with */
//  float        ez[3];		/* a coordinate transformation */
//  int          np;		/* Number of integration points */
//  float        **rmag;		/* The field point locations */
//  float        **cosmag;	/* The corresponding direction cosines */
//  float        *w;		/* The weighting coefficients */
//} *fwdCoil,fwdCoilRec;


//typedef struct {
//  fwdCoil *coils;		/* The coil or electrode positions */
//  int     ncoil;
//  int     coord_frame;		/* Common coordinate frame */
//  void    *user_data;		/* We can put whatever in here */
//  fwdUserFreeFunc user_data_free;
//} *fwdCoilSet,fwdCoilSetRec;	/* A collection of the above */

/*
 * This is a convenient generic field / potential computation function
 */
typedef int (*fwdFieldFunc)(float *rd,float *Q,INVERSELIB::FwdCoilSet* coils,float *res,void *client);
typedef int (*fwdVecFieldFunc)(float *rd,INVERSELIB::FwdCoilSet* coils,float **res,void *client);
typedef int (*fwdFieldGradFunc)(float *rd,float *Q,INVERSELIB::FwdCoilSet* coils, float *res,
				float *xgrad, float *ygrad, float *zgrad, void *client);


/*
 * This is used in the function which evaluates integrals over triangles
 */

typedef struct {
  double **xyz; 	         /* The coordinates */
  double *w;			 /* Weights */
  int    np;			 /* How many */
} *fwdIntPoints,fwdIntPointsRec; /* Integration points in a triangle */

typedef double (*fwdIntApproxEvalFunc)(fwdIntPoints int_points, double *r0, void *user);

/*
 * Definitions for the EEG sphere model
 */

//typedef struct {
//  float rad;			/* The actual rads */
//  float rel_rad;		/* Relative rads */
//  float sigma;			/* Conductivity */
//} *fwdEegSphereLayer,fwdEegSphereLayerRec;


//typedef struct {
//  char  *name;                              /* Textual identifier */
//  int   nlayer;                             /* Number of layers */
//  INVERSELIB::FwdEegSphereLayer* layers;    /* An array of layers */
//  float  r0[3];                             /* The origin */

//  double *fn;                               /* Coefficients saved to speed up the computations */
//  int    nterms;                            /* How many? */

//  float  *mu;                               /* The Berg-Scherg equivalence parameters */
//  float  *lambda;
//  int    nfit;                              /* How many? */
//  int    scale_pos;                         /* Scale the positions to the surface of the sphere? */
//} *fwdEegSphereModel,fwdEegSphereModelRec;

//typedef struct {
//  INVERSELIB::FwdEegSphereModel** models;	/* Set of EEG sphere model definitions */
//  int               nmodel;
//} *fwdEegSphereModelSet,fwdEegSphereModelSetRec;

#define FWD_BEM_UNKNOWN           -1
#define FWD_BEM_CONSTANT_COLL     1
#define FWD_BEM_LINEAR_COLL       2

#define FWD_BEM_IP_APPROACH_LIMIT 0.1

#define FWD_BEM_LIN_FIELD_SIMPLE    1
#define FWD_BEM_LIN_FIELD_FERGUSON  2
#define FWD_BEM_LIN_FIELD_URANKAR   3

typedef struct {		      /* Space to store a solution matrix */
  float **solution;		      /* The solution matrix */
  int   ncoil;			      /* Number of sensors */
  int   np;		              /* Number of potential solution points */
} *fwdBemSolution,fwdBemSolutionRec;  /* Mapping from infinite medium potentials to a particular set of coils or electrodes */

typedef struct {
  char       *surf_name;	/* Name of the file where surfaces were loaded from */
  mneSurface *surfs;		/* The interface surfaces from outside towards inside */
  int        *ntri;		/* Number of triangles on each surface */
  int        *np;		/* Number of vertices on each surface */
  int        nsurf;		/* How many */
  float      *sigma;		/* The conductivities */
  float      **gamma;		/* The gamma factors */
  float      *source_mult;	/* These multiply the infinite medium potentials */
  float      *field_mult;	/* Multipliers for the magnetic field */
  int        bem_method;	/* Which approximation method is used */
  char       *sol_name;		/* Name of the file where the solution was loaded from */

  float      **solution;	/* The potential solution matrix */
  float      *v0;		/* Space for the infinite-medium potentials */
  int        nsol;		/* Size of the solution matrix */

  fiffCoordTrans head_mri_t;	/* Coordinate transformation from head to MRI coordinates */

  float      ip_approach_limit;	/* Controls whether we need to use the isolated problem approach */
  int        use_ip_approach;	/* Do we need it */
} *fwdBemModel,fwdBemModelRec;	/* Holds the BEM model definition */

typedef struct {
  mneCTFcompDataSet set;	         /* The compensation data set */
  INVERSELIB::FwdCoilSet*        comp_coils;	         /* The compensation coil definitions */
  fwdFieldFunc      field;	         /* Computes the field of given direction dipole */
  fwdVecFieldFunc   vec_field;	         /* Computes the fields of all three dipole components  */
  fwdFieldGradFunc  field_grad;	         /* Computes the field and gradient of one dipole direction */
  void              *client;	         /* Client data to pass to the above functions */
  fwdUserFreeFunc   client_free;
  float             *work;	         /* The work areas */
  float             **vec_work;
} *fwdCompData,fwdCompDataRec;	         /* This structure is used in the compensated field calculations */


#if defined(__cplusplus) 
}
#endif
#endif
