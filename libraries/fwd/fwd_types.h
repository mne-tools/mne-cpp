#ifndef _fwd_fwd_types_h
#define _fwd_fwd_types_h

/*
 * Type definitions for forward calculations
 */

#include <fiff/fiff_types.h>
#include <mne/c/mne_types.h>

#include "fwd_coil_set.h"

#include <mne/c/mne_ctf_comp_data_set.h>

typedef void (*fwdUserFreeFunc)(void *);  /* General purpose */

/*
 * This is a convenient generic field / potential computation function
 */
typedef int (*fwdFieldFunc)(float *rd,float *Q,FWDLIB::FwdCoilSet* coils,float *res,void *client);
typedef int (*fwdVecFieldFunc)(float *rd,FWDLIB::FwdCoilSet* coils,float **res,void *client);
typedef int (*fwdFieldGradFunc)(float *rd,float *Q,FWDLIB::FwdCoilSet* coils, float *res,
                                float *xgrad, float *ygrad, float *zgrad, void *client);

//#define FWD_BEM_UNKNOWN           -1
//#define FWD_BEM_CONSTANT_COLL     1
//#define FWD_BEM_LINEAR_COLL       2

//#define FWD_BEM_IP_APPROACH_LIMIT 0.1

//#define FWD_BEM_LIN_FIELD_SIMPLE    1
//#define FWD_BEM_LIN_FIELD_FERGUSON  2
//#define FWD_BEM_LIN_FIELD_URANKAR   3

//typedef struct {		      /* Space to store a solution matrix */
//  float **solution;		      /* The solution matrix */
//  int   ncoil;			      /* Number of sensors */
//  int   np;		              /* Number of potential solution points */
//} *fwdBemSolution,fwdBemSolutionRec;  /* Mapping from infinite medium potentials to a particular set of coils or electrodes */

//typedef struct {
//  char       *surf_name;	/* Name of the file where surfaces were loaded from */
//  FWDLIB::MneCSurface* *surfs;   /* The interface surfaces from outside towards inside */
//  int        *ntri;		/* Number of triangles on each surface */
//  int        *np;		/* Number of vertices on each surface */
//  int        nsurf;		/* How many */
//  float      *sigma;		/* The conductivities */
//  float      **gamma;		/* The gamma factors */
//  float      *source_mult;	/* These multiply the infinite medium potentials */
//  float      *field_mult;	/* Multipliers for the magnetic field */
//  int        bem_method;	/* Which approximation method is used */
//  char       *sol_name;		/* Name of the file where the solution was loaded from */

//  float      **solution;	/* The potential solution matrix */
//  float      *v0;		/* Space for the infinite-medium potentials */
//  int        nsol;		/* Size of the solution matrix */

//  FWDLIB::FiffCoordTransOld* head_mri_t;	/* Coordinate transformation from head to MRI coordinates */

//  float      ip_approach_limit;	/* Controls whether we need to use the isolated problem approach */
//  int        use_ip_approach;	/* Do we need it */
//} *fwdBemModel,fwdBemModelRec;	/* Holds the BEM model definition */

//typedef struct {
//  FWDLIB::MneCTFCompDataSet* set;	         /* The compensation data set */
//  FWDLIB::FwdCoilSet*        comp_coils;	         /* The compensation coil definitions */
//  fwdFieldFunc      field;	         /* Computes the field of given direction dipole */
//  fwdVecFieldFunc   vec_field;	         /* Computes the fields of all three dipole components  */
//  fwdFieldGradFunc  field_grad;	         /* Computes the field and gradient of one dipole direction */
//  void              *client;	         /* Client data to pass to the above functions */
//  fwdUserFreeFunc   client_free;
//  float             *work;	         /* The work areas */
//  float             **vec_work;
//} *fwdCompData,fwdCompDataRec;	         /* This structure is used in the compensated field calculations */

#endif
