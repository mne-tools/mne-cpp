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

#ifndef FWD_EEG_SPHERE_MODEL_H
#define FWD_EEG_SPHERE_MODEL_H

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

#include <QDebug>

#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

/*
 * This is the beginning of the specific code
 */
/**
 * @brief Workspace for the linear least-squares fit of Berg-Scherg parameters in the EEG sphere model (SVD matrices, residuals, weights).
 */
struct fitUserRec {
    Eigen::VectorXd y;
    Eigen::VectorXd resi;
    Eigen::MatrixXd M;
    Eigen::MatrixXd uu;
    Eigen::MatrixXd vv;
    Eigen::VectorXd sing;
    Eigen::VectorXd fn;
    Eigen::VectorXd w;
    int    nfit;
    int    nterms;
};
using fitUser = fitUserRec*;

//=============================================================================================================
/**
 * Implements FwdEegSphereModel (Replaces *fwdEegSphereModel,fwdEegSphereModelRec struct of MNE-C fwd_types.h).
 *
 * @brief Multi-layer spherical head model for EEG forward computation.
 */
class FWDSHARED_EXPORT FwdEegSphereModel
{
public:
    typedef std::unique_ptr<FwdEegSphereModel> UPtr;      /**< Unique pointer type for FwdEegSphereModel. */

    //=========================================================================================================
    /**
     * Constructs the Forward EEG Sphere Model
     *
     */
    explicit FwdEegSphereModel();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FwdEegSphereModel      Forward EEG Sphere Model which should be copied.
     */
    explicit FwdEegSphereModel(const FwdEegSphereModel& p_FwdEegSphereModel);

    //=========================================================================================================
    /**
     * Create a new multi-layer EEG sphere model structure.
     *
     * Sorts layers by radius and scales radii relative to the outermost layer.
     *
     * @param[in] name       Textual identifier for the model.
     * @param[in] nlayer     Number of concentric spherical layers.
     * @param[in] rads       Radius values for each layer.
     * @param[in] sigmas     Conductivity values for each layer.
     *
     * @return Pointer to the newly created sphere model.
     */
    static FwdEegSphereModel::UPtr fwd_create_eeg_sphere_model(const QString& name,
                                                         int nlayer,
                                                         const Eigen::VectorXf& rads,
                                                         const Eigen::VectorXf& sigmas);

    //=========================================================================================================
    /**
     * Destroys the Electric Current Dipole description
     */
    virtual ~FwdEegSphereModel();

    //=========================================================================================================
    /**
     * Set up the desired sphere model for EEG
     *
     * @param[in] eeg_model_file     Contains the model specifications.
     * @param[in] eeg_model_name     Name of the model to use.
     * @param[in] eeg_sphere_rad     Outer surface radius.
     *
     * @return the setup eeg sphere model.
     */
    static FwdEegSphereModel::UPtr setup_eeg_sphere_model(const QString& eeg_model_file, QString eeg_model_name, float eeg_sphere_rad);

    //=========================================================================================================
    /**
     * Allocate and initialize a fitting workspace for Berg-Scherg parameter estimation.
     *
     * @param[in] nfit       Number of equivalent dipoles to fit.
     * @param[in] nterms     Number of terms in the series expansion.
     *
     * @return Pointer to the initialized fitting data structure.
     */
    static fitUser new_fit_user(int nfit, int nterms);

    //=========================================================================================================
    /**
     * fwd_multi_spherepot.c
     * Get the model depended weighting factor for n
     *
     * @param[in] n  coefficient to which the expansion shopuld be calculated.
     *
     * @return       the weighting factor for n.
     */
    double fwd_eeg_get_multi_sphere_model_coeff(int n);

