//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fs_colortable.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     December 2012
 * @brief    Implementation of @ref mne::FsColortable: in-memory FreeSurferColorLUT-style colour and structure lookup table.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_colortable.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
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
