//=============================================================================================================
/**
 * @file     mna_node.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MnaNode struct declaration — one operation in the computational graph.
 *
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
 * @brief Graph node representing a processing step.
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
