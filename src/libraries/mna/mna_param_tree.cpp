//=============================================================================================================
/**
 * @file     mna_param_tree.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    MnaParamTree class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_param_tree.h"

#include <QJsonArray>
#include <QRegularExpression>

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnaParamTree::MnaParamTree()
{
}

//=============================================================================================================

void MnaParamTree::setParam(const QString& path, const QVariant& value)
{
    m_params.insert(path, value);
}

//=============================================================================================================

QVariant MnaParamTree::param(const QString& path) const
{
    return m_params.value(path);
}

//=============================================================================================================

bool MnaParamTree::hasParam(const QString& path) const
{
    return m_params.contains(path);
}

//=============================================================================================================

QStringList MnaParamTree::allPaths() const
{
    return m_params.keys();
}

//=============================================================================================================

void MnaParamTree::addBinding(const MnaParamBinding& binding)
{
    m_bindings.insert(binding.targetPath, binding);
}

//=============================================================================================================

void MnaParamTree::removeBinding(const QString& targetPath)
{
    m_bindings.remove(targetPath);
}

//=============================================================================================================

QList<MnaParamBinding> MnaParamTree::bindings() const
{
    return m_bindings.values();
}

//=============================================================================================================

bool MnaParamTree::hasBinding(const QString& targetPath) const
{
    return m_bindings.contains(targetPath);
}

//=============================================================================================================

QStringList MnaParamTree::evaluate(const QMap<QString, QVariant>& results)
{
    QStringList changed;

    for (auto it = m_bindings.constBegin(); it != m_bindings.constEnd(); ++it) {
        const MnaParamBinding& binding = it.value();

        // Skip manual bindings — they require explicit invocation
        if (binding.trigger == QStringLiteral("manual")) {
            continue;
        }

        QVariant newValue = evaluateExpression(binding.expression, results);

        if (newValue.isValid()) {
            QVariant oldValue = m_params.value(binding.targetPath);
            if (newValue != oldValue) {
                m_params.insert(binding.targetPath, newValue);
                changed.append(binding.targetPath);
            }
        }
    }

    return changed;
}

//=============================================================================================================

QVariant MnaParamTree::evaluateExpression(const QString& expr,
                                           const QMap<QString, QVariant>& results) const
{
    // Built-in function: ref('path') — look up a parameter or result value
    static const QRegularExpression refRe(QStringLiteral("ref\\('([^']+)'\\)"));

    // Simple case: bare ref('path')
    QRegularExpressionMatch refMatch = refRe.match(expr.trimmed());
    if (refMatch.hasMatch() && refMatch.capturedStart() == 0
        && refMatch.capturedEnd() == expr.trimmed().length()) {
        const QString path = refMatch.captured(1);
        if (m_params.contains(path)) {
            return m_params.value(path);
        }
        return results.value(path);
    }

    // Built-in: clamp(value, min, max)
    static const QRegularExpression clampRe(
        QStringLiteral("clamp\\((.+),\\s*([\\d.eE+-]+),\\s*([\\d.eE+-]+)\\)"));
    QRegularExpressionMatch clampMatch = clampRe.match(expr.trimmed());
    if (clampMatch.hasMatch()) {
        QVariant inner = evaluateExpression(clampMatch.captured(1).trimmed(), results);
        double val = inner.toDouble();
        double lo  = clampMatch.captured(2).toDouble();
        double hi  = clampMatch.captured(3).toDouble();
        return QVariant(std::clamp(val, lo, hi));
    }

    // Built-in: scale(value, factor)
    static const QRegularExpression scaleRe(
        QStringLiteral("scale\\((.+),\\s*([\\d.eE+-]+)\\)"));
    QRegularExpressionMatch scaleMatch = scaleRe.match(expr.trimmed());
    if (scaleMatch.hasMatch()) {
        QVariant inner = evaluateExpression(scaleMatch.captured(1).trimmed(), results);
        double factor = scaleMatch.captured(2).toDouble();
        return QVariant(inner.toDouble() * factor);
    }

    // Built-in: threshold(value, thresh, above, below)
    static const QRegularExpression threshRe(
        QStringLiteral("threshold\\((.+),\\s*([\\d.eE+-]+),\\s*([\\d.eE+-]+),\\s*([\\d.eE+-]+)\\)"));
    QRegularExpressionMatch threshMatch = threshRe.match(expr.trimmed());
    if (threshMatch.hasMatch()) {
        QVariant inner = evaluateExpression(threshMatch.captured(1).trimmed(), results);
        double val     = inner.toDouble();
        double thresh  = threshMatch.captured(2).toDouble();
        double above   = threshMatch.captured(3).toDouble();
        double below   = threshMatch.captured(4).toDouble();
        return QVariant(val > thresh ? above : below);
    }

    // Built-in: lerp(a, b, t)
    static const QRegularExpression lerpRe(
        QStringLiteral("lerp\\(([\\d.eE+-]+),\\s*([\\d.eE+-]+),\\s*(.+)\\)"));
    QRegularExpressionMatch lerpMatch = lerpRe.match(expr.trimmed());
    if (lerpMatch.hasMatch()) {
        double a = lerpMatch.captured(1).toDouble();
        double b = lerpMatch.captured(2).toDouble();
        QVariant tVal = evaluateExpression(lerpMatch.captured(3).trimmed(), results);
        double t = tVal.toDouble();
        return QVariant(a + (b - a) * t);
    }

    // Try to parse as a simple arithmetic expression with one ref and operators
    // Pattern: ref('path') * number or number / (ref('path') + number)
    // For now, handle "expr1 * expr2" and "expr1 / expr2" and "expr1 + expr2"
    // by simple splitting on top-level operators (no parentheses nesting for now)

    // Numeric literal
    bool ok = false;
    double numVal = expr.trimmed().toDouble(&ok);
    if (ok) {
        return QVariant(numVal);
    }

    // Simple binary: left * right
    static const QRegularExpression mulRe(QStringLiteral("^(.+)\\s*\\*\\s*([\\d.eE+-]+)$"));
    QRegularExpressionMatch mulMatch = mulRe.match(expr.trimmed());
    if (mulMatch.hasMatch()) {
        QVariant left = evaluateExpression(mulMatch.captured(1).trimmed(), results);
        double right = mulMatch.captured(2).toDouble();
        return QVariant(left.toDouble() * right);
    }

    // Simple binary: number / (expr)
    static const QRegularExpression divRe(QStringLiteral("^([\\d.eE+-]+)\\s*/\\s*\\((.+)\\)$"));
    QRegularExpressionMatch divMatch = divRe.match(expr.trimmed());
    if (divMatch.hasMatch()) {
        double left = divMatch.captured(1).toDouble();
        QVariant right = evaluateExpression(divMatch.captured(2).trimmed(), results);
        double rightVal = right.toDouble();
        if (std::abs(rightVal) < 1e-15) {
            return QVariant();
        }
        return QVariant(left / rightVal);
    }

    // Simple binary: expr + number
    static const QRegularExpression addRe(QStringLiteral("^(.+)\\s*\\+\\s*([\\d.eE+-]+)$"));
    QRegularExpressionMatch addMatch = addRe.match(expr.trimmed());
    if (addMatch.hasMatch()) {
        QVariant left = evaluateExpression(addMatch.captured(1).trimmed(), results);
        double right = addMatch.captured(2).toDouble();
        return QVariant(left.toDouble() + right);
    }

    return QVariant();
}

