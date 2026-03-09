//=============================================================================================================
/**
 * @file     mne_mgh_tag_group.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MNEMghTagGroup class declaration.
 *
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
