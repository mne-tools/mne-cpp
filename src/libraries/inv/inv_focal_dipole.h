//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_focal_dipole.h
 * @since March 2026
 * @brief Single off-grid focal dipole result with free position, moment and fit-quality metrics.
 *
 * @ref INVLIB::InvFocalDipole captures one equivalent current dipole that
 * is not constrained to a source-space vertex — typically the output of
 * the ECD dipole-fit pipeline or of a max-power beamformer. Position is in
 * head-coordinate metres, moment is in Am, and the goodness, chi-squared
 * and degrees-of-freedom fields propagate fit diagnostics to downstream
 * viewers and report writers. A list of @ref InvFocalDipole instances is
 * attached to @ref InvSourceEstimate to carry sparse focal results alongside
 * dense grid estimates.
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
