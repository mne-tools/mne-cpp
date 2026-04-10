//=============================================================================================================
/**
 * @file     channel_derivation.h
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
 * @brief    ChannelDerivation class declaration — channel derivation and bipolar re-referencing.
 *
 */

#ifndef CHANNEL_DERIVATION_H
#define CHANNEL_DERIVATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief A single derivation rule mapping input channels (with weights) to one output channel.
 */
struct DSPSHARED_EXPORT DerivationRule
{
    QString outputName;                 ///< Name of the derived output channel.
    QMap<QString, double> inputWeights; ///< Map of input channel name → weight.
};

//=============================================================================================================
/**
 * @brief Channel derivation and re-referencing utilities.
 *
 * Provides methods to build bipolar and common-average re-referencing schemes,
 * apply arbitrary linear derivation rules to data matrices, and read/write
 * derivation definition files.
 *
 * @code
 *   QStringList chNames = {"LH1","LH2","LH3","RA1","RA2"};
 *   auto rules = ChannelDerivation::buildBipolar(chNames);
 *   // rules: LH1-LH2, LH2-LH3, RA1-RA2
 *
 *   auto [matDerived, derivedNames] = ChannelDerivation::apply(matData, chNames, rules);
 * @endcode
 */
class DSPSHARED_EXPORT ChannelDerivation
{
public:
    /**
     * @brief Build bipolar derivation rules from sequential electrode pairs.
     *
     * Channels are grouped by shaft prefix (all leading alphabetic characters and
     * apostrophes before the first digit). Within each group, consecutive pairs are
     * subtracted: channel[i] − channel[i+1].
     *
     * @param[in] channelNames   List of channel names, e.g. {"LH1","LH2","LH3","RA1","RA2"}.
     *
     * @return Vector of DerivationRule, e.g. LH1-LH2 (+1·LH1, −1·LH2), LH2-LH3, RA1-RA2.
     */
    static QVector<DerivationRule> buildBipolar(const QStringList& channelNames);

    /**
     * @brief Build common-average reference derivation rules.
     *
     * Each output channel equals the original channel minus the mean of all channels:
     * out_i = ch_i − (1/N) Σ ch_j.
     *
     * @param[in] channelNames   List of channel names.
     *
     * @return Vector of DerivationRule, one per input channel.
     */
    static QVector<DerivationRule> buildCommonAverage(const QStringList& channelNames);

    /**
     * @brief Apply derivation rules to a data matrix.
     *
     * @param[in] matData        Input data matrix (n_channels × n_times).
     * @param[in] channelNames   Channel names corresponding to rows of matData.
     * @param[in] rules          Derivation rules to apply.
     *
     * @return A pair of (derived data matrix, derived channel names).
     *         The output matrix has n_rules rows × n_times columns.
     */
    static QPair<Eigen::MatrixXd, QStringList> apply(
        const Eigen::MatrixXd& matData,
        const QStringList& channelNames,
        const QVector<DerivationRule>& rules);

    /**
     * @brief Read derivation rules from a text definition file.
     *
     * File format (lines starting with '#' are comments):
     * @code
     * # output_name = weight1 * input1 + weight2 * input2 + ...
     * LH1-LH2 = 1.0 * LH1 + -1.0 * LH2
     * @endcode
     *
     * @param[in] path   Path to the definition file.
     *
     * @return Vector of parsed DerivationRule. Empty on error.
     */
    static QVector<DerivationRule> readDefinitionFile(const QString& path);

    /**
     * @brief Write derivation rules to a text definition file.
     *
     * @param[in] path    Path to the output file.
     * @param[in] rules   Derivation rules to write.
     *
     * @return True if the file was written successfully.
     */
    static bool writeDefinitionFile(const QString& path, const QVector<DerivationRule>& rules);
};

} // namespace UTILSLIB

#endif // CHANNEL_DERIVATION_H
