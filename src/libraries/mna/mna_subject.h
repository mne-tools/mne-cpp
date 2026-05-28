//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_subject.h
 * @since April 2026
 * @brief One participant in an MNA project — owner of measurement sessions and link to the FreeSurfer anatomy.
 *
 * @ref MnaSubject is the root of the per-participant subtree in an
 * MNA project. It binds together all sessions/recordings that
 * belong to one person and pins their structural MRI reconstruction
 * via @c freeSurferDir, a project-relative path to the matching
 * @c $SUBJECTS_DIR entry. Keeping that pointer alongside the data
 * means forward solutions, source spaces and parcellations always
 * resolve to the correct anatomy without relying on environment
 * variables at analysis time.
 *
 * The @c id mirrors the BIDS @c sub-XX convention and is the key
 * downstream consumers (forward modelling, source estimation,
 * group statistics) use to look the subject up. @c sessions holds
 * the ordered @ref MnaSession list, and @c extras preserves any
 * demographic, clinical or consent-tracking fields that newer
 * tooling may attach so older MNALIB builds can still read and
 * re-write the project without data loss.
 */

#ifndef MNA_SUBJECT_H
#define MNA_SUBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_session.h"

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
 * Represents a subject (participant) in the project.
 */
struct MNASHARED_EXPORT MnaSubject
{
    QString            id;              /**< Subject identifier. */
    QString            freeSurferDir;   /**< Relative path to FreeSurfer SUBJECTS_DIR. */
    QList<MnaSession>  sessions;        /**< Sessions for this subject. */
    QJsonObject        extras;          /**< Unknown keys preserved for lossless round-trip. */

    //=========================================================================================================
    /**
     * Serialize to QJsonObject.
     */
    QJsonObject toJson() const;

    //=========================================================================================================
    /**
     * Deserialize from QJsonObject.
     */
    static MnaSubject fromJson(const QJsonObject& json);

    //=========================================================================================================
    /**
     * Serialize to QCborMap.
     */
    QCborMap toCbor() const;

    //=========================================================================================================
    /**
     * Deserialize from QCborMap.
     */
    static MnaSubject fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_SUBJECT_H
