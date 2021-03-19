//=============================================================================================================
/**
 * @file     fwd_bem_model.h
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
 * @brief    FwdBemModel class declaration.
 *
 */

#ifndef FWDBEMMODEL_H
#define FWDBEMMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_coil_set.h"

#include <fiff/c/fiff_coord_trans_old.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_named_matrix.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
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

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB
{
    class MneTriangle;
    class MneSurfaceOld;
    class MneSourceSpaceOld;
    class MneCTFCompDataSet;
    class MneNamedMatrix;
}

namespace FIFFLIB {
    class FiffNamedMatrix;
}
//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
// FWDLIB FORWARD DECLARATIONS
//=============================================================================================================

class FwdEegSphereModel;

//=============================================================================================================
/**
 * Implements FwdBemModel (Replaces *FwdBemModel*,FwdBemModel*Rec struct of MNE-C fwd_types.h).
 *
 * @brief Holds the BEM model definition
 */
class FWDSHARED_EXPORT FwdBemModel
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
     * Refactored: fwd_bem_free_model (fwd_bem_free_model.c)
     */
    virtual ~FwdBemModel();

    //=========================================================================================================
    /**
     * Refactored: fwd_bem_free_solution (fwd_bem_model.c)
     */
    void fwd_bem_free_solution();

//char *fwd_bem_make_bem_name(char *name)
///*
// * Make a standard BEM file name
// */
//{
//    char *s1,*s2;

