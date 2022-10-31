//=============================================================================================================
/**
 * @file     fwd_eeg_sphere_model.h
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
 * @brief    FwdEegSphereModel class declaration.
 *
 */

#ifndef FWDEEGSPHEREMODEL_H
#define FWDEEGSPHEREMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_eeg_sphere_layer.h"
#include "fwd_coil_set.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

/*
 * This is the beginning of the specific code
 */
typedef struct {
    double *y;
    double *resi;
    double **M;
    double **uu;
    double **vv;
    double *sing;
    double *fn;
    double *w;
    int    nfit;
    int    nterms;
} *fitUser,fitUserRec;

//=============================================================================================================
/**
 * Implements FwdEegSphereModel (Replaces *fwdEegSphereModel,fwdEegSphereModelRec struct of MNE-C fwd_types.h).
 *
 * @brief Electric Current Dipole description
 */
class FWDSHARED_EXPORT FwdEegSphereModel
{
public:
    typedef QSharedPointer<FwdEegSphereModel> SPtr;              /**< Shared pointer type for FwdEegSphereModel. */
    typedef QSharedPointer<const FwdEegSphereModel> ConstSPtr;   /**< Const shared pointer type for FwdEegSphereModel. */

    //=========================================================================================================
    /**
     * Constructs the Forward EEG Sphere Model
     * Refactored: fwd_new_eeg_sphere_model
     *
     */
    explicit FwdEegSphereModel();

    //=========================================================================================================
    /**
     * Copy constructor.
     * Refactored: fwd_dup_eeg_sphere_model
     *
     * @param[in] p_FwdEegSphereModel      Forward EEG Sphere Model which should be copied.
     */
    explicit FwdEegSphereModel(const FwdEegSphereModel& p_FwdEegSphereModel);

    /*
          * Produce a new sphere model structure
          */
    static FwdEegSphereModel* fwd_create_eeg_sphere_model(const QString& name,
                                                         int nlayer,
                                                         const Eigen::VectorXf& rads,
                                                         const Eigen::VectorXf& sigmas);

    //=========================================================================================================
    /**
     * Destroys the Electric Current Dipole description
     * Refactored: fwd_free_eeg_sphere_model
     */
    virtual ~FwdEegSphereModel();

    //=========================================================================================================
    /**
     * Set up the desired sphere model for EEG
     * Refactored: setup_eeg_sphere_model (dipole_fit_setup.c)
     *
     * @param[in] eeg_model_file     Contains the model specifications.
     * @param[in] eeg_model_name     Name of the model to use.
     * @param[in] eeg_sphere_rad     Outer surface radius.
     *
     * @return the setup eeg sphere model.
     */
    static FwdEegSphereModel* setup_eeg_sphere_model(const QString& eeg_model_file, QString eeg_model_name, float eeg_sphere_rad);

    static fitUser new_fit_user(int nfit, int nterms);

    //=========================================================================================================
    /**
     * fwd_multi_spherepot.c
     * Get the model depended weighting factor for n
     * Refactored: fwd_eeg_get_multi_sphere_model_coeff (fwd_multi_spherepot.c)
     *
     * @param[in] n  coefficient to which the expansion shopuld be calculated.
     *
     * @return       the weighting factor for n.
     */
    double fwd_eeg_get_multi_sphere_model_coeff(int n);

    static void next_legen (int n,
                double x,
                double *p0,         /* Input: P0(n-1) Output: P0(n) */
                double *p01,        /* Input: P0(n-2) Output: P0(n-1) */
                double *p1,	    /* Input: P1(n-1) Output: P1(n) */
                double *p11);

    static void calc_pot_components(double beta,   /* rd/r */
                    double cgamma, /* Cosine of the angle between
                            * the source and field points */
                    double *Vrp,   /* Potential component for the radial dipole */
                    double *Vtp,   /* Potential component for the tangential dipole */
                    const Eigen::VectorXd& fn,
                    int    nterms);

    static int fwd_eeg_multi_spherepot(float   *rd,	          /* Dipole position */
                       float   *Q,	          /* Dipole moment */
                       float   **el,	  /* Electrode positions */
                       int     neeg,	  /* Number of electrodes */
                       float   *Vval,         /* The potential values */
                       void    *client);

    static int fwd_eeg_multi_spherepot_coil1(float *rd,    /* Dipole position */
                      float      *Q,                /* Dipole moment */
                      FwdCoilSet* els,              /* Electrode positions */
                      float      *Vval,             /* The potential values */
                      void       *client);

    //=========================================================================================================
    /**
     * Compute the electric potentials in a set of electrodes in spherically
     * Symmetric head model. This routine calculates the fields for all
     * dipole directions.
     *
     * The code is based on the formulas presented in
     *
     * J.C. Moscher, R.M. Leahy, and P.S. Lewis, Matrix Kernels for
     * Modeling of EEG and MEG Data, Los Alamos Technical Report,
     * LA-UR-96-1993, 1996.
     *
     * This routine uses the acceleration with help of equivalent sources
     * in the homogeneous sphere.
     *
     * Refactored: fwd_eeg_spherepot_vec (fwd_multi_spherepot.c)
     *
     *
     * @param[in] rd         Dipole position.
     * @param[in] el         Electrode positions.
     * @param[in] neeg       Number of electrodes.
     * @param[in] Vval_vec   The potential values Vval_vec[0][k] potentials given by Q = (1.0,0.0,0.0) at electrode k; Vval_vec[1][k] potentials given by Q = (0.0,1.0,0.0) at electrode k; Vval_vec[2][k] potentials given by Q = (0.0,0.0,1.0) at electrode k.
     *
     * @return true when successful.
     */
    static bool fwd_eeg_spherepot_vec (float *rd, float **el, int neeg, float **Vval_vec, void *client);