    //=========================================================================================================
    /**
     * Compute the next Legendre polynomials of the first and second kind
     * using a recursion formula. Self-initializes for n = 0 and n = 1.
     *
     * @param[in]  n      Order of the Legendre polynomial.
     * @param[in]  x      Evaluation point (cosine of the angle).
     * @param[out] p0     Legendre polynomial of the first kind.
     * @param[out] p01    Previous value of p0.
     * @param[out] p1     Legendre polynomial of the second kind.
     * @param[out] p11    Previous value of p1.
     */
    static void next_legen (int n,
                double x,
                double &p0,
                double &p01,
                double &p1,
                double &p11);

    //=========================================================================================================
    /**
     * Calculate the radial and tangential potential components for a dipole
     * in a layered sphere model using a Legendre polynomial series expansion.
     *
     * @param[in]  beta       Ratio of dipole distance to field point distance.
     * @param[in]  cgamma     Cosine of the angle between dipole and field point vectors.
     * @param[out] Vrp        Radial component of the potential.
     * @param[out] Vtp        Tangential component of the potential.
     * @param[in]  fn         Pre-computed expansion coefficients.
     * @param[in]  nterms     Maximum number of terms in the series.
     */
    static void calc_pot_components(double beta,
                    double cgamma,
                    double &Vrp,
                    double &Vtp,
                    const Eigen::VectorXd& fn,
                    int    nterms);

    //=========================================================================================================
    /**
     * Compute EEG potentials at multiple electrodes for a current dipole
     * in a multi-layer spherical head model.
     *
     * Based on the formulas in Zhang (1995) and Mosher et al. (1996).
     *
     * @param[in]  rd         Dipole position.
     * @param[in]  Q          Dipole moment.
     * @param[in]  el         Electrode positions (neeg x 3).
     * @param[in]  neeg       Number of electrodes.
     * @param[out] Vval       Computed potential values at each electrode.
     * @param[in]  client     Pointer to the FwdEegSphereModel.
     *
     * @return OK on success, FAIL otherwise.
     */
    static int fwd_eeg_multi_spherepot(const Eigen::Vector3f& rd,
                       const Eigen::Vector3f& Q,
                       const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>& el,
                       int     neeg,
                       Eigen::VectorXf& Vval,
                       void    *client);

