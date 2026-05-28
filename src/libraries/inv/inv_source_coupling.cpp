//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_source_coupling.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Definition of the @ref INVLIB::InvSourceCoupling default constructor.
 *
 * Initialises the empty grid-index and moment vectors plus the time
 * window to zero, so an @ref InvSourceCoupling produced by aggregate
 * initialisation has well-defined defaults for the fields that
 * downstream code reads unconditionally.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_source_coupling.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvSourceCoupling::InvSourceCoupling()
    : tmin(0.0f)
    , tmax(0.0f)
{
}
