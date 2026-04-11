//=============================================================================================================
/**
 * @file     mna_verification.h
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
 * @brief    MnaVerification, MnaVerificationCheck, MnaVerificationResult, MnaProvenance declarations.
 *
 */

#ifndef MNA_VERIFICATION_H
#define MNA_VERIFICATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <QJsonObject>
#include <QCborMap>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * A declarative check (pre- or post-condition) attached to a graph node.
 * Evaluated by the executor; can warn, abort, or log an informational note.
 *
 * @brief Verification check for a graph node.
 */
struct MNASHARED_EXPORT MnaVerificationCheck
{
    QString     id;             ///< Unique check identifier within the node (e.g. "cov_posdef")
    QString     description;    ///< Human-readable: "Covariance matrix must be positive-definite"
    QString     phase;          ///< "pre" (before execution) or "post" (after execution)
    QString     expression;     ///< Evaluable expression: "rank(covariance) > 0"
    QString     severity;       ///< "error" (abort), "warning" (log + continue), "info" (always continue)
    QString     onFail;         ///< Optional remediation hint

    QJsonObject toJson() const;
    static MnaVerificationCheck fromJson(const QJsonObject& json);
    QCborMap toCbor() const;
    static MnaVerificationCheck fromCbor(const QCborMap& cbor);
};

//=============================================================================================================
/**
 * The result of evaluating a single verification check.
 *
 * @brief Check evaluation result.
 */
struct MNASHARED_EXPORT MnaVerificationResult
{
    QString     checkId;        ///< References MnaVerificationCheck::id
    bool        passed = false; ///< true if the expression evaluated to true
    QString     severity;       ///< Echoed from the check definition
    QString     message;        ///< Formatted message: "PASS: ..." or "FAIL [error]: ..."
    QVariant    actualValue;    ///< The evaluated expression result
    QDateTime   evaluatedAt;    ///< When this check was evaluated

    QJsonObject toJson() const;
    static MnaVerificationResult fromJson(const QJsonObject& json);
    QCborMap toCbor() const;
    static MnaVerificationResult fromCbor(const QCborMap& cbor);
};

//=============================================================================================================
/**
 * Complete provenance snapshot captured after node execution.
 * Records input hashes, resolved parameter values, software versions, and timing.
 *
 * @brief Provenance record for reproducibility.
 */
struct MNASHARED_EXPORT MnaProvenance
{
    // Input snapshot
    QMap<QString, QString> inputHashes;     ///< portName → SHA-256

    // Resolved attributes at execution time (after param-tree evaluation)
    QVariantMap resolvedAttributes;

    // Software environment
    QString     mneCppVersion;              ///< e.g. "2.2.0"
    QString     qtVersion;                  ///< e.g. "6.11.0"
    QString     compilerInfo;               ///< e.g. "AppleClang 16.0.0"
    QString     osInfo;                     ///< e.g. "macOS 15.4 arm64"
    QString     hostName;                   ///< Machine name (for cluster provenance)

    // For IPC/Script nodes
    QString     externalToolVersion;        ///< e.g. "FreeSurfer 7.4.1", "Python 3.11.5"

    // Timing
    QDateTime   startedAt;
    QDateTime   finishedAt;
    qint64      wallTimeMs = 0;             ///< Wall-clock duration in milliseconds
    qint64      peakMemoryBytes = 0;        ///< Peak RSS (if measurable), 0 otherwise

    // Random seed (if stochastic operations were used)
    qint64      randomSeed = -1;            ///< -1 if not applicable

    QJsonObject toJson() const;
    static MnaProvenance fromJson(const QJsonObject& json);
    QCborMap toCbor() const;
    static MnaProvenance fromCbor(const QCborMap& cbor);
};

//=============================================================================================================
/**
 * Top-level verification container attached to each MnaNode.
 * Combines human-readable explanation, declarative checks, evaluation results,
 * and a complete provenance snapshot.
 *
 * @brief Verification, explanation, and provenance for a graph node.
 */
struct MNASHARED_EXPORT MnaVerification
{
    /// Human-readable explanation of what this node does and why
    QString     explanation;

    /// Declarative checks (authored by user, evaluated by executor)
    QList<MnaVerificationCheck> checks;

    /// Results of pre-execution checks (populated by executor)
    QList<MnaVerificationResult> preResults;

    /// Results of post-execution checks (populated by executor)
    QList<MnaVerificationResult> postResults;

    /// Complete provenance snapshot (populated by executor)
    MnaProvenance provenance;

    QJsonObject toJson() const;
    static MnaVerification fromJson(const QJsonObject& json);
    QCborMap toCbor() const;
    static MnaVerification fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_VERIFICATION_H