//=============================================================================================================

QJsonObject MnaParamTree::toJson() const
{
    QJsonObject json;

    // Bindings
    QJsonArray bindingsArr;
    for (auto it = m_bindings.constBegin(); it != m_bindings.constEnd(); ++it) {
        bindingsArr.append(it.value().toJson());
    }
    json[QStringLiteral("bindings")] = bindingsArr;

    // Static parameters (only those without a binding)
    QJsonObject paramsObj;
    for (auto it = m_params.constBegin(); it != m_params.constEnd(); ++it) {
        if (!m_bindings.contains(it.key())) {
            paramsObj.insert(it.key(), QJsonValue::fromVariant(it.value()));
        }
    }
    if (!paramsObj.isEmpty()) {
        json[QStringLiteral("parameters")] = paramsObj;
    }

    return json;
}

//=============================================================================================================

MnaParamTree MnaParamTree::fromJson(const QJsonObject& obj)
{
    MnaParamTree tree;

    const QJsonArray bindingsArr = obj.value(QStringLiteral("bindings")).toArray();
    for (const QJsonValue& v : bindingsArr) {
        tree.addBinding(MnaParamBinding::fromJson(v.toObject()));
    }

    const QJsonObject paramsObj = obj.value(QStringLiteral("parameters")).toObject();
    for (auto it = paramsObj.constBegin(); it != paramsObj.constEnd(); ++it) {
        tree.setParam(it.key(), it.value().toVariant());
    }

    return tree;
}
