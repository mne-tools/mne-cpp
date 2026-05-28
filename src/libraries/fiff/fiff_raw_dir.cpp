//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_raw_dir.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffRawDir: per-FIFF_DATA_BUFFER directory entry used by random-access raw reads.
 *
 * Built once by @ref FiffStream during open and indexed by sample
 * position thereafter.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_raw_dir.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawDir::FiffRawDir()
: first(-1)
, last(-1)
, nsamp(-1)
{
}

//=============================================================================================================

FiffRawDir::FiffRawDir(const FiffRawDir &p_FiffRawDir)
: ent(p_FiffRawDir.ent)
, first(p_FiffRawDir.first)
, last(p_FiffRawDir.last)
, nsamp(p_FiffRawDir.nsamp)
{
}

//=============================================================================================================

FiffRawDir::~FiffRawDir()
{
}
