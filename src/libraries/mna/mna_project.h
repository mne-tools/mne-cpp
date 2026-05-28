//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_project.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Top-level MNA container binding subjects, processing pipeline, schema version and project metadata.
 *
 * @ref MnaProject is the in-memory representation of one @c .mna
 * (JSON) or @c .mnx (CBOR) file and the entry point for every other
 * MNALIB type: it owns the @ref MnaSubject list (subject → session
 * → recording → file) plus the @ref MnaNode pipeline that describes
 * how derivatives are produced from raw inputs. Reading or writing
 * a project is a single @ref MnaIO call away — @ref read and
 * @ref write are thin façades that dispatch on extension.
 *
 * The container is intentionally @em declarative: it records what
 * should be computed (operations, parameters, file references and
 * hashes) rather than embedding executable code, so collaborators
 * on different platforms can reproduce the analysis using their
 * own MNE-CPP build. @c mnaVersion / @ref CURRENT_SCHEMA_VERSION
 * are bumped on breaking schema changes, while @c extras preserves
 * forward-compatible additions so projects written by a newer
 * version round-trip losslessly through an older one.
 */

#ifndef MNA_PROJECT_H
#define MNA_PROJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_subject.h"
#include "mna_node.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QList>
#include <QDateTime>
#include <QSharedPointer>
#include <QJsonObject>
#include <QCborMap>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB{

//=============================================================================================================
/**
 * Top-level MNA project container. Holds subjects, pipeline steps, and project metadata.
 */
class MNASHARED_EXPORT MnaProject
{
public:
    typedef QSharedPointer<MnaProject>       SPtr;            /**< Shared pointer type. */
    typedef QSharedPointer<const MnaProject>  ConstSPtr;      /**< Const shared pointer type. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    MnaProject();

    //=========================================================================================================

    static constexpr const char* CURRENT_SCHEMA_VERSION = "1.0";  /**< Current MNA schema version. */

    QString            name;          /**< Project name. */
    QString            description;   /**< Project description. */
    QString            mnaVersion;    /**< MNA schema version. */
    QDateTime          created;       /**< Creation timestamp. */
    QDateTime          modified;      /**< Last modification timestamp. */
    QList<MnaSubject>  subjects;      /**< Subjects in the project. */
    QList<MnaNode>     pipeline;      /**< Processing pipeline nodes. */
    QJsonObject        extras;        /**< Unknown keys preserved for lossless round-trip. */

    //=========================================================================================================
    /**
     * Serialize to QJsonObject.
     */
    QJsonObject toJson() const;

    //=========================================================================================================
    /**
     * Deserialize from QJsonObject.
     */
    static MnaProject fromJson(const QJsonObject& json);

    //=========================================================================================================
    /**
     * Serialize to QCborMap.
     */
    QCborMap toCbor() const;

    //=========================================================================================================
    /**
     * Deserialize from QCborMap.
     */
    static MnaProject fromCbor(const QCborMap& cbor);

    //=========================================================================================================
    /**
     * Read an MNA project from file. Delegates to MnaIO.
     *
     * @param[in] path   Path to the .mna or .mnx file.
     *
     * @return The deserialized MnaProject.
     */
    static MnaProject read(const QString& path);

    //=========================================================================================================
    /**
     * Write an MNA project to file. Delegates to MnaIO.
     *
     * @param[in] project   The project to write.
     * @param[in] path      Path to the .mna or .mnx file.
     *
     * @return True if successful.
     */
    static bool write(const MnaProject& project, const QString& path);
};

} // namespace MNALIB

#endif // MNA_PROJECT_H
