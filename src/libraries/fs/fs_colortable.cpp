//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fs_colortable.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of @ref FSLIB::FsColortable: in-memory FreeSurferColorLUT-style colour and structure lookup table.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_colortable.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsColortable::FsColortable()
: orig_tab("")
, numEntries(0)
{
}

//=============================================================================================================

void FsColortable::clear()
{
    orig_tab = "";
    numEntries = 0;
    struct_names.clear();
    table = MatrixXi();
}
