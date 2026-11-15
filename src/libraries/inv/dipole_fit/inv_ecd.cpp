//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_ecd.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::InvEcd (default/copy ctors, destructor, debug print).
 *
 * Provides the field initialisers and the human-readable debug print
 * helper used by the @c mne_dipole_fit driver to dump per-time-point
 * fit results to the console while a run is in progress.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_ecd.h"
#include <base/log.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvEcd::InvEcd()
: valid(false)
, time(-1)
, rd(Vector3f::Zero(3))
, Q(Vector3f::Zero(3))
, good(0)
, khi2(0)
, nfree(0)
, neval(-1)
{
}

//=============================================================================================================

InvEcd::InvEcd(const InvEcd& p_ECD)
: valid(p_ECD.valid)
, time(p_ECD.time)
, rd(p_ECD.rd)
, Q(p_ECD.Q)
, good(p_ECD.good)
, khi2(p_ECD.khi2)
, nfree(p_ECD.nfree)
, neval(p_ECD.neval)
{
}

//=============================================================================================================

InvEcd::~InvEcd()
{
}

//=============================================================================================================

void InvEcd::print() const
{
    if (!this->valid)
        return;

    // Time, location, moment magnitude and components, reduced khi^2, goodness of fit, function evaluations
    mne::Log::info("{:6.1f} {:7.2f} {:7.2f} {:7.2f} {:6.2f} {:6.2f} {:6.2f} {:6.2f} {:6.2f} {:6.2f} {}",
                   1000 * this->time,
                   1000 * this->rd[0],
                   1000 * this->rd[1],
                   1000 * this->rd[2],
                   1e9 * this->Q.norm(),
                   1e9 * this->Q[0], 1e9 * this->Q[1], 1e9 * this->Q[2],
                   this->khi2 / this->nfree,
                   100 * this->good,
                   this->neval);
}
