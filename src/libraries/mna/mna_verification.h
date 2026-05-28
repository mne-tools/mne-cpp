//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_verification.h
 * @since 2026
 * @date  April 2026
 * @brief Verification, declarative checks, evaluation results and provenance snapshot attached to every @ref MnaNode for reproducible execution.
 *
 * This header carries the four structs that turn an MNA graph from
 * a black-box runner into a self-auditing pipeline. @ref MnaVerificationCheck
 * is a declarative pre- or post-condition (e.g. @c rank(covariance)
 * @c > @c 0) authored alongside the node; @ref MnaVerificationResult
 * records each evaluation outcome with the actual value, severity
 * and timestamp; and the parent @ref MnaVerification aggregates an
 * explanation string, the check list, both result lists and the
 * full @ref MnaProvenance snapshot.
 *
 * @ref MnaProvenance captures everything needed to re-run a node
 * bit-for-bit later: SHA-256 of every input, the resolved parameter
 * map after @ref MnaParamTree evaluation, MNE-CPP / Qt / compiler /
 * OS versions, external tool versions for IPC and Script nodes,
 * wall-clock and peak-RSS measurements, and the random seed when
 * stochastic ops are involved. Together these structs are what
 * lets MNA projects double as audit trails for clinical and
 * publication-grade analyses.
 */

#ifndef MNA_VERIFICATION_H
#define MNA_VERIFICATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_script.h"

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
    QString     expression;     ///< Simple evaluable expression: "rank(covariance) > 0"
    MnaScript   script;         ///< Optional script for complex checks (exit code 0 = pass).
                                ///< When script.code is non-empty, the executor runs the script
                                ///< instead of evaluating `expression`. Supports {{placeholder}}
                                ///< substitution for node inputs and attributes.
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
