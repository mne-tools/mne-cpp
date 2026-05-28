//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     surface_laplacian.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Spherical-spline surface Laplacian (Current Source Density) for EEG.
 *
 * The surface Laplacian ∇²V approximates the radial current density at
 * the scalp and acts as a reference-free spatial high-pass filter:
 * it sharpens focal cortical sources, removes the smearing introduced by
 * volume conduction, and is largely insensitive to the choice of recording
 * reference. The implementation follows the spherical-spline interpolation
 * scheme of Perrin et al. (Electroenceph. Clin. Neurophysiol. 72, 1989)
 * with the order-@c m Legendre series of Perrin & Bertrand (1990), so the
 * full pipeline reduces to two dense matrix multiplications and a single
 * channel-rank inversion.
 *
 * Smoothness is governed by the spline order @c m (typically 3 or 4) and
 * the diagonal regularisation λ; flexibility is the same as MNE-Python's
 * @c mne.preprocessing.compute_current_source_density.
 */

#ifndef SURFACE_LAPLACIAN_H
#define SURFACE_LAPLACIAN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Result of a surface Laplacian (CSD) computation.
 */
struct DSPSHARED_EXPORT SurfaceLaplacianResult
{
    Eigen::MatrixXd matData;          ///< Transformed data (n_eeg_channels × n_times)
    Eigen::MatrixXd matTransform;     ///< CSD transformation matrix (n_eeg × n_eeg)
};

//=============================================================================================================
/**
 * @brief Surface Laplacian / Current Source Density (CSD) transformation.
 *
 * Computes a reference-free spatial Laplacian of EEG data using spherical
 * spline interpolation (Perrin et al. 1989). This improves spatial resolution
 * by acting as a high-pass spatial filter that enhances local sources and
 * attenuates volume-conducted far-field activity.
 *
 * @code
 *   Eigen::MatrixX3d pos = ...;  // EEG electrode positions (n_ch × 3)
 *   Eigen::MatrixXd  data = ...; // EEG data (n_ch × n_times)
 *   auto result = SurfaceLaplacian::compute(data, pos);
 *   // result.matData      → CSD-transformed data
 *   // result.matTransform → n_ch × n_ch transformation matrix
 * @endcode
 */
class DSPSHARED_EXPORT SurfaceLaplacian
{
public:
    //=========================================================================================================
    /**
     * Compute the surface Laplacian (CSD) transformation.
     *
     * @param[in] matData           EEG data matrix (n_channels × n_times).
     * @param[in] matPositions      Electrode positions in 3D (n_channels × 3), in head coordinates.
     * @param[in] dLambda2          Regularization parameter (default 1e-5).
     * @param[in] iStiffness        Spline stiffness parameter (default 4).
     * @param[in] iNLegendreTerms   Number of Legendre terms to evaluate (default 50).
     * @param[in] dSphereRadius     Sphere radius in metres; if <= 0, fitted from positions (default -1).
     * @return                      SurfaceLaplacianResult with transformed data and transform matrix.
     */
    static SurfaceLaplacianResult compute(const Eigen::MatrixXd& matData,
                                          const Eigen::MatrixX3d& matPositions,
                                          double dLambda2 = 1e-5,
                                          int iStiffness = 4,
                                          int iNLegendreTerms = 50,
                                          double dSphereRadius = -1.0);

    //=========================================================================================================
    /**
     * Compute only the CSD transformation matrix (without applying to data).
     *
     * @param[in] matPositions      Electrode positions in 3D (n_channels × 3).
     * @param[in] dLambda2          Regularization parameter (default 1e-5).
     * @param[in] iStiffness        Spline stiffness parameter (default 4).
     * @param[in] iNLegendreTerms   Number of Legendre terms (default 50).
     * @param[in] dSphereRadius     Sphere radius; if <= 0, fitted from positions.
     * @return                      Transformation matrix (n_channels × n_channels).
     */
    static Eigen::MatrixXd computeTransform(const Eigen::MatrixX3d& matPositions,
                                            double dLambda2 = 1e-5,
                                            int iStiffness = 4,
                                            int iNLegendreTerms = 50,
                                            double dSphereRadius = -1.0);

private:
    /**
     * Compute the G matrix (spherical spline interpolation kernel).
     */
    static Eigen::MatrixXd computeG(const Eigen::MatrixXd& matCosAng,
                                    int iStiffness,
                                    int iNLegendreTerms);

    /**
     * Compute the H matrix (Laplacian kernel).
     */
    static Eigen::MatrixXd computeH(const Eigen::MatrixXd& matCosAng,
                                    int iStiffness,
                                    int iNLegendreTerms);

    /**
     * Evaluate Legendre polynomials up to a given order using Bonnet's recurrence.
     */
    static Eigen::MatrixXd evaluateLegendre(const Eigen::MatrixXd& matX, int iMaxOrder);
};

} // namespace UTILSLIB

#endif // SURFACE_LAPLACIAN_H
