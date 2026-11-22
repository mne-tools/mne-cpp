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
 * @ref mne::MnaSession sits between @ref mne::MnaSubject and @ref mne::MnaRecording
 * in the MNA project tree and mirrors the BIDS @c ses-XX directory
 * level. It captures the natural grouping that occurs when a
 * subject visits the scanner more than once — e.g. baseline,
 * follow-up, intervention — without forcing those repeats into
 * separate subject entries that would break longitudinal analyses.
 *
 * The structure is intentionally thin: an opaque @c id (typically
 * @c ses-01, @c ses-pre, @c ses-post), an ordered list of
 * @ref mne::MnaRecording instances, and an @c extras bag for
 * session-level sidecar metadata (date, scanner head-coil swap,
 * paradigm version) that should round-trip losslessly even when
 * unknown to the current mne_mna build.
 */

#ifndef MNA_SESSION_H
#define MNA_SESSION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_recording.h"

#include <mne/base/cbor.h>
#include <mne/base/json.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Recordings of one experimental session of a subject.
 *
 * @snippet ex_mna/main.cpp mna_project_files
 */
struct MNASHARED_EXPORT MnaSession
{
    std::string id;                       /**< Session identifier. */
    std::vector<MnaRecording> recordings; /**< Recordings in this session. */
    JsonObject extras;                    /**< Unknown keys preserved for lossless round-trip. */

    /** @return The session as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A session as written by to_json(). @return The session. */
    static MnaSession from_json(const JsonObject& json);

    /** @return The session as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A session as written by to_cbor(). @return The session. */
    static MnaSession from_cbor(const CborMap& cbor);
};

} // namespace mne

#endif // MNA_SESSION_H
