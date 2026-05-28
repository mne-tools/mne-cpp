//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sphara.h
 * @since March 2026
 * @brief SPatial HARmonic Analysis (SPHARA) spatial-filter projector assembly.
 *
 * SPHARA represents a discrete EEG / MEG sensor montage as a triangulated
 * manifold and decomposes its spatial degrees of freedom into the eigen-
 * functions of the discrete Laplace–Beltrami operator on that mesh. Truncating
 * the expansion at the lowest @c iNBaseFct harmonics suppresses high spatial
 * frequencies that are dominated by sensor noise and reference artefacts,
 * giving a denoising / smoothing projector that is the spatial-domain analogue
 * of a low-pass filter.
 *
 * @ref makeSpharaProjector assembles the dense @c (iOperatorDim × iOperatorDim)
 * projection matrix from a stack of pre-computed basis functions. The
 * @c vecIndices argument maps the rows of the basis-function matrix back to
 * channel indices in the full data matrix, and @c iSkip lets a single basis
 * set be reused for interleaved sensor triplets such as the Neuromag /
 * MEGIN VectorView gradiometer pairs.
 *
 * Reference: Graichen U. et al., NeuroImage 86 (2014) 467–478.
 */

#ifndef SPHARA_UTILS_H
#define SPHARA_UTILS_H

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
// DEFINES
//=============================================================================================================

//=========================================================================================================
/**
 * Constructs a SPHARA operator.
 *
 * @param[in] matBaseFct        The SPHARA basis functions.
 * @param[in] vecIndices        The indices of the positions in the final oeprator which are to be filled with the basis functions weights (i.e. these indices could respond to the indices of gradioemteres in a VectorView system).
 * @param[in] iOperatorDim      The dimensions of the final SPHARA operator. Make sure that these correspond to the dimensions of the data matrix you want tol multiply with the SPHARA operator.
 * @param[in] iNBaseFct         The number of SPHARA basis functions to take.
 * @param[in] iSkip             The value to skip when reading the vecIndices variabel. I.e. use this when dealing with VectorView triplets, which include two gradiometers.
 *
 * @return Returns the final SPHARA operator with dimensions (iOperatorDim,iOperatorDim).
 */
DSPSHARED_EXPORT Eigen::MatrixXd makeSpharaProjector(const Eigen::MatrixXd& matBaseFct,
                                                      const Eigen::VectorXi& vecIndices,
                                                      int iOperatorDim,
                                                      int iNBaseFct,
                                                      int iSkip = 0);

} // NAMESPACE UTILSLIB

#endif // SPHARA_UTILS_H
