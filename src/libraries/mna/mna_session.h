//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_session.h
 * @since 2026
 * @date  April 2026
 * @brief Container for all recordings collected within a single experimental session for one subject.
 *
 * @ref MnaSession sits between @ref MnaSubject and @ref MnaRecording
 * in the MNA project tree and mirrors the BIDS @c ses-XX directory
 * level. It captures the natural grouping that occurs when a
 * subject visits the scanner more than once — e.g. baseline,
 * follow-up, intervention — without forcing those repeats into
 * separate subject entries that would break longitudinal analyses.
 *
 * The structure is intentionally thin: an opaque @c id (typically
 * @c ses-01, @c ses-pre, @c ses-post), an ordered list of
 * @ref MnaRecording instances, and an @c extras bag for
 * session-level sidecar metadata (date, scanner head-coil swap,
 * paradigm version) that should round-trip losslessly even when
 * unknown to the current MNALIB build.
 */

#ifndef MNA_SESSION_H
#define MNA_SESSION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_recording.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QCborMap>
#include <QSet>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB{

//=============================================================================================================
/**
 * Groups recordings belonging to one measurement session.
 */
struct MNASHARED_EXPORT MnaSession
{
    QString              id;          /**< Session identifier. */
    QList<MnaRecording>  recordings;  /**< Recordings in this session. */
    QJsonObject          extras;      /**< Unknown keys preserved for lossless round-trip. */

    //=========================================================================================================
    /**
     * Serialize to QJsonObject.
     */
    QJsonObject toJson() const;

    //=========================================================================================================
    /**
     * Deserialize from QJsonObject.
     */
    static MnaSession fromJson(const QJsonObject& json);

    //=========================================================================================================
    /**
     * Serialize to QCborMap.
     */
    QCborMap toCbor() const;

    //=========================================================================================================
    /**
     * Deserialize from QCborMap.
     */
    static MnaSession fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_SESSION_H
