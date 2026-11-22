//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_mgh_tag.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     April 2017
 * @brief    Single FreeSurfer MGH/MGZ tag (key, length, opaque payload).
 *
 * @ref mne::MNEMghTag mirrors the trailing tag stream of an MGH file
 * (volume geometry, TR, FOV, command-line history, talairach transform
 * and so on). Each tag is a 32-bit kind, a 64-bit length and an opaque
 * byte payload that the consumer decodes on demand.
 */

#ifndef MNEMGHTAG_H
#define MNEMGHTAG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Single tag entry in a FreeSurfer MGH/MGZ file header.
 *
 * MGH files can carry optional typed metadata tags in the file footer.
 * Each tag has an integer identifier, a byte length, and a raw payload.
 */
struct MNEMghTag
{
    int tag = 0;                 /**< Tag identifier code. */
    long long len = 0;           /**< Byte length of the tag data payload. */
    std::vector<std::byte> data; /**< Raw tag data payload. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEMGHTAG_H
