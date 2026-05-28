//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_param_binding.h
 * @since April 2026
 * @brief Formula-driven binding that recomputes one MNA parameter from an expression whenever its triggers fire.
 *
 * @ref MnaParamBinding turns the otherwise static @ref MnaParamTree
 * into a reactive system. Each binding links a @c targetPath
 * (typically @c nodeId/attrKey) to an @c expression that references
 * upstream results or other parameters through the @c ref("…")
 * helper — for example @c "clamp(ref('noise_est_01/snr') * 0.1,
 * @c 0.01, @c 1.0)" — so a downstream node's regularisation can be
 * derived live from an SNR estimate produced earlier in the graph.
 *
 * The @c trigger field selects when the formula is re-evaluated:
 * @c on_change fires whenever any path in @c dependencies updates,
 * @c periodic re-evaluates every @c periodMs (useful for streaming
 * pipelines), and @c manual leaves evaluation to explicit user
 * action. JSON/CBOR round-trip is symmetric so bindings survive
 * project save/load without loss of semantics.
 */

#ifndef MNA_PARAM_BINDING_H
#define MNA_PARAM_BINDING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QCborMap>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * A formula-driven parameter binding: links a target parameter path to an
 * expression that is re-evaluated when a trigger condition is met.
 *
 * @brief Dynamic parameter binding for the MNA parameter tree.
 */
struct MNASHARED_EXPORT MnaParamBinding
{
    QString     targetPath;     ///< Parameter to control: "nodeId/attrKey"
    QString     expression;     ///< Formula string, e.g. "clamp(ref('noise_est_01/snr') * 0.1, 0.01, 1.0)"
    QString     trigger;        ///< "on_change", "periodic", "manual"
    int         periodMs = 0;   ///< Evaluation period when trigger == "periodic" (ignored otherwise)
    QStringList dependencies;   ///< Paths this binding reads from

    QJsonObject toJson() const;
    static MnaParamBinding fromJson(const QJsonObject& json);
    QCborMap toCbor() const;
    static MnaParamBinding fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_PARAM_BINDING_H
