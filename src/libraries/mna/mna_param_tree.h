//=============================================================================================================
/**
 * @file     mna_param_tree.h
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
 * @brief    MnaParamTree class declaration — hierarchical parameter store with formula-driven bindings.
 *
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
 * @brief Parameter tree with formula-driven dynamic bindings.
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
