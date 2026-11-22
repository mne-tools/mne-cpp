//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_recording.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Grouping of every file that belongs to one continuous measurement run inside an MNA project.
 *
 * An @ref mne::MnaRecording is the second tier of the
 * subject → session → recording → file hierarchy used by MNA, and
 * corresponds to a single "press record / stop record" interval in
 * the acquisition software: one FIFF raw file plus its derived
 * artefacts (events, evoked, behavioural log, …). Modelling this
 * boundary explicitly avoids the legacy @c .ds / @c .fif sprawl
 * where the only grouping was a shared filename prefix.
 *
 * The @c id field is opaque to mne_mna but is conventionally the
 * recording's base name (e.g. @c run-01_meg) so the user-facing
 * GUI can render meaningful tree labels. @c files is an ordered
 * @ref mne::MnaFileRef list whose first @c MnaFileRole::Raw entry is
 * treated as the canonical recording; everything else is derived
 * data. @c extras keeps tool-specific metadata (acquisition notes,
 * BIDS sidecars) attached without forcing schema changes.
 */

#ifndef MNA_RECORDING_H
#define MNA_RECORDING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_file_ref.h"

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
 * @brief Files of one continuous measurement run in an MNA project.
 *
 * @snippet ex_mna/main.cpp mna_project_files
 */
struct MNASHARED_EXPORT MnaRecording
{
    std::string id;                /**< Recording identifier. */
    std::vector<MnaFileRef> files; /**< Files belonging to this recording. */
    JsonObject extras;             /**< Unknown keys preserved for lossless round-trip. */

    /** @return The recording as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A recording as written by to_json(). @return The recording. */
    static MnaRecording from_json(const JsonObject& json);

    /** @return The recording as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A recording as written by to_cbor(). @return The recording. */
    static MnaRecording from_cbor(const CborMap& cbor);
};

} // namespace mne

#endif // MNA_RECORDING_H
