//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channel_derivation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    ChannelDerivation class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channel_derivation.h"

#include <mne/mne_deriv_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

/**
 * @brief Extract the shaft prefix from a channel name.
 *
 * Collects all leading alphabetic characters and apostrophes before the first digit.
 * E.g. "LH1" → "LH", "RA12" → "RA", "A'1" → "A'".
 */
static QString extractShaftPrefix(const QString& name)
{
    QString prefix;
    for (int i = 0; i < name.size(); ++i) {
        QChar ch = name[i];
        if (ch.isDigit()) {
            break;
        }
        prefix.append(ch);
    }
    return prefix;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QVector<DerivationRule> ChannelDerivation::buildBipolar(const QStringList& channelNames)
{
    // Group channels by shaft prefix, maintaining original order within each group
    QMap<QString, QStringList> groups;
    QStringList groupOrder;

    for (const QString& name : channelNames) {
        QString prefix = extractShaftPrefix(name);
        if (!groups.contains(prefix)) {
            groupOrder.append(prefix);
        }
        groups[prefix].append(name);
    }

    // Build bipolar pairs within each group
    QVector<DerivationRule> rules;
    for (const QString& prefix : groupOrder) {
        const QStringList& group = groups[prefix];
        for (int i = 0; i < group.size() - 1; ++i) {
            DerivationRule rule;
            rule.outputName = group[i] + "-" + group[i + 1];
            rule.inputWeights[group[i]] = 1.0;
            rule.inputWeights[group[i + 1]] = -1.0;
            rules.append(rule);
        }
    }

    return rules;
}

//=============================================================================================================

QVector<DerivationRule> ChannelDerivation::buildCommonAverage(const QStringList& channelNames)
{
    const int N = channelNames.size();
    if (N == 0) {
        return {};
    }

    const double invN = 1.0 / static_cast<double>(N);

    QVector<DerivationRule> rules;
    rules.reserve(N);

    for (const QString& target : channelNames) {
        DerivationRule rule;
        rule.outputName = target;
        for (const QString& ch : channelNames) {
            rule.inputWeights[ch] = -invN;
        }
        // Override target channel: weight = 1.0 - 1/N
        rule.inputWeights[target] = 1.0 - invN;
        rules.append(rule);
    }

    return rules;
}

//=============================================================================================================

QPair<MatrixXd, QStringList> ChannelDerivation::apply(
    const MatrixXd& matData,
    const QStringList& channelNames,
    const QVector<DerivationRule>& rules)
{
    // Build channel name → row index lookup
    QMap<QString, int> chIndex;
    for (int i = 0; i < channelNames.size(); ++i) {
        chIndex[channelNames[i]] = i;
    }

    const Eigen::Index nTimes = matData.cols();
    MatrixXd matResult = MatrixXd::Zero(rules.size(), nTimes);
    QStringList outputNames;
    outputNames.reserve(rules.size());

    for (int r = 0; r < rules.size(); ++r) {
        const DerivationRule& rule = rules[r];
        outputNames.append(rule.outputName);

        for (auto it = rule.inputWeights.constBegin(); it != rule.inputWeights.constEnd(); ++it) {
            auto idxIt = chIndex.constFind(it.key());
            if (idxIt == chIndex.constEnd()) {
                qWarning() << "ChannelDerivation::apply - channel not found:" << it.key()
                           << "in rule:" << rule.outputName;
                continue;
            }
            matResult.row(r) += it.value() * matData.row(idxIt.value());
        }
    }

    return qMakePair(matResult, outputNames);
}

//=============================================================================================================

QVector<DerivationRule> ChannelDerivation::readDefinitionFile(const QString& path)
{
    const std::optional<MNEDerivSet> set = MNEDerivSet::readText(path);
    if (!set) {
        return {};
    }
    QVector<DerivationRule> rules;
    for (const MNEDerivSet::Definition& definition : set->definitions()) {
        rules.append({definition.first, definition.second});
    }
    return rules;
}

//=============================================================================================================

bool ChannelDerivation::writeDefinitionFile(const QString& path, const QVector<DerivationRule>& rules)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "ChannelDerivation::writeDefinitionFile - cannot open:" << path;
        return false;
    }
    // MNE-C mne_make_derivations syntax; names are quoted because they may contain spaces.
    QTextStream out(&file);
    out << "# Channel derivations: \"name\" = weight * \"input\" + ...\n";
    for (const DerivationRule& rule : rules) {
        out << '"' << rule.outputName << "\" =";
        for (auto it = rule.inputWeights.constBegin(); it != rule.inputWeights.constEnd(); ++it) {
            out << ' ' << (it.value() < 0.0 ? '-' : '+') << ' ' << QString::number(std::fabs(it.value()), 'g', 17) << " * \"" << it.key() << '"';
        }
        out << '\n';
    }
    return true;
}