//    s1 = strip_from(name,(char*)(".fif"));
//    s2 = strip_from(s1,(char*)("-sol"));
//    FREE_3(s1);
//    s1 = strip_from(s2,(char*)("-bem"));
//    FREE_3(s2);
//    s2 = MALLOC_3(strlen(s1)+strlen(BEM_SUFFIX)+1,char);
//    sprintf(s2,"%s%s",s1,BEM_SUFFIX);
//    FREE_3(s1);
//    return s2;
//}

    static QString fwd_bem_make_bem_sol_name(const QString& name);

    //============================= fwd_bem_model.c =============================

    static const QString& fwd_bem_explain_surface(int kind);

    static const QString& fwd_bem_explain_method(int method);

    static int get_int( FIFFLIB::FiffStream::SPtr& stream, const FIFFLIB::FiffDirNode::SPtr& node,int what,int *res);

    /*
     * Return a pointer to a specific surface in a BEM
     */
    // Refactored: fwd_bem_find_surface (fwd_bem_model.c)
    MNELIB::MneSurfaceOld* fwd_bem_find_surface(int kind);

    static FwdBemModel* fwd_bem_load_surfaces(const QString& name,
                                      int  *kinds,
                                      int  nkind);

    static FwdBemModel* fwd_bem_load_homog_surface(const QString& name);

    static FwdBemModel* fwd_bem_load_three_layer_surfaces(const QString& name);

    static int fwd_bem_load_solution(const QString& name, int bem_method, FwdBemModel* m);

    static int fwd_bem_set_head_mri_t(FwdBemModel* m, FIFFLIB::FiffCoordTransOld* t);

    //============================= dipole_fit_guesses.c =============================

    static MNELIB::MneSurfaceOld* make_guesses(MNELIB::MneSurfaceOld* guess_surf, /* Predefined boundary for the guesses */
                                     float guessrad,     /* Radius for the spherical boundary if the above is missing */
                                     float *guess_r0,    /* Origin for the spherical boundary */
                                     float grid,         /* Spacing between guess points */
                                     float exclude,      /* Exclude points closer than this to the CM of the guess boundary surface */
                                     float mindist);

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
                               MNELIB::MneTriangle* to,	/* The destination triangle */
                               double omega[3]);

    static void correct_auto_elements (MNELIB::MneSurfaceOld* surf,
                                       float      **mat);

    static float **fwd_bem_lin_pot_coeff (const QList<MNELIB::MneSurfaceOld*>& surfs);

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

    static float **fwd_bem_solid_angles (const QList<MNELIB::MneSurfaceOld*>& surfs);

    static int fwd_bem_constant_collocation_solution(FwdBemModel* m);

    //============================= fwd_bem_model.c =============================

    static int fwd_bem_compute_solution(FwdBemModel* m,
                                 int         bem_method);

    static int fwd_bem_load_recompute_solution(const QString& name,
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

    static void fwd_bem_pot_grad_calc (float *rd,   /* Dipole position */
                       float        *Q,             /* Dipole orientation */
                       FwdBemModel* m,              /* The model */
                       FwdCoilSet*  els,            /* Use this electrode set if available */
                       int          all_surfs,      /* Compute solution on all surfaces? */
                       float        *xgrad,
                       float        *ygrad,
                       float        *zgrad);

    static void fwd_bem_lin_pot_calc (float       *rd,		/* Dipole position */
                                      float       *Q,		/* Dipole orientation */
                                      FwdBemModel* m,		/* The model */
                                      FwdCoilSet*  els,              /* Use this electrode set if available */
                                      int         all_surfs,	/* Compute on all surfaces? */
                                      float      *pot);

    static void fwd_bem_lin_pot_grad_calc (float       *rd,		/* Dipole position */
                           float       *Q,		/* Dipole orientation */
                           FwdBemModel* m,		/* The model */
                           FwdCoilSet* els,         /* Use this electrode set if available */
                           int         all_surfs,	/* Compute on all surfaces? */
                           float       *xgrad,
                           float       *ygrad,
                           float       *zgrad);

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

    static int fwd_bem_pot_grad_els (float       *rd,     /* Dipole position */
                  float       *Q,      /* Dipole orientation */
                  FwdCoilSet* els,     /* Electrode descriptors */
                  float       *pot,    /* Potentials */
                  float       *xgrad,  /* Derivatives with respect to dipole position */
                  float       *ygrad,
                  float       *zgrad,
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
                                 MNELIB::MneTriangle* to,
                                 double *I1p,
                                 double *T,double *S1,double *S2,
                                 double *f0,double *fx,double *fy);

    static double one_field_coeff (float       *dest,	/* The destination field point */
                                   float       *normal,	/* The field direction we are interested in */
                                   MNELIB::MneTriangle* tri);

    static float **fwd_bem_field_coeff(FwdBemModel* m,	/* The model */
                                FwdCoilSet*  coils);

    /*
     * These are the formulas from Ferguson et al
     * A Complete Linear Discretization for Calculating the Magnetic Field
     * Using the Boundary-Element Method, IEEE Trans. Biomed. Eng., submitted
     */
    static double calc_gamma (double *rk,double *rk1);

    static void fwd_bem_one_lin_field_coeff_ferg (float *dest,	/* The field point */
                                           float *dir,	/* The interesting direction */
                                           MNELIB::MneTriangle* tri,	/* The destination triangle */
                                           double *res);

    static void fwd_bem_one_lin_field_coeff_uran(float *dest,	/* The field point */
                                          float *dir,	/* The interesting direction */
                                          MNELIB::MneTriangle* tri,	/* The destination triangle */
                                          double *res);

    static void fwd_bem_one_lin_field_coeff_simple (float       *dest,    /* The destination field point */
                                             float       *normal,  /* The field direction we are interested in */
                                             MNELIB::MneTriangle* source,   /* The source triangle */
                                             double      *res);

    typedef void (* linFieldIntFunc)(float *dest,float *dir,MNELIB::MneTriangle* tri, double *res);

    static float **fwd_bem_lin_field_coeff (FwdBemModel* m,        /* The model */
                                     FwdCoilSet*  coils,    /* Coil information */
                                     int         method);

    static int fwd_bem_specify_coils(FwdBemModel* m,
                              FwdCoilSet*  coils);

    #define MAG_FACTOR 1e-7         /* \mu_0/4\pi */

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

    static void fwd_bem_field_grad_calc(float       *rd,
                        float       *Q,
                        FwdCoilSet  *coils,
                        FwdBemModel *m,
                        float       *xgrad,
                        float       *ygrad,
                        float       *zgrad);

    static float fwd_bem_inf_field_der(float *rd,      /* Dipole position */
                       float *Q,	   /* Dipole moment */
                       float *rp,	   /* Field point */
                       float *dir,     /* Which field component */
                       float *comp);

    static float fwd_bem_inf_pot_der (float *rd,   /* Dipole position */
                               float *Q,    /* Dipole moment */
                               float *rp,   /* Potential point */
                               float *comp);

    static void fwd_bem_lin_field_grad_calc(float       *rd,
                                            float       *Q,
                                            FwdCoilSet  *coils,
                                            FwdBemModel *m,
                                            float       *xgrad,
                                            float       *ygrad,
                                            float       *zgrad);

    static int fwd_bem_field(float       *rd,	/* Dipole position */
                      float       *Q,	/* Dipole orientation */
                      FwdCoilSet*  coils,    /* Coil descriptors */
                      float       *B,       /* Result */
                      void        *client);

    static int fwd_bem_field_grad(float        *rd,      /* The dipole location */
                   float        Q[],      /* The dipole components (xyz) */
                   FwdCoilSet*  coils,    /* The coil definitions */
                   float        Bval[],   /* Results */
                   float        xgrad[],  /* The derivatives with respect to */
                   float        ygrad[],  /* the dipole position coordinates */
                   float        zgrad[],
                   void         *client);

    //============================= compute_forward.c =============================

    static void *meg_eeg_fwd_one_source_space(void *arg);

    // TODO check if this is the correct class or move
    static int compute_forward_meg( MNELIB::MneSourceSpaceOld*  *spaces,        /**< Source spaces. */
                                    int                         nspace,         /**< How many?. */
                                    FwdCoilSet*                 coils,          /**< MEG Coilset. */
                                    FwdCoilSet*                 comp_coils,     /**< Compensator Coilset. */
                                    MNELIB::MneCTFCompDataSet*  comp_data,      /**< Compensator Data. */
                                    bool                        fixed_ori,      /**< Use fixed-orientation dipoles. */
                                    FwdBemModel*                bem_model,      /**< BEM model definition. */
                                    Eigen::Vector3f*            r0,             /**< Sphere model origin. */
                                    bool                        use_threads,    /**< Parallelize with threads?. */
                                    FIFFLIB::FiffNamedMatrix&   resp,           /**< The results. */
                                    FIFFLIB::FiffNamedMatrix&   resp_grad,
                                    bool bDoGRad);                              /**< calculate gradient solution. */

    static int compute_forward_eeg( MNELIB::MneSourceSpaceOld*  *spaces,        /**< Source spaces. */
                                    int                         nspace,         /**< How many?. */
                                    FwdCoilSet*                 els,            /**< Electrode locations. */
                                    bool                        fixed_ori,      /**< Use fixed-orientation dipoles. */
                                    FwdBemModel*                bem_model,      /**< BEM model definition. */
                                    FwdEegSphereModel*          m,              /**< Sphere model definition. */
                                    bool                        use_threads,    /**< Parallelize with threads?. */
                                    FIFFLIB::FiffNamedMatrix&   resp,           /**< The results. */
                                    FIFFLIB::FiffNamedMatrix&   resp_grad,
                                    bool                        bDoGrad);       /**< calculate gradient solution. */

    //============================= fwd_spherefield.c =============================
    // TODO location of these functions need to be checked -> evtl moving to more suitable space
    static int fwd_sphere_field(float        *rd,	        /* The dipole location */
                         float        Q[],	        /* The dipole components (xyz) */
                         FwdCoilSet*   coils,	/* The coil definitions */
                         float        Bval[],	/* Results */
                         void         *client);

    static int fwd_sphere_field_vec(float        *rd,	/* The dipole location */
                             FwdCoilSet*   coils,	/* The coil definitions */
                             float        **Bval,  /* Results: rows are the fields of the x,y, and z direction dipoles */
                             void         *client);

    static int fwd_sphere_field_grad(float        *rd,	 /* The dipole location */
                  float        Q[],      /* The dipole components (xyz) */
                  FwdCoilSet*  coils,    /* The coil definitions */
                  float        Bval[],   /* Results */
                  float        xgrad[],  /* The derivatives with respect to */
                  float        ygrad[],  /* the dipole position coordinates */
                  float        zgrad[],
                  void         *client);

    //============================= fwd_mag_dipole_field.c =============================
    // TODO location of these functions need to be checked -> evtl moving to mor suitable space
    /*
     * Compute the field of a magnetic dipole
     */
    static int fwd_mag_dipole_field( float        *rm,      /* The dipole location in the same coordinate system as the coils */
                                     float        M[],	/* The dipole components (xyz) */
                                     FwdCoilSet*   coils,	/* The coil definitions */
                                     float        Bval[],	/* Results */
                                     void         *client);

    static int fwd_mag_dipole_field_vec( float        *rm,	        /* The dipole location */
                                         FwdCoilSet*   coils,	/* The coil definitions */
                                         float        **Bval,       /* Results: rows are the fields of the x,y, and z direction dipoles */
                                         void         *client);

public:
    QString     surf_name;      /* Name of the file where surfaces were loaded from */
    QList<MNELIB::MneSurfaceOld*> surfs;      /* The interface surfaces from outside towards inside */
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

    FIFFLIB::FiffCoordTransOld* head_mri_t;  /* Coordinate transformation from head to MRI coordinates */

    float      ip_approach_limit;   /* Controls whether we need to use the isolated problem approach */
    bool       use_ip_approach;     /* Do we need it */

// ### OLD STRUCT ###
//typedef struct {
//    char       *surf_name;              /* Name of the file where surfaces were loaded from */
//    FWDLIB::MneSurfaceOld* *surfs;    /* The interface surfaces from outside towards inside */
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

//    FWDLIB::FiffCoordTransOld* head_mri_t;      /* Coordinate transformation from head to MRI coordinates */

//    float      ip_approach_limit;   /* Controls whether we need to use the isolated problem approach */
//    int        use_ip_approach;     /* Do we need it */
//} *FwdBemModel*,FwdBemModel*Rec;      /* Holds the BEM model definition */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWDBEMMODEL_H
