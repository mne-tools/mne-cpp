//=============================================================================================================
/**
 * @file     inv_focal_dipole.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2025
 *
 * @section  LICENSE
 *
 * Copyright (C) 2025, Christoph Dinh. All rights reserved.
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
 * @brief    InvFocalDipole struct declaration for off-grid dipole results (e.g. ECD dipole fit).
 *
 */

#ifndef INV_FOCAL_DIPOLE_H
#define INV_FOCAL_DIPOLE_H

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Represents a single focal (off-grid) current dipole, typically the result of an equivalent current dipole
 * (ECD) fit. The dipole has a free 3D position (not constrained to a source-space grid), a current moment,
 * and associated fit quality metrics.
 *
 * @brief Single focal dipole with free 3D position, moment, and fit-quality metrics.
 */
struct InvFocalDipole
{
    Eigen::Vector3f position;   /**< Dipole position (m) in head coordinates. */
    Eigen::Vector3f moment;     /**< Current dipole moment (Am). */
    int   gridIndex;            /**< Nearest grid index (-1 if truly off-grid). */
    float goodness;             /**< Goodness-of-fit (0..1). */
    float khi2;                 /**< Chi-squared value of the fit. */
    int   nfree;                /**< Degrees of freedom. */
    bool  valid;                /**< Whether this dipole passed validity checks. */
    float tmin;                 /**< Start of the time window (s) this dipole represents. */
    float tmax;                 /**< End of the time window (s) this dipole represents. */

    InvFocalDipole()
        : position(Eigen::Vector3f::Zero())
        , moment(Eigen::Vector3f::Zero())
        , gridIndex(-1)
        , goodness(0.0f)
        , khi2(0.0f)
        , nfree(0)
        , valid(false)
        , tmin(0.0f)
        , tmax(0.0f)
    {
    }
};

} // NAMESPACE INVLIB

#endif // INV_FOCAL_DIPOLE_H
