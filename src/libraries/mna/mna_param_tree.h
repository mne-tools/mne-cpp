//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_param_tree.h
 * @since 2026
 * @date  April 2026
 * @brief Hierarchical parameter store with formula-driven dynamic bindings shared across an MNA graph.
 *
 * @ref MnaParamTree is the second authoritative source of node
 * attribute values — the first being a node's own @c attributes
 * map. Where the latter holds the literal value typed by the user
 * for a single operation, the param tree expresses values that are
 * either reused across many nodes (e.g. @c subject/fsdir) or
 * computed from other values via @ref MnaParamBinding formulas
 * resolved at execution time.
 *
 * Paths are slash-separated and conventionally namespaced by node
 * id (@c bandpass_01/lowcut, @c bandpass_01/highcut), letting the
 * executor merge static defaults from the op schema, static
 * overrides from the project, and dynamically bound values into
 * the final @c resolvedAttributes captured in the
 * @ref MnaProvenance record. @ref evaluate walks all registered
 * bindings whose triggers have fired and returns the list of paths
 * whose values changed, which the executor uses to mark downstream
 * nodes dirty.
 */

#ifndef MNA_PARAM_TREE_H
#define MNA_PARAM_TREE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_param_binding.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QMap>
#include <QList>
#include <QJsonObject>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * Hierarchical parameter store for the MNA graph. Each parameter has a path
 * ("nodeId/attrKey") and an optional binding — a formula that recomputes the
 * value whenever a trigger condition is met.
 *
 * @brief Path-keyed parameter store with formula-driven dynamic bindings.
 */
class MNASHARED_EXPORT MnaParamTree
{
public:
    MnaParamTree();

    //=========================================================================================================
    // Static parameters
    //=========================================================================================================

    void        setParam(const QString& path, const QVariant& value);
    QVariant    param(const QString& path) const;
    bool        hasParam(const QString& path) const;
    QStringList allPaths() const;

    //=========================================================================================================
    // Dynamic bindings
    //=========================================================================================================

    void addBinding(const MnaParamBinding& binding);
    void removeBinding(const QString& targetPath);
    QList<MnaParamBinding> bindings() const;
    bool hasBinding(const QString& targetPath) const;

    //=========================================================================================================
    // Evaluation
    //=========================================================================================================

    /**
     * Evaluate all bindings whose trigger condition is met.
     * @param results   Map of nodeId::portName → data (from execution context).
     * @return Paths whose values changed.
     */
    QStringList evaluate(const QMap<QString, QVariant>& results);

    /**
     * Evaluate a single expression in the current context.
     * @param expr      Expression string.
     * @param results   Map of nodeId::portName → data (from execution context).
     * @return Computed value.
     */
    QVariant evaluateExpression(const QString& expr,
                                const QMap<QString, QVariant>& results) const;

    //=========================================================================================================
    // Serialization
    //=========================================================================================================

    QJsonObject toJson() const;
    static MnaParamTree fromJson(const QJsonObject& obj);

private:
    QMap<QString, QVariant>        m_params;
    QMap<QString, MnaParamBinding> m_bindings;
};

} // namespace MNALIB

#endif // MNA_PARAM_TREE_H
