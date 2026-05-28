//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_mgh_tag.h
 * @since March 2026
 * @brief Single FreeSurfer MGH/MGZ tag (key, length, opaque payload).
 *
 * @ref MNELIB::MNEMghTag mirrors the trailing tag stream of an MGH file
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
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Single tag entry in a FreeSurfer MGH/MGZ file header.
 *
 * MGH files can carry optional typed metadata tags in the file footer.
 * Each tag has an integer identifier, a byte length, and a raw payload.
 */
class MNESHARED_EXPORT MNEMghTag
{
public:
    typedef QSharedPointer<MNEMghTag> SPtr;              /**< Shared pointer type for MNEMghTag. */
    typedef QSharedPointer<const MNEMghTag> ConstSPtr;   /**< Const shared pointer type for MNEMghTag. */

    //=========================================================================================================
    /**
     * Constructs an empty MNEMghTag.
     */
    MNEMghTag() = default;

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNEMghTag() = default;

public:
    int        tag = 0;            /**< Tag identifier code. */
    long long  len = 0;            /**< Byte length of the tag data payload. */
    QByteArray data;               /**< Raw tag data payload. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMGHTAG_H
