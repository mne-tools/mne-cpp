//=============================================================================================================
/**
 * @file     mne_event_list.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    MNEEventList class declaration.
 *
 */

#ifndef MNE_EVENT_LIST_H
#define MNE_EVENT_LIST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_event.h"

#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Ordered list of trigger-event markers.
 *
 * Owns its MNEEvent objects via unique_ptr.
 */
class MNESHARED_EXPORT MNEEventList
{
public:
    MNEEventList() = default;
    ~MNEEventList() = default;

    MNEEventList(const MNEEventList& other)
    {
        events.reserve(other.events.size());
        for (const auto& e : other.events)
            events.push_back(std::make_unique<MNEEvent>(*e));
    }

    MNEEventList& operator=(const MNEEventList& other)
    {
        if (this != &other) {
            events.clear();
            events.reserve(other.events.size());
            for (const auto& e : other.events)
                events.push_back(std::make_unique<MNEEvent>(*e));
        }
        return *this;
    }

    MNEEventList(MNEEventList&&) = default;
    MNEEventList& operator=(MNEEventList&&) = default;

    //=========================================================================================================
    /**
     * @brief Returns the number of events in the list.
     */
    int nevent() const { return static_cast<int>(events.size()); }

    std::vector<std::unique_ptr<MNEEvent>> events; /**< Owned event pointers. */
};

} // namespace MNELIB

#endif // MNE_EVENT_LIST_H
