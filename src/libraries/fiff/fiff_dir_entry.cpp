//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiff_dir_entry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     January 2013
 * @brief    Implementation of @ref FiffDirEntry: streaming of the 16-byte (kind, type, size, pos) record found in the FIFF_DIR tag.
 *
 * Pure data-record marshalling. The arrays of these records build the
 * random-access FIFF directory consumed by @ref FiffStream and
 * @ref FiffDirNode.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dir_entry.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDirEntry::FiffDirEntry()
: kind(-1)
, type(-1)
, size(-1)
, pos(-1)
{
}

//=============================================================================================================

FiffDirEntry::FiffDirEntry(const FiffDirEntry& p_FiffDirEntry)
: kind(p_FiffDirEntry.kind)
, type(p_FiffDirEntry.type)
, size(p_FiffDirEntry.size)
, pos(p_FiffDirEntry.pos)
{
}

//=============================================================================================================

FiffDirEntry::~FiffDirEntry()
{
}