    //=========================================================================================================
    /**
     * fwd_multi_spherepot.c
     *
     * Calculate the EEG in the sphere model using the fwdCoilSet structure
     * MEG channels are skipped
     *
     * This routine uses the acceleration with help of equivalent sources
     * in the homogeneous sphere.
     *
     *
     * @param[in] rd         Dipole position.
     * @param[in] els        Electrode positions.
     * @param[in] Vval_vec   The potential values; Vval_vec[0][k] potentials given by Q = (1.0,0.0,0.0) at electrode k; Vval_vec[1][k] potentials given by Q = (0.0,1.0,0.0) at electrode k; Vval_vec[2][k] potentials given by Q = (0.0,0.0,1.0) at electrode k.
     * @param[in] client.
     *
     * @return true when successful.
     */
    static int fwd_eeg_spherepot_coil_vec(float *rd, FwdCoilSet* els, float **Vval_vec, void *client);

    static int fwd_eeg_spherepot_grad_coil( float        *rd,      /* The dipole location */
                                            float        Q[],      /* The dipole components (xyz) */
                                            FwdCoilSet*  coils,    /* The coil definitions */
                                            float        Vval[],   /* Results */
                                            float        xgrad[],  /* The derivatives with respect to */
                                            float        ygrad[],  /* the dipole position coordinates */
                                            float        zgrad[],
                                            void         *client);

    //=========================================================================================================
    /**
     * fwd_multi_spherepot.c
     *
     * This routine calculates the potentials for a specific dipole direction
     *
     * This routine uses the acceleration with help of equivalent sources
     * in the homogeneous sphere.
     *
     * @param[in] rd         Dipole position.
     * @param[in] Q          Dipole moment.
     * @param[in] el         Electrode positions.
     * @param[in] neeg       Number of electrodes.
     * @param[in] Vval       The potential values.
     * @param[in] client.
     *
     * @return true when successful.
     */
    static int fwd_eeg_spherepot( float *rd, float *Q, float **el, int neeg, Eigen::VectorXf& Vval, void *client);

    //=========================================================================================================
    /**
     * fwd_multi_spherepot.c
     *
     * Calculate the EEG in the sphere model using the megCoil structure
     * MEG channels are skipped
     *
     * @param[in] rd         Dipole position.
     * @param[in] Q          Dipole moment.
     * @param[in] els        Electrode positions.
     * @param[in] Vval       The potential values.
     * @param[in] client.
     *
     * @return true when successful.
     */
    static int fwd_eeg_spherepot_coil(float *rd, float *Q, FwdCoilSet* els, float *Vval, void *client);

    //=========================================================================================================
    /**
     * fwd_eeg_sphere_models.c
     *
     * Setup the EEG sphere model calculations
     *
     * @param[in] rad.
     * @param[in] fit_berg_scherg    If Fit Berg Scherg should be performed.
     * @param[in] nfit.
     *
     * @return True when setup was successful, false otherwise.
     */
    bool fwd_setup_eeg_sphere_model(float rad, bool fit_berg_scherg, int nfit);

    // fwd_fit_berg_scherg.c

    static void compose_linear_fitting_data(const Eigen::VectorXd& mu,fitUser u);

    // fwd_fit_berg_scherg.c
    /*
          * Compute the best-fitting linear parameters
          * Return the corresponding RV
          */
    static double compute_linear_parameters(const Eigen::VectorXd& mu, Eigen::VectorXd& lambda, fitUser u);

    // fwd_fit_berg_scherg.c
    /*
          * Evaluate the residual sum of squares fit for one set of
          * mu values
          */
    static double one_step (const Eigen::VectorXd& mu, const void *user_data);

    /*
          * This routine fits the Berg-Scherg equivalent spherical model
          * dipole parameters by minimizing the difference between the
          * actual and approximative series expansions
          */
    bool fwd_eeg_fit_berg_scherg(int   nterms,              /* Number of terms to use in the series expansion
                                                                                        * when fitting the parameters */
                                int   nfit,	               /* Number of equivalent dipoles to fit */
                                float &rv);

/**< Number of layers. */
    int   nlayer() const
    {
        return layers.size();
    }

public:
    QString                     name;   /**< Textual identifier. */
    QList<FwdEegSphereLayer>    layers; /**< An array of layers. */
    Eigen::Vector3f             r0;     /**< The origin. */

    Eigen::VectorXd fn;                 /**< Coefficients saved to speed up the computations. */
    int             nterms;             /**< How many?. */

    Eigen::VectorXf mu;             /**< The Berg-Scherg equivalence parameters. */
    Eigen::VectorXf lambda;
    int             nfit;           /**< How many?. */
    int             scale_pos;      /**< Scale the positions to the surface of the sphere?. */

// ### OLD STRUCT ###
//    typedef struct {
//      char  *name;                /* Textual identifier */
//      int   nlayer;               /* Number of layers */
//      fwdEegSphereLayer layers;   /* An array of layers */
//      float  r0[3];               /* The origin */

//      double *fn;         /* Coefficients saved to speed up the computations */
//      int    nterms;      /* How many? */

//      float  *mu;         /* The Berg-Scherg equivalence parameters */
//      float  *lambda;
//      int    nfit;        /* How many? */
//      int    scale_pos;   /* Scale the positions to the surface of the sphere? */
//    } *fwdEegSphereModel,fwdEegSphereModelRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWDEEGSPHEREMODEL_H
