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

#ifndef FWD_BEM_MODEL_H
#define FWD_BEM_MODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_coil_set.h"

#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_named_matrix.h>

#include <memory>
#include <vector>

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
    class MNETriangle;
    class MNESurface;
    class MNESourceSpace;
    class MNECTFCompDataSet;
    class MNENamedMatrix;
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
class FwdThreadArg;

//=============================================================================================================
/**
 * @brief BEM (Boundary Element Method) model definition.
 *
 * Holds the BEM model surfaces, conductivity parameters, and potential
 * solution matrix used in MEG/EEG forward computations. Surfaces are
 * stored from outermost to innermost and owned by this object.
 *
 * Refactored from the MNE-C fwdBemModel / fwdBemModelRec struct
 * (fwd_types.h). Raw C-style arrays replaced with Eigen types;
 * surface ownership managed through std::unique_ptr.
 */
class FWDSHARED_EXPORT FwdBemModel
{
public:
    typedef QSharedPointer<FwdBemModel> SPtr;              /**< Shared pointer type for FwdBemModel. */
    typedef QSharedPointer<const FwdBemModel> ConstSPtr;   /**< Const shared pointer type for FwdBemModel. */

    //=========================================================================================================
    /**
     * @brief Constructs an empty BEM model.
     *
     * All Eigen containers are left at size zero; scalar members are
     * set to safe defaults (nsurf = 0, nsol = 0, bem_method = FWD_BEM_UNKNOWN).
     */
    explicit FwdBemModel();

    //=========================================================================================================
    /**
     * @brief Destroys the BEM model.
     *
     * Surface ownership is released automatically via std::unique_ptr.
     * The solution matrix is freed by fwd_bem_free_solution().
     */
    virtual ~FwdBemModel();

    //=========================================================================================================
    /**
     * @brief Release the potential solution matrix and associated workspace.
     *
     * Resets solution, v0, sol_name, nsol, and bem_method to their
     * default (empty / unknown) state.
     */
    void fwd_bem_free_solution();

    /**
     * @brief Build a standard BEM solution file name from a model name.
     *
     * @param[in] name  Base BEM file name.
     * @return The derived solution file name.
     */
    static QString fwd_bem_make_bem_sol_name(const QString& name);

    //============================= fwd_bem_model.c =============================

    /**
     * @brief Return a human-readable label for a BEM surface kind.
     *
     * @param[in] kind  FIFF surface ID (e.g. FIFFV_BEM_SURF_ID_BRAIN).
     * @return Reference to a static string label.
     */
    static const QString& fwd_bem_explain_surface(int kind);

    /**
     * @brief Return a human-readable label for a BEM method.
     *
     * @param[in] method  BEM method constant (FWD_BEM_CONSTANT_COLL / FWD_BEM_LINEAR_COLL).
     * @return Reference to a static string label.
     */
    static const QString& fwd_bem_explain_method(int method);

    /**
     * @brief Read an integer tag from a FIFF node.
     */
    static int get_int( FIFFLIB::FiffStream::SPtr& stream, const FIFFLIB::FiffDirNode::SPtr& node,int what,int *res);

    /**
     * @brief Find a surface of the given kind in this BEM model.
     *
     * @param[in] kind  Surface ID to look for.
     * @return Non-owning pointer to the surface, or nullptr if not found.
     */
    MNELIB::MNESurface* fwd_bem_find_surface(int kind);

    //=========================================================================================================
    /**
     * @brief Load BEM surfaces of specified kinds from a FIFF file.
     *
     * @param[in] name   Path to the BEM FIFF file.
     * @param[in] kinds  Surface IDs to load (e.g. FIFFV_BEM_SURF_ID_BRAIN).
     * @return BEM model containing the requested surfaces, or nullptr on failure.
     */
    static std::unique_ptr<FwdBemModel> fwd_bem_load_surfaces(const QString& name,
                                              const std::vector<int>& kinds);

    //=========================================================================================================
    /**
     * @brief Load a single-layer (homogeneous) BEM model from a FIFF file.
     *
     * Convenience wrapper that loads only the inner skull surface.
     *
     * @param[in] name  Path to the BEM FIFF file.
     * @return BEM model, or nullptr on failure.
     */
    static std::unique_ptr<FwdBemModel> fwd_bem_load_homog_surface(const QString& name);

    //=========================================================================================================
    /**
     * @brief Load a three-layer BEM model (scalp, outer skull, inner skull) from a FIFF file.
     *
     * @param[in] name  Path to the BEM FIFF file.
     * @return BEM model, or nullptr on failure.
     */
    static std::unique_ptr<FwdBemModel> fwd_bem_load_three_layer_surfaces(const QString& name);

    //=========================================================================================================
    /**
     * @brief Load a pre-computed BEM solution from a FIFF file.
     *
     * Reads the solution matrix from disk and stores it in this model.
     * The solution must match the requested BEM method.
     *
     * @param[in] name        Path to the solution FIFF file.
     * @param[in] bem_method  Required BEM method (FWD_BEM_CONSTANT_COLL or FWD_BEM_LINEAR_COLL).
     * @return OK on success, FAIL on error.
     */
    int fwd_bem_load_solution(const QString& name, int bem_method);

    //=========================================================================================================
    /**
     * @brief Set the Head-to-MRI coordinate transform for this BEM model.
     *
     * @param[in] t  The head-to-MRI coordinate transformation.
     * @return OK on success, FAIL on error.
     */
    int fwd_bem_set_head_mri_t(const FIFFLIB::FiffCoordTrans& t);

    //============================= dipole_fit_guesses.c =============================

    //=========================================================================================================
    /**
     * @brief Generate a set of dipole guess locations inside a boundary surface.
     *
     * Creates an evenly-spaced grid of candidate source locations, optionally
     * excluding points too close to the center of mass or outside the boundary.
     *
     * @param[in] guess_surf  Predefined boundary surface for the guesses (may be nullptr).
     * @param[in] guessrad    Radius for a spherical boundary if guess_surf is nullptr.
     * @param[in] guess_r0    Origin for the spherical boundary.
     * @param[in] grid        Spacing between guess points (meters).
     * @param[in] exclude     Exclude points closer than this to the CM of the boundary.
     * @param[in] mindist     Minimum distance from the boundary surface.
     * @return Surface containing the guess locations, or nullptr on failure.
     */
    static std::unique_ptr<MNELIB::MNESurface> make_guesses(MNELIB::MNESurface* guess_surf,
                                            float guessrad,
                                            const Eigen::Vector3f& guess_r0,
                                            float grid,
                                            float exclude,
                                            float mindist);

    //============================= fwd_bem_linear_collocation.c =============================

    /*
     * The following approach is based on:
     *
     * de Munck JC: "A linear discretization of the volume conductor boundary integral equation
     * using analytically integrated elements",
     * IEEE Trans Biomed Eng. 1992 39(9) : 986 - 990
     */

    //=========================================================================================================
    /**
     * @brief Compute the beta angle used in the linear collocation integration.
     *
     * @param[in] rk   Position vector of vertex k.
     * @param[in] rk1  Position vector of vertex k+1.
     * @return The beta angle value.
     */
    static double calc_beta(const Eigen::Vector3d& rk, const Eigen::Vector3d& rk1);

    //=========================================================================================================
    /**
     * @brief Compute the linear potential coefficients for one source-destination pair.
     *
     * @param[in]  from   Source point.
     * @param[in]  to     Destination triangle.
     * @param[out] omega  Output coefficients for the three triangle vertices.
     */
    static void lin_pot_coeff(const Eigen::Vector3f& from,
                              MNELIB::MNETriangle& to,
                              Eigen::Vector3d& omega);

    //=========================================================================================================
    /**
     * @brief Correct the auto (self-coupling) elements of the linear collocation matrix.
     *
     * @param[in]     surf  The BEM surface.
     * @param[in,out] mat   The coefficient matrix to correct (modified in-place).
     */
    static void correct_auto_elements(MNELIB::MNESurface& surf,
                                      Eigen::MatrixXf& mat);

    //=========================================================================================================
    /**
     * @brief Assemble the full linear-collocation potential coefficient matrix.
     *
     * @param[in] surfs  Vector of BEM surfaces.
     * @return Coefficient matrix (rows = nodes, columns = nodes).
     */
    static Eigen::MatrixXf fwd_bem_lin_pot_coeff(const std::vector<MNELIB::MNESurface*>& surfs);

    //=========================================================================================================
    /**
     * @brief Compute the linear-collocation BEM solution for this model.
     *
     * @return OK on success, FAIL on error.
     */
    int fwd_bem_linear_collocation_solution();

    //============================= fwd_bem_solution.c =============================

    //=========================================================================================================
    /**
     * @brief Compute the multi-surface BEM solution from solid-angle coefficients.
     *
     * Applies the deflation technique and LU decomposition to produce
     * the final BEM solution matrix for a multi-compartment model.
     *
     * @param[in] solids  Solid-angle coefficient matrix.
     * @param[in] gamma   Conductivity-ratio coupling matrix (nullptr for homogeneous).
     * @param[in] nsurf   Number of surfaces.
     * @param[in] ntri    Triangle or node count per surface.
     * @return Solution matrix, or empty matrix on error.
     */
    static Eigen::MatrixXf fwd_bem_multi_solution(Eigen::MatrixXf& solids,
                                          const Eigen::MatrixXf *gamma,
                                          int nsurf,
                                          const Eigen::VectorXi& ntri);

    //=========================================================================================================
    /**
     * @brief Compute the homogeneous (single-layer) BEM solution.
     *
     * @param[in] solids  Solid-angle coefficient matrix.
     * @param[in] ntri    Number of triangles.
     * @return Solution matrix, or empty matrix on error.
     */
    static Eigen::MatrixXf fwd_bem_homog_solution(Eigen::MatrixXf& solids, int ntri);

    //=========================================================================================================
    /**
     * @brief Modify the BEM solution with the isolated-problem (IP) approach.
     *
     * Applies the correction for the innermost surface conductivity jump
     * (the "isolated problem" approach of Hamalainen and Sarvas, 1989).
     *
     * @param[in,out] solution     The original solution matrix (modified in-place).
     * @param[in]     ip_solution  The isolated-problem solution matrix.
     * @param[in]     ip_mult      Conductivity ratio for the IP correction.
     * @param[in]     nsurf        Number of surfaces.
     * @param[in]     ntri         Triangle or node count per surface.
     */
    static void fwd_bem_ip_modify_solution(Eigen::MatrixXf &solution,
                                           Eigen::MatrixXf& ip_solution,
                                           float ip_mult,
                                           int nsurf,
                                           const Eigen::VectorXi &ntri);

    //============================= fwd_bem_constant_collocation.c =============================

    //=========================================================================================================
    /**
     * @brief Verify that solid-angle sums match the expected value.
     *
     * @param[in] angles   Solid-angle matrix.
     * @param[in] ntri1    Row count.
     * @param[in] ntri2    Column count.
     * @param[in] desired  Expected solid-angle sum per row.
     * @return OK if all rows match within tolerance, FAIL otherwise.
     */
    static int fwd_bem_check_solids(const Eigen::MatrixXf& angles, int ntri1, int ntri2, float desired);

    //=========================================================================================================
    /**
     * @brief Compute the solid-angle matrix for all BEM surfaces.
     *
     * @param[in] surfs  Vector of BEM surfaces.
     * @return Solid-angle matrix, or empty matrix on error.
     */
    static Eigen::MatrixXf fwd_bem_solid_angles(const std::vector<MNELIB::MNESurface*>& surfs);

    //=========================================================================================================
    /**
     * @brief Compute the constant-collocation BEM solution for this model.
     *
     * @return OK on success, FAIL on error.
     */
    int fwd_bem_constant_collocation_solution();

    //============================= fwd_bem_model.c =============================

    //=========================================================================================================
    /**
     * @brief Compute the BEM solution matrix using the specified method.
     *
     * @param[in] bem_method  BEM method (FWD_BEM_CONSTANT_COLL or FWD_BEM_LINEAR_COLL).
     * @return OK on success, FAIL on error.
     */
    int fwd_bem_compute_solution(int bem_method);

    //=========================================================================================================
    /**
     * @brief Load a BEM solution from file, recomputing if necessary.
     *
     * Attempts to read the pre-computed solution; if it is missing or
     * force_recompute is set, computes a fresh solution.
     *
     * @param[in] name             Path to the BEM model file.
     * @param[in] bem_method       Required BEM method.
     * @param[in] force_recompute  If non-zero, always recompute the solution.
     * @return OK on success, FAIL on error.
     */
    int fwd_bem_load_recompute_solution(const QString& name,
                                        int bem_method,
                                        int force_recompute);

    //============================= fwd_bem_pot.c =============================

    //=========================================================================================================
    /**
     * @brief Compute the infinite-medium magnetic field at a single point.
     *
     * Returns the component of the magnetic field along the given direction,
     * without the mu_0 / (4 pi) prefactor.
     *
     * @param[in] rd   Dipole position.
     * @param[in] Q    Dipole moment.
     * @param[in] rp   Field point.
     * @param[in] dir  Field direction of interest (unit vector).
     * @return The infinite-medium magnetic field component.
     */
    static float fwd_bem_inf_field(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, const Eigen::Vector3f& rp, const Eigen::Vector3f& dir);

    //=========================================================================================================
    /**
     * @brief Compute the infinite-medium electric potential at a single point.
     *
     * Returns the potential without the 1 / (4 pi sigma) prefactor.
     *
     * @param[in] rd  Dipole position.
     * @param[in] Q   Dipole moment.
     * @param[in] rp  Potential evaluation point.
     * @return The infinite-medium electric potential.
     */
    static float fwd_bem_inf_pot(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, const Eigen::Vector3f& rp);

    //=========================================================================================================
    /**
     * @brief Precompute the electrode-specific BEM solution.
     *
     * Builds the coil-specific solution matrix for EEG electrodes by
     * multiplying the BEM solution with infinite-medium potentials.
     *
     * @param[in] els  Electrode (coil) set to prepare.
     * @return OK on success, FAIL on error.
     */
    int fwd_bem_specify_els(FwdCoilSet* els);

    //=========================================================================================================
    /**
     * @brief Compute the gradient of BEM potentials with respect to dipole position (constant collocation).
     *
     * @param[in]  rd        Dipole position.
     * @param[in]  Q         Dipole orientation.
     * @param[in]  els       Electrode set.
     * @param[in]  all_surfs If non-zero, compute on all surfaces.
     * @param[out] xgrad     Gradient with respect to x (one value per electrode).
     * @param[out] ygrad     Gradient with respect to y.
     * @param[out] zgrad     Gradient with respect to z.
     */
    void fwd_bem_pot_grad_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                               FwdCoilSet* els, int all_surfs,
                               Eigen::VectorXf& xgrad, Eigen::VectorXf& ygrad, Eigen::VectorXf& zgrad);

    //=========================================================================================================
    /**
     * @brief Compute BEM potentials at electrodes using linear collocation.
     *
     * @param[in]  rd        Dipole position.
     * @param[in]  Q         Dipole orientation.
     * @param[in]  els       Electrode set.
     * @param[in]  all_surfs If non-zero, compute on all surfaces.
     * @param[out] pot       Output potentials (one value per electrode).
     */
    void fwd_bem_lin_pot_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                              FwdCoilSet* els, int all_surfs,
                              Eigen::VectorXf& pot);

    //=========================================================================================================
    /**
     * @brief Compute the gradient of BEM potentials with respect to dipole position (linear collocation).
     *
     * @param[in]  rd        Dipole position.
     * @param[in]  Q         Dipole orientation.
     * @param[in]  els       Electrode set.
     * @param[in]  all_surfs If non-zero, compute on all surfaces.
     * @param[out] xgrad     Gradient with respect to x (one value per electrode).
     * @param[out] ygrad     Gradient with respect to y.
     * @param[out] zgrad     Gradient with respect to z.
     */
    void fwd_bem_lin_pot_grad_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                   FwdCoilSet* els, int all_surfs,
                                   Eigen::VectorXf& xgrad, Eigen::VectorXf& ygrad, Eigen::VectorXf& zgrad);

    //=========================================================================================================
    /**
     * @brief Compute BEM potentials at electrodes using constant collocation.
     *
     * @param[in]  rd        Dipole position.
     * @param[in]  Q         Dipole orientation.
     * @param[in]  els       Electrode set.
     * @param[in]  all_surfs If non-zero, compute on all surfaces.
     * @param[out] pot       Output potentials (one value per electrode).
     */
    void fwd_bem_pot_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                          FwdCoilSet* els, int all_surfs,
                          Eigen::VectorXf& pot);

    //=========================================================================================================
    /**
     * @brief Callback: compute BEM potentials at electrodes for a dipole.
     *
     * Matches the fwdFieldFunc signature for use in forward computation threads.
     *
     * @param[in]  rd      Dipole position (3-element array).
     * @param[in]  Q       Dipole orientation (3-element array).
     * @param[in]  els     Electrode descriptors.
     * @param[out] pot     Output potentials.
     * @param[in]  client  Opaque pointer to the FwdBemModel instance.
     * @return OK on success, FAIL on error.
     */
    static int fwd_bem_pot_els(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                               FwdCoilSet* els, float *pot,
                               void *client);

    //=========================================================================================================
    /**
     * @brief Callback: compute BEM potentials and position gradients at electrodes.
     *
     * Matches the fwdFieldGradFunc signature for use in forward computation threads.
     *
     * @param[in]  rd      Dipole position (3-element array).
     * @param[in]  Q       Dipole orientation (3-element array).
     * @param[in]  els     Electrode descriptors.
     * @param[out] pot     Output potentials.
     * @param[out] xgrad   Gradient with respect to x.
     * @param[out] ygrad   Gradient with respect to y.
     * @param[out] zgrad   Gradient with respect to z.
     * @param[in]  client  Opaque pointer to the FwdBemModel instance.
     * @return OK on success, FAIL on error.
     */
    static int fwd_bem_pot_grad_els(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                    FwdCoilSet* els, float *pot,
                                    float *xgrad, float *ygrad, float *zgrad,
                                    void *client);

    //============================= fwd_bem_field.c =============================

    /*
     * Integration formulas from:
     * L. Urankar, "Common compact analytical formulas for computation of
     * geometry integrals on a basic Cartesian sub-domain in boundary and
     * volume integral methods", Engineering Analysis with Boundary Elements,
     * 7(3), 1990, 124-129.
     */

    //=========================================================================================================
    /**
     * @brief Compute the f0, fx, fy integration helper values from corner coordinates.
     *
     * @param[in]  xx  Corner x-coordinates.
     * @param[in]  yy  Corner y-coordinates.
     * @param[out] f0  Integral f0.
     * @param[out] fx  Integral fx.
     * @param[out] fy  Integral fy.
     */
    static void calc_f(const Eigen::Vector3d& xx, const Eigen::Vector3d& yy,
                       Eigen::Vector3d& f0, Eigen::Vector3d& fx, Eigen::Vector3d& fy);

    //=========================================================================================================
    /**
     * @brief Compute the "magic" beta and D factors for the Urankar field integration.
     *
     * @param[in]  u     Coordinate u.
     * @param[in]  z     Coordinate z.
     * @param[in]  A     Parameter A.
     * @param[in]  B     Parameter B.
     * @param[out] beta  Output beta factor.
     * @param[out] D     Output D factor.
     */
    static void calc_magic(double u, double z,
                           double A, double B,
                           Eigen::Vector3d& beta, double& D);

    //=========================================================================================================
    /**
     * @brief Compute the geometry integrals for the magnetic field from a triangle.
     *
     * @param[in]  from  Source point.
     * @param[in]  to    Destination triangle.
     * @param[out] I1p   Monopolar integral.
     * @param[out] T     Integral T.
     * @param[out] S1    Integral S1.
     * @param[out] S2    Integral S2.
     * @param[out] f0    Integral f0.
     * @param[out] fx    Integral fx.
     * @param[out] fy    Integral fy.
     */
    static void field_integrals(const Eigen::Vector3f& from,
                                MNELIB::MNETriangle& to,
                                double& I1p,
                                Eigen::Vector2d& T, Eigen::Vector2d& S1, Eigen::Vector2d& S2,
                                Eigen::Vector3d& f0, Eigen::Vector3d& fx, Eigen::Vector3d& fy);

    //=========================================================================================================
    /**
     * @brief Compute the constant-collocation magnetic field coefficient for one triangle.
     *
     * @param[in] dest    Destination field point.
     * @param[in] normal  Field direction of interest (unit vector).
     * @param[in] tri     Source triangle.
     * @return The field coefficient.
     */
    static double one_field_coeff(const Eigen::Vector3f& dest, const Eigen::Vector3f& normal,
                                  MNELIB::MNETriangle& tri);

    //=========================================================================================================
    /**
     * @brief Assemble the constant-collocation magnetic field coefficient matrix.
     *
     * @param[in] coils  MEG coil set.
     * @return Coefficient matrix, or empty matrix on error.
     */
    Eigen::MatrixXf fwd_bem_field_coeff(FwdCoilSet* coils);

    /*
     * Linear field formulas from:
     * Ferguson et al., "A Complete Linear Discretization for Calculating
     * the Magnetic Field Using the Boundary-Element Method",
     * IEEE Trans. Biomed. Eng., submitted.
     */

    //=========================================================================================================
    /**
     * @brief Compute the gamma angle for the linear field integration (Ferguson).
     *
     * @param[in] rk   Position vector of vertex k.
     * @param[in] rk1  Position vector of vertex k+1.
     * @return The gamma angle value.
     */
    static double calc_gamma(const Eigen::Vector3d& rk, const Eigen::Vector3d& rk1);

    //=========================================================================================================
    /**
     * @brief Compute linear field coefficients using the Ferguson method.
     *
     * @param[in]  dest  Field point.
     * @param[in]  dir   Field direction of interest (unit vector).
     * @param[in]  tri   Destination triangle.
     * @param[out] res   Output coefficients for the three triangle vertices.
     */
    static void fwd_bem_one_lin_field_coeff_ferg(const Eigen::Vector3f& dest, const Eigen::Vector3f& dir,
                                                 MNELIB::MNETriangle& tri,
                                                 Eigen::Vector3d& res);

    //=========================================================================================================
    /**
     * @brief Compute linear field coefficients using the Urankar method.
     *
     * @param[in]  dest  Field point.
     * @param[in]  dir   Field direction of interest (unit vector).
     * @param[in]  tri   Destination triangle.
     * @param[out] res   Output coefficients for the three triangle vertices.
     */
    static void fwd_bem_one_lin_field_coeff_uran(const Eigen::Vector3f& dest, const Eigen::Vector3f& dir,
                                                 MNELIB::MNETriangle& tri,
                                                 Eigen::Vector3d& res);

    //=========================================================================================================
    /**
     * @brief Compute linear field coefficients using the simple (direct) method.
     *
     * @param[in]  dest    Destination field point.
     * @param[in]  normal  Field direction of interest (unit vector).
     * @param[in]  source  Source triangle.
     * @param[out] res     Output coefficients for the three triangle vertices.
     */
    static void fwd_bem_one_lin_field_coeff_simple(const Eigen::Vector3f& dest, const Eigen::Vector3f& normal,
                                                   MNELIB::MNETriangle& source,
                                                   Eigen::Vector3d& res);

    //=========================================================================================================
    /**
     * @brief Function pointer type for linear field coefficient integration methods.
     */
    typedef void (*linFieldIntFunc)(const Eigen::Vector3f& dest, const Eigen::Vector3f& dir,
                                    MNELIB::MNETriangle& tri, Eigen::Vector3d& res);

    //=========================================================================================================
    /**
     * @brief Assemble the linear-collocation magnetic field coefficient matrix.
     *
     * @param[in] coils   MEG coil set.
     * @param[in] method  Integration method (FWD_BEM_LIN_FIELD_SIMPLE, _FERGUSON, or _URANKAR).
     * @return Coefficient matrix, or empty matrix on error.
     */
    Eigen::MatrixXf fwd_bem_lin_field_coeff(FwdCoilSet* coils, int method);

    //=========================================================================================================
    /**
     * @brief Precompute the coil-specific BEM solution for MEG.
     *
     * Builds the coil-specific solution matrix by multiplying the BEM
     * solution with the field coefficients.
     *
     * @param[in] coils  MEG coil set to prepare.
     * @return OK on success, FAIL on error.
     */
    int fwd_bem_specify_coils(FwdCoilSet* coils);

    #define MAG_FACTOR 1e-7  /**< Magnetic constant mu_0 / (4 * pi). */

    //=========================================================================================================
    /**
     * @brief Compute BEM magnetic fields at coils using linear collocation.
     *
     * @param[in]  rd     Dipole position.
     * @param[in]  Q      Dipole orientation.
     * @param[in]  coils  MEG coil set.
     * @param[out] B      Output magnetic fields (one value per coil).
     */
    void fwd_bem_lin_field_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                FwdCoilSet& coils, Eigen::VectorXf& B);

    //=========================================================================================================
    /**
     * @brief Compute BEM magnetic fields at coils using constant collocation.
     *
     * @param[in]  rd     Dipole position.
     * @param[in]  Q      Dipole orientation.
     * @param[in]  coils  MEG coil set.
     * @param[out] B      Output magnetic fields (one value per coil).
     */
    void fwd_bem_field_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                            FwdCoilSet& coils, Eigen::VectorXf& B);

    //=========================================================================================================
    /**
     * @brief Compute the gradient of BEM magnetic fields with respect to dipole position (constant collocation).
     *
     * @param[in]  rd     Dipole position.
     * @param[in]  Q      Dipole orientation.
     * @param[in]  coils  MEG coil set.
     * @param[out] xgrad  Gradient with respect to x (one value per coil).
     * @param[out] ygrad  Gradient with respect to y.
     * @param[out] zgrad  Gradient with respect to z.
     */
    void fwd_bem_field_grad_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                 FwdCoilSet& coils,
                                 Eigen::VectorXf& xgrad, Eigen::VectorXf& ygrad, Eigen::VectorXf& zgrad);

    //=========================================================================================================
    /**
     * @brief Compute the derivative of the infinite-medium magnetic field with respect to dipole position.
     *
     * Returns the field derivative without the mu_0 / (4 pi) prefactor.
     *
     * @param[in] rd    Dipole position.
     * @param[in] Q     Dipole moment.
     * @param[in] rp    Field point.
     * @param[in] dir   Field direction of interest (unit vector).
     * @param[in] comp  Gradient component direction (unit vector).
     * @return The field derivative value.
     */
    static float fwd_bem_inf_field_der(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, const Eigen::Vector3f& rp,
                                       const Eigen::Vector3f& dir, const Eigen::Vector3f& comp);

    //=========================================================================================================
    /**
     * @brief Compute the derivative of the infinite-medium electric potential with respect to dipole position.
     *
     * @param[in] rd    Dipole position.
     * @param[in] Q     Dipole moment.
     * @param[in] rp    Potential evaluation point.
     * @param[in] comp  Gradient component direction (unit vector).
     * @return The potential derivative value.
     */
    static float fwd_bem_inf_pot_der(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, const Eigen::Vector3f& rp,
                                     const Eigen::Vector3f& comp);

    //=========================================================================================================
    /**
     * @brief Compute the gradient of BEM magnetic fields with respect to dipole position (linear collocation).
     *
     * @param[in]  rd     Dipole position.
     * @param[in]  Q      Dipole orientation.
     * @param[in]  coils  MEG coil set.
     * @param[out] xgrad  Gradient with respect to x (one value per coil).
     * @param[out] ygrad  Gradient with respect to y.
     * @param[out] zgrad  Gradient with respect to z.
     */
    void fwd_bem_lin_field_grad_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                     FwdCoilSet& coils,
                                     Eigen::VectorXf& xgrad, Eigen::VectorXf& ygrad, Eigen::VectorXf& zgrad);

    //=========================================================================================================
    /**
     * @brief Callback: compute BEM magnetic fields at coils for a dipole.
     *
     * Dispatches to fwd_bem_field_calc or fwd_bem_lin_field_calc based on
     * the current BEM method. Matches the fwdFieldFunc signature.
     *
     * @param[in]  rd      Dipole position (3-element array).
     * @param[in]  Q       Dipole orientation (3-element array).
     * @param[in]  coils   MEG coil descriptors.
     * @param[out] B       Output magnetic fields.
     * @param[in]  client  Opaque pointer to the FwdBemModel instance.
     * @return OK on success, FAIL on error.
     */
    static int fwd_bem_field(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                             FwdCoilSet* coils, float *B,
                             void *client);

    //=========================================================================================================
    /**
     * @brief Callback: compute BEM magnetic fields and position gradients at coils.
     *
     * Matches the fwdFieldGradFunc signature for use in forward computation threads.
     *
     * @param[in]  rd      Dipole position (3-element array).
     * @param[in]  Q       Dipole orientation (3-element array).
     * @param[in]  coils   MEG coil definitions.
     * @param[out] Bval    Output magnetic fields.
     * @param[out] xgrad   Gradient with respect to x.
     * @param[out] ygrad   Gradient with respect to y.
     * @param[out] zgrad   Gradient with respect to z.
     * @param[in]  client  Opaque pointer to the FwdBemModel instance.
     * @return OK on success, FAIL on error.
     */
    static int fwd_bem_field_grad(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                  FwdCoilSet* coils, float Bval[],
                                  float xgrad[], float ygrad[], float zgrad[],
                                  void *client);

    //============================= compute_forward.c =============================

    //=========================================================================================================
    /**
     * @brief Thread worker: compute the forward solution for one source space.
     *
     * Used as a thread entry point; the argument is cast to the appropriate
     * worker struct internally.
     *
     * @param[in] arg  Per-thread work descriptor.
     */
    static void meg_eeg_fwd_one_source_space(FwdThreadArg* arg);

    //=========================================================================================================
    /**
     * @brief Compute the MEG forward solution for one or more source spaces.
     *
     * @param[in]     spaces      Source spaces.
     * @param[in]     coils       MEG coil set.
     * @param[in]     comp_coils  Compensator coil set (may be nullptr).
     * @param[in]     comp_data   CTF compensation data (may be nullptr).
     * @param[in]     fixed_ori   If true, use fixed-orientation dipoles.
     * @param[in]     r0          Sphere model origin.
     * @param[in]     use_threads If true, parallelize across source spaces.
     * @param[out]    resp        Forward solution matrix.
     * @param[out]    resp_grad   Gradient forward solution matrix.
     * @param[in]     bDoGRad     If true, also compute the gradient solution.
     * @return OK on success, FAIL on error.
     */
    int compute_forward_meg(std::vector<std::unique_ptr<MNELIB::MNESourceSpace>>& spaces,
                            FwdCoilSet*                 coils,
                            FwdCoilSet*                 comp_coils,
                            MNELIB::MNECTFCompDataSet*  comp_data,
                            bool                        fixed_ori,
                            const Eigen::Vector3f&      r0,
                            bool                        use_threads,
                            FIFFLIB::FiffNamedMatrix&   resp,
                            FIFFLIB::FiffNamedMatrix&   resp_grad,
                            bool                        bDoGRad);

    //=========================================================================================================
    /**
     * @brief Compute the EEG forward solution for one or more source spaces.
     *
     * @param[in]     spaces      Source spaces.
     * @param[in]     els         Electrode locations.
     * @param[in]     fixed_ori   If true, use fixed-orientation dipoles.
     * @param[in]     eeg_model   Sphere model definition.
     * @param[in]     use_threads If true, parallelize across source spaces.
     * @param[out]    resp        Forward solution matrix.
     * @param[out]    resp_grad   Gradient forward solution matrix.
     * @param[in]     bDoGrad     If true, also compute the gradient solution.
     * @return OK on success, FAIL on error.
     */
    int compute_forward_eeg(std::vector<std::unique_ptr<MNELIB::MNESourceSpace>>& spaces,
                            FwdCoilSet*                 els,
                            bool                        fixed_ori,
                            FwdEegSphereModel*          eeg_model,
                            bool                        use_threads,
                            FIFFLIB::FiffNamedMatrix&   resp,
                            FIFFLIB::FiffNamedMatrix&   resp_grad,
                            bool                        bDoGrad);

    //============================= fwd_spherefield.c =============================

    //=========================================================================================================
    /**
     * @brief Callback: compute the spherical-model magnetic field at coils.
     *
     * Matches the fwdFieldFunc signature.
     *
     * @param[in]  rd      Dipole position (3-element array).
     * @param[in]  Q       Dipole components (xyz).
     * @param[in]  coils   MEG coil definitions.
     * @param[out] Bval    Output magnetic fields.
     * @param[in]  client  Opaque pointer to client data (sphere model origin).
     * @return OK on success, FAIL on error.
     */
    static int fwd_sphere_field(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                FwdCoilSet* coils, float Bval[],
                                void *client);

    //=========================================================================================================
    /**
     * @brief Callback: compute the spherical-model vector magnetic field at coils.
     *
     * Returns one row per cardinal dipole direction (x, y, z).
     * Matches the fwdVecFieldFunc signature.
     *
     * @param[in]  rd      Dipole position (3-element array).
     * @param[in]  coils   MEG coil definitions.
     * @param[out] Bval    Output fields (3 x ncoil matrix, row-major).
     * @param[in]  client  Opaque pointer to client data.
     * @return OK on success, FAIL on error.
     */
    static int fwd_sphere_field_vec(const Eigen::Vector3f& rd,
                                    FwdCoilSet* coils, float **Bval,
                                    void *client);

    //=========================================================================================================
    /**
     * @brief Callback: compute the spherical-model magnetic field and its position gradient at coils.
     *
     * Matches the fwdFieldGradFunc signature.
     *
     * @param[in]  rd      Dipole position (3-element array).
     * @param[in]  Q       Dipole components (xyz).
     * @param[in]  coils   MEG coil definitions.
     * @param[out] Bval    Output magnetic fields.
     * @param[out] xgrad   Gradient with respect to x.
     * @param[out] ygrad   Gradient with respect to y.
     * @param[out] zgrad   Gradient with respect to z.
     * @param[in]  client  Opaque pointer to client data.
     * @return OK on success, FAIL on error.
     */
    static int fwd_sphere_field_grad(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                     FwdCoilSet* coils, float Bval[],
                                     float xgrad[], float ygrad[], float zgrad[],
                                     void *client);

    //============================= fwd_mag_dipole_field.c =============================

    //=========================================================================================================
    /**
     * @brief Callback: compute the magnetic field of a magnetic dipole at coils.
     *
     * @param[in]  rm      Dipole position (in the same coordinate system as the coils).
     * @param[in]  M       Dipole moment components (xyz).
     * @param[in]  coils   MEG coil definitions.
     * @param[out] Bval    Output magnetic fields.
     * @param[in]  client  Opaque pointer to client data (unused, may be nullptr).
     * @return OK on success, FAIL on error.
     */
    static int fwd_mag_dipole_field(const Eigen::Vector3f& rm, const Eigen::Vector3f& M,
                                    FwdCoilSet* coils, float Bval[],
                                    void *client);

    //=========================================================================================================
    /**
     * @brief Callback: compute the vector magnetic field of a magnetic dipole at coils.
     *
     * Returns one row per cardinal dipole direction (x, y, z).
     *
     * @param[in]  rm      Dipole position (3-element array).
     * @param[in]  coils   MEG coil definitions.
     * @param[out] Bval    Output fields (3 x ncoil matrix, row-major).
     * @param[in]  client  Opaque pointer to client data (unused, may be nullptr).
     * @return OK on success, FAIL on error.
     */
    static int fwd_mag_dipole_field_vec(const Eigen::Vector3f& rm,
                                        FwdCoilSet* coils, float **Bval,
                                        void *client);

public:
    QString     surf_name;              /**< File from which surfaces were loaded. */

    std::vector<std::shared_ptr<MNELIB::MNESurface>> surfs;  /**< Interface surfaces, outermost first. */

    Eigen::VectorXi ntri;               /**< Triangle count per surface (length nsurf). */
    Eigen::VectorXi np;                 /**< Vertex count per surface (length nsurf). */
    int             nsurf;              /**< Number of interface surfaces. */

    Eigen::VectorXf sigma;              /**< Conductivity of each layer (length nsurf). */
    Eigen::MatrixXf gamma;              /**< Conductivity-ratio coupling matrix (nsurf x nsurf). */
    Eigen::VectorXf source_mult;        /**< Infinite-medium potential multipliers (length nsurf). */
    Eigen::VectorXf field_mult;         /**< Magnetic-field multipliers (length nsurf). */

    int             bem_method;         /**< Approximation method (FWD_BEM_CONSTANT_COLL or FWD_BEM_LINEAR_COLL). */
    QString         sol_name;           /**< File from which the solution was loaded. */

    Eigen::MatrixXf solution;           /**< Potential solution matrix (nsol x nsol). */
    Eigen::VectorXf v0;                 /**< Workspace for infinite-medium potentials (length nsol). */
    int             nsol;               /**< Dimension of the solution matrix. */

    FIFFLIB::FiffCoordTrans head_mri_t; /**< Head-to-MRI coordinate transform. */

    float           ip_approach_limit;  /**< Threshold for isolated-problem approach. */
    bool            use_ip_approach;    /**< Whether the isolated-problem approach is active. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_BEM_MODEL_H
