//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_mgh_tag_group.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Ordered group of @ref mne::MNEMghTag entries appended to an MGH/MGZ file.
 *
 * @ref mne::MNEMghTagGroup gathers every tag present at the tail of
 * an MGH volume and offers typed accessors for the commonly used kinds
 * (@c MGH_TAG_OLD_COLORTABLE, @c MGH_TAG_MGH_XFORM,
 * @c MGH_TAG_CMDLINE, ...) so callers do not have to walk the raw byte
 * stream.
 */

#ifndef MNEMGHTAGGROUP_H
#define MNEMGHTAGGROUP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_mgh_tag.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Collection of MNEMghTag entries from a FreeSurfer MGH/MGZ file footer.
 *
 * Holds its tags by value.
 */
class MNESHARED_EXPORT MNEMghTagGroup
{
public:
    //=========================================================================================================
    /**
     * Constructs an empty MNEMghTagGroup.
     */
    MNEMghTagGroup() = default;

public:
    std::vector<MNEMghTag> tags; /**< The tags in file order. */

    /**
     * @brief Returns the number of tags in the group.
     *
     * @return Number of owned tag entries.
     */
    int ntags() const
    {
        return static_cast<int>(tags.size());
    }
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEMGHTAGGROUP_H
