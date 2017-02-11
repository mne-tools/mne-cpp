//=============================================================================================================
/**
* @file     fwd_bem_model.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    FwdBemModel class declaration.
*
*/

#ifndef FWDBEMMODEL_H
#define FWDBEMMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "mne_surface_or_volume.h"
#include "fiff_coord_trans_old.h"
#include "fwd_coil_set.h"

#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_tag.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>



#define FWD_BEM_UNKNOWN           -1
#define FWD_BEM_CONSTANT_COLL     1
#define FWD_BEM_LINEAR_COLL       2

#define FWD_BEM_IP_APPROACH_LIMIT 0.1

#define FWD_BEM_LIN_FIELD_SIMPLE    1
#define FWD_BEM_LIN_FIELD_FERGUSON  2
#define FWD_BEM_LIN_FIELD_URANKAR   3



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{


//=============================================================================================================
/**
* Implements FwdBemModel (Replaces *fwdBemModel,fwdBemModelRec struct of MNE-C fwd_types.h).
*
* @brief Holds the BEM model definition
*/
class INVERSESHARED_EXPORT FwdBemModel
{
public:
    typedef QSharedPointer<FwdBemModel> SPtr;              /**< Shared pointer type for FwdBemModel. */
    typedef QSharedPointer<const FwdBemModel> ConstSPtr;   /**< Const shared pointer type for FwdBemModel. */

    //=========================================================================================================
    /**
    * Constructs the BEM Model
    * Refactored: fwd_bem_new_model (fwd_bem_model.c)
    *
    */
    explicit FwdBemModel();

    //=========================================================================================================
    /**
    * Destroys the BEM Model
    * Refactored: fwd_bem_free_solution (fwd_bem_model.c)
    */
    virtual ~FwdBemModel();




    //============================= fwd_bem_model.c =============================

    static const char *fwd_bem_explain_surface(int kind);

    static const char *fwd_bem_explain_method(int method);




    static int get_int( FIFFLIB::FiffStream::SPtr& stream, const FIFFLIB::FiffDirNode::SPtr& node,int what,int *res);






    /*
    * Return a pointer to a specific surface in a BEM
    */
    // Refactored: fwd_bem_find_surface (fwd_bem_model.c)
    MneSurfaceOrVolume::MneCSurface* fwd_bem_find_surface(int kind);


    static FwdBemModel* fwd_bem_load_surfaces(const QString& name,
                                      int  *kinds,
                                      int  nkind);


    static FwdBemModel* fwd_bem_load_homog_surface(const QString& name);

    static FwdBemModel* fwd_bem_load_three_layer_surfaces(const QString& name);



    static int fwd_bem_load_solution(const QString& name, int bem_method, FwdBemModel* m);


    static int fwd_bem_set_head_mri_t(FwdBemModel* m, FiffCoordTransOld* t);




    //============================= fwd_bem_linear_collocation.c =============================

    /*
     * The following approach is based on:
     *
     * de Munck JC: "A linear discretization of the volume conductor boundary integral equation using analytically integrated elements",
     * IEEE Trans Biomed Eng. 1992 39(9) : 986 - 990
     *
     */

    static double calc_beta (double *rk,double *rk1);

    static void lin_pot_coeff (float  *from,	/* Origin */
                               mneTriangle to,	/* The destination triangle */
                               double omega[3]);

    static void correct_auto_elements (MneSurfaceOrVolume::MneCSurface* surf,
                                       float      **mat);

    static float **fwd_bem_lin_pot_coeff (MneSurfaceOrVolume::MneCSurface* *surfs,int nsurf);

    static int fwd_bem_linear_collocation_solution(FwdBemModel* m);



    //============================= fwd_bem_solution.c =============================

    static float **fwd_bem_multi_solution (float **solids,    /* The solid-angle matrix */
                                    float **gamma,     /* The conductivity multipliers */
                                    int   nsurf,       /* Number of surfaces */
                                    int   *ntri);

    static float **fwd_bem_homog_solution (float **solids,int ntri);


    static void fwd_bem_ip_modify_solution(float **solution,    /* The original solution */
                                    float **ip_solution,        /* The isolated problem solution */
                                    float ip_mult,              /* Conductivity ratio */
                                    int nsurf,                  /* Number of surfaces */
                                    int *ntri);



    //============================= fwd_bem_constant_collocation.c =============================


    static int fwd_bem_check_solids (float **angles,int ntri1,int ntri2, float desired);

    static float **fwd_bem_solid_angles (MneSurfaceOrVolume::MneCSurface* *surfs, int nsurf);

    static int fwd_bem_constant_collocation_solution(FwdBemModel* m);



    //============================= fwd_bem_model.c =============================

    static int fwd_bem_compute_solution(FwdBemModel* m,
                                 int         bem_method);

    static int fwd_bem_load_recompute_solution(char        *name,
                                        int         bem_method,
                                        int         force_recompute,
                                        FwdBemModel* m);



    //============================= fwd_bem_pot.c =============================

    static float fwd_bem_inf_field(float *rd,      /* Dipole position */
                                   float *Q,       /* Dipole moment */
                                   float *rp,      /* Field point */
                                   float *dir);


    static float fwd_bem_inf_pot (float *rd,	/* Dipole position */
                           float *Q,	/* Dipole moment */
                           float *rp);

    static int fwd_bem_specify_els(FwdBemModel* m,
                            FwdCoilSet*  els);

    static void fwd_bem_lin_pot_calc (float       *rd,		/* Dipole position */
                                      float       *Q,		/* Dipole orientation */
                                      FwdBemModel* m,		/* The model */
                                      FwdCoilSet*  els,              /* Use this electrode set if available */
                                      int         all_surfs,	/* Compute on all surfaces? */
                                      float      *pot);

    static void fwd_bem_pot_calc (float       *rd,	     /* Dipole position */
                                  float       *Q,        /* Dipole orientation */
                                  FwdBemModel* m,	     /* The model */
                                  FwdCoilSet*  els,       /* Use this electrode set if available */
                                  int         all_surfs, /* Compute solution on all surfaces? */
                                  float       *pot);

    static int fwd_bem_pot_els (float       *rd,	  /* Dipole position */
                         float       *Q,	  /* Dipole orientation */
                         FwdCoilSet*  els,     /* Electrode descriptors */
                         float       *pot,    /* Result */
                         void        *client);



    //============================= fwd_bem_field.c =============================


    /*
     * These are some of the integration formulas listed in
     *
     * L. Urankar, Common compact analytical formulas for computation of
     * geometry integrals on a basic Cartesian sub-domain in boundary and
     * volume integral methods, Engineering Analysis with Boundary Elements,
     * 7 (3), 1990, 124 - 129.
     *
     */

    static void calc_f (double *xx,
                        double *yy,		/* Corner coordinates */
                        double *f0,
                        double *fx,
                        double *fy);

    static void calc_magic (double u,double z,
                            double A,
                            double B,
                            double *beta,
                            double *D);

    static void field_integrals (float *from,
                                 mneTriangle to,
                                 double *I1p,
                                 double *T,double *S1,double *S2,
                                 double *f0,double *fx,double *fy);

    static double one_field_coeff (float       *dest,	/* The destination field point */
                                   float       *normal,	/* The field direction we are interested in */
                                   mneTriangle tri);

    float **fwd_bem_field_coeff(FwdBemModel* m,	/* The model */
                                FwdCoilSet*  coils);

    /*
     * These are the formulas from Ferguson et al
     * A Complete Linear Discretization for Calculating the Magnetic Field
     * Using the Boundary-Element Method, IEEE Trans. Biomed. Eng., submitted
     */
    static double calc_gamma (double *rk,double *rk1);

    static void fwd_bem_one_lin_field_coeff_ferg (float *dest,	/* The field point */
                                           float *dir,	/* The interesting direction */
                                           mneTriangle tri,	/* The destination triangle */
                                           double *res);

    static void fwd_bem_one_lin_field_coeff_uran(float *dest,	/* The field point */
                                          float *dir,	/* The interesting direction */
                                          mneTriangle tri,	/* The destination triangle */
                                          double *res);

    static void fwd_bem_one_lin_field_coeff_simple (float       *dest,    /* The destination field point */
                                             float       *normal,  /* The field direction we are interested in */
                                             mneTriangle source,   /* The source triangle */
                                             double      *res);

    typedef void (* linFieldIntFunc)(float *dest,float *dir,mneTriangle tri, double *res);

    static float **fwd_bem_lin_field_coeff (FwdBemModel* m,        /* The model */
                                     FwdCoilSet*  coils,    /* Coil information */
                                     int         method);

    static int fwd_bem_specify_coils(FwdBemModel* m,
                              FwdCoilSet*  coils);


    #define MAG_FACTOR 1e-7		/* \mu_0/4\pi */

    static void fwd_bem_lin_field_calc(float       *rd,
                                       float       *Q,
                                       FwdCoilSet*  coils,
                                       FwdBemModel* m,
                                       float       *B);

    static void fwd_bem_field_calc(float       *rd,
                                   float       *Q,
                                   FwdCoilSet*  coils,
                                   FwdBemModel* m,
                                   float       *B);

    static int fwd_bem_field(float       *rd,	/* Dipole position */
                      float       *Q,	/* Dipole orientation */
                      FwdCoilSet*  coils,    /* Coil descriptors */
                      float       *B,       /* Result */
                      void        *client);


public:
    QString     surf_name;      /* Name of the file where surfaces were loaded from */
    MneCSurface* *surfs;        /* The interface surfaces from outside towards inside */
    int        *ntri;           /* Number of triangles on each surface */
    int        *np;             /* Number of vertices on each surface */
    int        nsurf;           /* How many */
    float      *sigma;          /* The conductivities */
    float      **gamma;         /* The gamma factors */
    float      *source_mult;    /* These multiply the infinite medium potentials */
    float      *field_mult;     /* Multipliers for the magnetic field */
    int        bem_method;      /* Which approximation method is used */
    QString     sol_name;       /* Name of the file where the solution was loaded from */

    float      **solution;      /* The potential solution matrix */
    float      *v0;             /* Space for the infinite-medium potentials */
    int        nsol;            /* Size of the solution matrix */

    FiffCoordTransOld* head_mri_t;  /* Coordinate transformation from head to MRI coordinates */

    float      ip_approach_limit;   /* Controls whether we need to use the isolated problem approach */
    bool       use_ip_approach;     /* Do we need it */

// ### OLD STRUCT ###
//typedef struct {
//    char       *surf_name;              /* Name of the file where surfaces were loaded from */
//    INVERSELIB::MneCSurface* *surfs;    /* The interface surfaces from outside towards inside */
//    int        *ntri;                   /* Number of triangles on each surface */
//    int        *np;                 /* Number of vertices on each surface */
//    int        nsurf;               /* How many */
//    float      *sigma;              /* The conductivities */
//    float      **gamma;             /* The gamma factors */
//    float      *source_mult;        /* These multiply the infinite medium potentials */
//    float      *field_mult;         /* Multipliers for the magnetic field */
//    int        bem_method;          /* Which approximation method is used */
//    char       *sol_name;           /* Name of the file where the solution was loaded from */

//    float      **solution;          /* The potential solution matrix */
//    float      *v0;                 /* Space for the infinite-medium potentials */
//    int        nsol;                /* Size of the solution matrix */

//    INVERSELIB::FiffCoordTransOld* head_mri_t;      /* Coordinate transformation from head to MRI coordinates */

//    float      ip_approach_limit;   /* Controls whether we need to use the isolated problem approach */
//    int        use_ip_approach;     /* Do we need it */
//} *fwdBemModel,fwdBemModelRec;      /* Holds the BEM model definition */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE INVERSELIB

#endif // FWDBEMMODEL_H
