//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_recording.h
 * @since April 2026
 * @brief Grouping of every file that belongs to one continuous measurement run inside an MNA project.
 *
 * An @ref MnaRecording is the second tier of the
 * subject → session → recording → file hierarchy used by MNA, and
 * corresponds to a single "press record / stop record" interval in
 * the acquisition software: one FIFF raw file plus its derived
 * artefacts (events, evoked, behavioural log, …). Modelling this
 * boundary explicitly avoids the legacy @c .ds / @c .fif sprawl
 * where the only grouping was a shared filename prefix.
 *
 * The @c id field is opaque to MNALIB but is conventionally the
 * recording's base name (e.g. @c run-01_meg) so the user-facing
 * GUI can render meaningful tree labels. @c files is an ordered
 * @ref MnaFileRef list whose first @c MnaFileRole::Raw entry is
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
 * Groups files belonging to one recording run.
 */
struct MNASHARED_EXPORT MnaRecording
{
    QString            id;     /**< Recording identifier. */
    QList<MnaFileRef>  files;  /**< Files belonging to this recording. */
    QJsonObject        extras; /**< Unknown keys preserved for lossless round-trip. */

    //=========================================================================================================
    /**
     * Serialize to QJsonObject.
     */
    QJsonObject toJson() const;

    //=========================================================================================================
    /**
     * Deserialize from QJsonObject.
     */
    static MnaRecording fromJson(const QJsonObject& json);

    //=========================================================================================================
    /**
     * Serialize to QCborMap.
     */
    QCborMap toCbor() const;

    //=========================================================================================================
    /**
     * Deserialize from QCborMap.
     */
    static MnaRecording fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_RECORDING_H
