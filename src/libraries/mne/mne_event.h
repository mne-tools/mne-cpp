//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_event.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Single recorded event (sample, previous and new trigger code, comment).
 *
 * @ref MNELIB::MNEEvent is one entry of an @ref MNELIB::MNEEventList, holding the
 * sample relative to the start of the data, the trigger value before and
 * after the transition (the triplet of @c -eve.fif files) plus the comment
 * and display state MNE-C keeps for each event.
 */

#ifndef MNE_EVENT_H
#define MNE_EVENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Single trigger-event marker.
 *
 * Records a stimulus transition (from one value to another) at a given
 * sample position, together with an optional free-text comment.
 *
 * @snippet ex_mne_api/main.cpp mne_event_list_usage
 */
class MNESHARED_EXPORT MNEEvent
{
public:
    int sample = 0;            /**< Sample number. */
    unsigned int from = 0;     /**< Trigger value before the transition. */
    unsigned int to = 0;       /**< Trigger value after the transition. */
    bool show = false;         /**< Display flag (application-defined). */
    bool created_here = false; /**< True if the event was added by the program rather than read. */
    QString comment;           /**< Free-text event comment. */

    friend bool operator==(const MNEEvent& a, const MNEEvent& b)
    {
        return a.sample == b.sample && a.from == b.from && a.to == b.to && a.show == b.show && a.created_here == b.created_here && a.comment == b.comment;
    }
};

} // namespace MNELIB

#endif // MNE_EVENT_H
