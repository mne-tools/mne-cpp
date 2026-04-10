//=============================================================================================================
/**
 * @file     channel_derivation.cpp
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
 * @brief    ChannelDerivation class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channel_derivation.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

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
            rule.inputWeights[group[i]]     =  1.0;
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
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ChannelDerivation::readDefinitionFile - cannot open:" << path;
        return {};
    }

    QVector<DerivationRule> rules;
    QTextStream in(&file);

    // Pattern: output_name = weight1 * input1 + weight2 * input2 + ...
    static const QRegularExpression reTerms(
        R"(([+-]?\s*[\d.]+(?:[eE][+-]?\d+)?)\s*\*\s*(\S+))");

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        int eqPos = line.indexOf('=');
        if (eqPos < 0) {
            qWarning() << "ChannelDerivation::readDefinitionFile - malformed line:" << line;
            continue;
        }

        DerivationRule rule;
        rule.outputName = line.left(eqPos).trimmed();
        QString rhs = line.mid(eqPos + 1);

        QRegularExpressionMatchIterator matchIt = reTerms.globalMatch(rhs);
        while (matchIt.hasNext()) {
            QRegularExpressionMatch m = matchIt.next();
            QString weightStr = m.captured(1).remove(' ');
            bool ok = false;
            double weight = weightStr.toDouble(&ok);
            if (ok) {
                rule.inputWeights[m.captured(2)] = weight;
            } else {
                qWarning() << "ChannelDerivation::readDefinitionFile - bad weight:" << weightStr;
            }
        }

        if (!rule.inputWeights.isEmpty()) {
            rules.append(rule);
        }
    }

    file.close();
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

    QTextStream out(&file);
    out << "# Channel derivation file\n";
    out << "# Format: output_name = weight1 * input1 + weight2 * input2 + ...\n";

    for (const DerivationRule& rule : rules) {
        out << rule.outputName << " = ";
        bool first = true;
        for (auto it = rule.inputWeights.constBegin(); it != rule.inputWeights.constEnd(); ++it) {
            if (!first) {
                out << " + ";
            }
            out << it.value() << " * " << it.key();
            first = false;
        }
        out << "\n";
    }

    file.close();
    return true;
}
