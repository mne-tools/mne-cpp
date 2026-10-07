//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_session.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Container for all recordings collected within a single experimental session for one subject.
 *
 * @ref MNALIB::MnaSession sits between @ref MNALIB::MnaSubject and @ref MNALIB::MnaRecording
 * in the MNA project tree and mirrors the BIDS @c ses-XX directory
 * level. It captures the natural grouping that occurs when a
 * subject visits the scanner more than once — e.g. baseline,
 * follow-up, intervention — without forcing those repeats into
 * separate subject entries that would break longitudinal analyses.
 *
 * The structure is intentionally thin: an opaque @c id (typically
 * @c ses-01, @c ses-pre, @c ses-post), an ordered list of
 * @ref MNALIB::MnaRecording instances, and an @c extras bag for
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

namespace MNALIB
{

//=============================================================================================================
/**
 * Groups recordings belonging to one measurement session.
 *
 * @snippet ex_mna/main.cpp mna_project_files
 */
struct MNASHARED_EXPORT MnaSession
{
    QString id;                     /**< Session identifier. */
    QList<MnaRecording> recordings; /**< Recordings in this session. */
    QJsonObject extras;             /**< Unknown keys preserved for lossless round-trip. */

    //=========================================================================================================
    /**
     * Serialize to QJsonObject.
     *
     * @return JSON object with the session id, its recordings and preserved extras.
     */
    QJsonObject toJson() const;

    //=========================================================================================================
    /**
     * Deserialize from QJsonObject.
     *
     * @param[in] json   JSON object as produced by toJson().
     *
     * @return The session; missing keys take default values and unknown keys go to extras.
     */
    static MnaSession fromJson(const QJsonObject& json);

    //=========================================================================================================
    /**
     * Serialize to QCborMap.
     *
     * @return CBOR map with the session id, its recordings and preserved extras.
     */
    QCborMap toCbor() const;

    //=========================================================================================================
    /**
     * Deserialize from QCborMap.
     *
     * @param[in] cbor   CBOR map as produced by toCbor().
     *
     * @return The session; missing keys take default values and unknown keys go to extras.
     */
    static MnaSession fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_SESSION_H