    //=========================================================================================================
    /**
     * Calculate EEG potentials at compound electrodes (coils) by summing
     * weighted contributions from individual integration points.
     *
     * @param[in]  rd         Dipole position.
     * @param[in]  Q          Dipole moment.
     * @param[in]  els        Set of coils/electrodes.
     * @param[out] Vval       Computed potentials (one per coil).
     * @param[in]  client     Pointer to the FwdEegSphereModel.
     *
     * @return OK on success, FAIL otherwise.
     */
    static int fwd_eeg_multi_spherepot_coil1(const Eigen::Vector3f& rd,
                      const Eigen::Vector3f& Q,
                      FwdCoilSet& els,
                      Eigen::Ref<Eigen::VectorXf> Vval,
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
     *
     *
     * @param[in] rd         Dipole position.
     * @param[in] el         Electrode positions.
     * @param[in] neeg       Number of electrodes.
     * @param[in] Vval_vec   The potential values Vval_vec[0][k] potentials given by Q = (1.0,0.0,0.0) at electrode k; Vval_vec[1][k] potentials given by Q = (0.0,1.0,0.0) at electrode k; Vval_vec[2][k] potentials given by Q = (0.0,0.0,1.0) at electrode k.
     *
     * @return true when successful.
     */
    static bool fwd_eeg_spherepot_vec (const Eigen::Vector3f& rd, const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>& el, int neeg, Eigen::MatrixXf& Vval_vec, void *client);

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
    static int fwd_eeg_spherepot_coil_vec(const Eigen::Vector3f& rd, FwdCoilSet& els, Eigen::Ref<Eigen::MatrixXf> Vval_vec, void *client);

    //=========================================================================================================
    /**
     * Compute potentials and spatial gradients of the potential field at coils
     * using finite differences.
     *
     * @param[in]  rd         Dipole position.
     * @param[in]  Q          Dipole moment.
     * @param[in]  coils      Set of coils/electrodes.
     * @param[out] Vval       Computed potentials (one per coil).
     * @param[out] xgrad      Gradient with respect to x dipole coordinate.
     * @param[out] ygrad      Gradient with respect to y dipole coordinate.
     * @param[out] zgrad      Gradient with respect to z dipole coordinate.
     * @param[in]  client     Pointer to the FwdEegSphereModel.
     *
     * @return OK on success, FAIL otherwise.
     */
    static int fwd_eeg_spherepot_grad_coil( const Eigen::Vector3f& rd,
                                            const Eigen::Vector3f& Q,
                                            FwdCoilSet&  coils,
                                            Eigen::Ref<Eigen::VectorXf> Vval,
                                            Eigen::Ref<Eigen::VectorXf> xgrad,
                                            Eigen::Ref<Eigen::VectorXf> ygrad,
                                            Eigen::Ref<Eigen::VectorXf> zgrad,
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
    static int fwd_eeg_spherepot( const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>& el, int neeg, Eigen::VectorXf& Vval, void *client);

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
    static int fwd_eeg_spherepot_coil(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet& els, Eigen::Ref<Eigen::VectorXf> Vval, void *client);

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

    //=========================================================================================================
    /**
     * Build the data matrix and target vector for the linear part of the
     * Berg-Scherg parameter fit.
     *
     * @param[in]  mu     Distance multipliers for the equivalent dipoles.
     * @param[in]  u      Fitting workspace (M and y are populated on output).
     */
    static void compose_linear_fitting_data(const Eigen::VectorXd& mu,fitUser u);

    //=========================================================================================================
    /**
     * Solve for the optimal lambda (dipole magnitude) parameters given fixed mu
     * values using SVD. Returns the relative variance goodness-of-fit metric.
     *
     * @param[in]  mu         Distance multipliers for the equivalent dipoles.
     * @param[out] lambda     Computed dipole magnitudes.
     * @param[in]  u          Fitting workspace.
     *
     * @return Relative variance (0 = perfect fit, 1 = no fit).
     */
    static double compute_linear_parameters(const Eigen::VectorXd& mu, Eigen::VectorXd& lambda, fitUser u);

    //=========================================================================================================
    /**
     * Objective function for the simplex optimizer: evaluate the residual
     * sum of squares for a given set of mu values.
     *
     * @param[in] mu          Distance multipliers to evaluate.
     * @param[in] user_data   Pointer to the fitUser workspace.
     *
     * @return Sum of squared residuals; 1.0 if any mu exceeds +/-1.
     */
    static double one_step (const Eigen::VectorXd& mu, const void *user_data);

    //=========================================================================================================
    /**
     * Fit Berg-Scherg equivalent spherical model parameters (mu and lambda)
     * by minimizing the difference between actual and approximated
     * series expansions using simplex optimization.
     *
     * On success, updates the internal mu and lambda member vectors.
     *
     * @param[in]  nterms     Number of terms in the series expansion.
     * @param[in]  nfit       Number of equivalent dipoles to fit (must be >= 2).
     * @param[out] rv         Relative variance of the final fit.
     *
     * @return true if fitting succeeded, false otherwise.
     */
    bool fwd_eeg_fit_berg_scherg(int   nterms,
                                int   nfit,
                                float &rv);

/**< Number of layers. */
    int   nlayer() const
    {
        return layers.size();
    }

public:
    QString                     name;   /**< Textual identifier. */
    std::vector<FwdEegSphereLayer> layers; /**< An array of layers. */
    Eigen::Vector3f             r0;     /**< The origin. */

    Eigen::VectorXd fn;                 /**< Coefficients saved to speed up the computations. */
    int             nterms;             /**< How many?. */

    Eigen::VectorXf mu;             /**< The Berg-Scherg equivalence parameters. */
    Eigen::VectorXf lambda;
    int             nfit;           /**< How many?. */
    int             scale_pos;      /**< Scale the positions to the surface of the sphere?. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_EEG_SPHERE_MODEL_H
