//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fwd_bem_solution.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Per-sensor projection matrix that turns BEM node potentials into MEG coil readings or EEG electrode voltages.
 *
 * Solving a BEM model produces a node-potential vector @c v of length
 * @c np (one entry per surface vertex). For a fixed sensor array that
 * vector is contracted with the @c ncoil × @c np matrix @c solution to
 * yield the final lead-field column; the matrix encodes the Geselowitz
 * surface-integral weights for MEG coils, or the trilinear-interpolation
 * coefficients of the electrode position on the scalp triangulation for
 * EEG. Caching this projection per channel set lets the dipole loop
 * skip the full BEM forward substitution for every dipole and reduces
 * the marginal cost of one source position to a single dense
 * matrix-vector multiply.
 *
 * Refactored from the @c fwdBemSolutionRec struct of MNE-C
 * @c fwd_types.h.
 */

#ifndef FWD_BEM_SOLUTION_H
#define FWD_BEM_SOLUTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"

#include <memory>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * Implements a Forward BEM Solution (replaces @c fwdBemSolution / @c fwdBemSolutionRec from MNE-C @c fwd_types.h).
 *
 * @brief Channel-specific projection that contracts a BEM node-potential vector down to one entry per MEG coil or EEG electrode — i.e. the Geselowitz surface-integral weights cached as a dense @c ncoil × @c np matrix.
 */
class FWDSHARED_EXPORT FwdBemSolution
{
public:
    typedef std::unique_ptr<FwdBemSolution> UPtr;         /**< Unique pointer type for FwdBemSolution. */

    //=========================================================================================================
    /**
     * Constructs the Forward BEM Solution
     */
    FwdBemSolution();

    //=========================================================================================================
    /**
     * Destroys the Forward BEM Solution
     */
    ~FwdBemSolution();

public:
    Eigen::MatrixXf solution;          /**< Solution matrix (ncoil x np). */
    int   ncoil;                        /**< Number of sensors. */
    int   np;                           /**< Number of potential solution points. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_BEM_SOLUTION_H
