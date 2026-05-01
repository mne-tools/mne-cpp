//=============================================================================================================
/**
 * @file     surface_laplacian.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    SurfaceLaplacian class declaration — Current Source Density (CSD) via spherical spline.
 *
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
