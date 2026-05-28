//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_event.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Single recorded event (sample, previous and new trigger code).
 *
 * @ref MNELIB::MNEEvent is the per-event record used by the legacy
 * @c mne_process_raw event list: it stores the sample index relative to
 * the recording start, the trigger value before the transition and the
 * trigger value after - the canonical triplet of @c eve files.
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
 */
class MNESHARED_EXPORT MNEEvent
{
public:
    MNEEvent() = default;
    ~MNEEvent() = default;

    unsigned int from = 0;          /**< Source transition value. */
    unsigned int to = 0;            /**< Destination transition value. */
    int          sample = 0;        /**< Sample number. */
    bool         show = false;          /**< Display flag (application-defined). */
    bool         created_here = false;  /**< Non-zero if this event was created in the program. */
    QString      comment;           /**< Free-text event comment. */
};

} // namespace MNELIB

#endif // MNE_EVENT_H
