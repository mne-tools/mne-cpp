//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_coord_trans_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref FiffCoordTransSet: container management plus lookup of transforms by (from, to) frame pair.
 *
 * Used by @ref FiffStream::read_meas_info to return every device→head /
 * head→MRI / MRI→RAS transform discovered in a measurement file, and
 * to persist coregistered ``-trans.fif'' files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_coord_trans_set.h"
#include "fiff_coord_trans.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCoordTransSet::FiffCoordTransSet()
{
}

//=============================================================================================================

FiffCoordTransSet::~FiffCoordTransSet()
{
}

