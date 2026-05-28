//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_file_ref.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Reference to a file inside an MNA project — relative path, semantic role, SHA-256 hash, and optional embedded payload.
 *
 * @ref MnaFileRef is the leaf node of an MNA project tree. Every raw
 * recording, forward/inverse operator, covariance matrix, BEM model
 * or annotation that belongs to a subject is recorded as one of
 * these structures, never as a bare path string, so the container
 * always knows what the file @em is (via @ref MnaFileRole), what
 * format it carries (FIFF, MGH, STC, …), how big it is, and whether
 * its bytes have changed since the project was last saved.
 *
 * The @c sha256 field is the contract that makes pipelines
 * reproducible: the graph executor refuses to consume an input
 * whose digest no longer matches and downstream cached results are
 * invalidated automatically. The @c embedded flag plus inline
 * @c data buffer let a project be self-contained when shipped to a
 * collaborator (think "single-file analysis bundle"), at the cost
 * of larger container size. @c extras preserves any keys produced
 * by newer tooling so old readers can still round-trip projects
 * without dropping unknown metadata.
 */

#ifndef MNA_FILE_REF_H
#define MNA_FILE_REF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QCborMap>
#include <QSet>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB{

//=============================================================================================================
/**
 * A reference to a file within an MNA project.
 */
struct MNASHARED_EXPORT MnaFileRef
{
    MnaFileRole role = MnaFileRole::Custom;   /**< Role of the file. */
    QString     path;                         /**< Relative POSIX path. */
    QString     sha256;                       /**< SHA-256 hash of file contents. */
    QString     format;                       /**< File format: "fiff", "mgh", "stc", etc. */
    qint64      sizeBytes = 0;                /**< File size in bytes. */
    bool        embedded  = false;            /**< Whether data is embedded in the container. */
    QByteArray  data;                         /**< Embedded file data (only when embedded == true). */
    QJsonObject extras;                       /**< Unknown keys preserved for lossless round-trip. */

    //=========================================================================================================
    /**
     * Serialize to QJsonObject.
     */
    QJsonObject toJson() const;

    //=========================================================================================================
    /**
     * Deserialize from QJsonObject.
     */
    static MnaFileRef fromJson(const QJsonObject& json);

    //=========================================================================================================
    /**
     * Serialize to QCborMap.
     */
    QCborMap toCbor() const;

    //=========================================================================================================
    /**
     * Deserialize from QCborMap.
     */
    static MnaFileRef fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_FILE_REF_H
