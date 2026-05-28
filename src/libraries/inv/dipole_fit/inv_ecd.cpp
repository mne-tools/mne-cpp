//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_ecd.cpp
 * @since March 2026
 * @brief Implementation of @ref INVLIB::InvEcd (default/copy ctors, destructor, debug print).
 *
 * Provides the field initialisers and the human-readable debug print
 * helper used by the @c mne_dipole_fit driver to dump per-time-point
 * fit results to the console while a run is in progress.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_ecd.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVLIB;

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

    qInfo("%6.1f %7.2f %7.2f %7.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %d",
          1000*this->time,                              /* Time */
          1000*this->rd[0],                             /* Dipole location */
          1000*this->rd[1],
          1000*this->rd[2],
          1e9*this->Q.norm(),                           /* Dipole moment */
          1e9*this->Q[0],1e9*this->Q[1],1e9*this->Q[2],
          this->khi2/this->nfree,                       /* This is the reduced khi^2 value */
          100*this->good,                               /* Goodness of fit */
          this->neval);                                 /* Number of function evaluations required */
}
