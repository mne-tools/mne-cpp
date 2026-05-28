//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_node.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    One operation in an MNA graph — opType, typed I/O ports, attributes, execution mode (Batch / Stream / IPC / Script) and provenance.
 *
 * @ref MnaNode is the executable counterpart to a @ref MnaFileRef:
 * where the latter records a derived artefact, the node records
 * @em how that artefact is produced. The @c opType string is
 * resolved against @ref MnaOpRegistry to obtain the
 * @ref MnaOpSchema (and, for built-in ops, the implementation
 * function) used by @ref MnaGraphExecutor; @c attributes provides
 * the per-call parameter values validated against that schema.
 *
 * The @c execMode field selects one of four executors:
 * @c Batch (default, file-in/file-out), @c Stream (live MNE Scan
 * plugin), @c Ipc (delegates to an external binary via
 * @c ipcCommand / @c ipcArgs / @c ipcTransport), or @c Script
 * (inline @ref MnaScript run by an interpreter). Each mode reuses
 * the same node fields, so a pipeline can mix all four without
 * structural changes.
 *
 * @c verification carries pre/post checks, explanation text and the
 * full @ref MnaProvenance snapshot captured at execution time;
 * combined with @c toolVersion, @c executedAt and the @c dirty flag
 * this is what makes MNA pipelines reproducible and supports
 * incremental re-execution after a parameter edit.
 */

#ifndef MNA_NODE_H
#define MNA_NODE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_types.h"
#include "mna_port.h"
#include "mna_script.h"
#include "mna_verification.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QDateTime>
#include <QJsonObject>
#include <QCborMap>
#include <QList>
#include <QSet>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * One operation in the computational graph.
 *
 * @brief Single executable step in an MNA pipeline graph, with attributes, typed ports, exec mode, verification and provenance.
 */
struct MNASHARED_EXPORT MnaNode
{
    QString     id;                     ///< Unique node identifier
    QString     opType;                 ///< Operation type (looked up in MnaOpRegistry)
    QVariantMap attributes;             ///< Operation parameters

    QList<MnaPort> inputs;              ///< Input ports
    QList<MnaPort> outputs;             ///< Output ports

    MnaNodeExecMode execMode = MnaNodeExecMode::Batch; ///< Execution mode

    // IPC configuration (used when execMode == Ipc)
    QString     ipcCommand;             ///< External executable command
    QStringList ipcArgs;                ///< Command-line arguments (supports {{placeholder}} tokens)
    QString     ipcWorkDir;             ///< Working directory for external process
    QString     ipcTransport;           ///< "stdio", "tcp", "shm", "file"

    // Script configuration (used when execMode == Script)
    MnaScript   script;                 ///< Inline source code, interpreter, language

    // Verification & provenance
    MnaVerification verification;       ///< Explanation, checks, results, and provenance snapshot

    // Execution metadata
    QString     toolVersion;            ///< Version of tool that last executed this node
    QDateTime   executedAt;             ///< Timestamp of last execution
    bool        dirty = true;           ///< Whether node needs re-execution
    QJsonObject extras;                 ///< Unknown keys preserved for lossless round-trip

    QJsonObject toJson() const;
    static MnaNode fromJson(const QJsonObject& json);
    QCborMap toCbor() const;
    static MnaNode fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_NODE_H
