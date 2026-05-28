//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_beamformer.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of the @ref INVLIB::InvBeamformer container.
 *
 * Implements the default constructor and the small bookkeeping methods.
 * All non-inline accessors return shape information derived from the
 * held weight matrices; the heavy numerical work lives in
 * @ref InvBeamformerCompute and the @c make / @c apply members of
 * @ref InvLCMV and @ref InvDICS.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_beamformer.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvBeamformer::InvBeamformer()
: isFreOri(false)
, nSourcesTotal(0)
, weightNorm(BeamformerWeightNorm::None)
, pickOri(BeamformerPickOri::None)
, inversion(BeamformerInversion::Matrix)
, reg(0.05)
, rank(0)
{
}
