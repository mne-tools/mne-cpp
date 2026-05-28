//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_mgh_tag_group.h
 * @since March 2026
 * @brief Ordered group of @ref MNELIB::MNEMghTag entries appended to an MGH/MGZ file.
 *
 * @ref MNELIB::MNEMghTagGroup gathers every tag present at the tail of
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

#include <memory>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Collection of MNEMghTag entries from a FreeSurfer MGH/MGZ file footer.
 *
 * Owns its tags via unique_ptr.
 */
class MNESHARED_EXPORT MNEMghTagGroup
{
public:
    typedef QSharedPointer<MNEMghTagGroup> SPtr;              /**< Shared pointer type for MNEMghTagGroup. */
    typedef QSharedPointer<const MNEMghTagGroup> ConstSPtr;   /**< Const shared pointer type for MNEMghTagGroup. */

    //=========================================================================================================
    /**
     * Constructs an empty MNEMghTagGroup.
     */
    MNEMghTagGroup() = default;

    MNEMghTagGroup(const MNEMghTagGroup& other)
    {
        tags.reserve(other.tags.size());
        for (const auto& t : other.tags)
            tags.push_back(std::make_unique<MNEMghTag>(*t));
    }

    MNEMghTagGroup& operator=(const MNEMghTagGroup& other)
    {
        if (this != &other) {
            tags.clear();
            tags.reserve(other.tags.size());
            for (const auto& t : other.tags)
                tags.push_back(std::make_unique<MNEMghTag>(*t));
        }
        return *this;
    }

    MNEMghTagGroup(MNEMghTagGroup&&) = default;
    MNEMghTagGroup& operator=(MNEMghTagGroup&&) = default;

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNEMghTagGroup() = default;

public:
    std::vector<std::unique_ptr<MNEMghTag>> tags; /**< Owned tag entries. */

    /**
     * @brief Returns the number of tags in the group.
     */
    int ntags() const { return static_cast<int>(tags.size()); }
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMGHTAGGROUP_H
