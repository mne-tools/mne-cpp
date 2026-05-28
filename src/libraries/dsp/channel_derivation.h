//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file channel_derivation.h
 * @since 2026
 * @date  April 2026
 * @brief Linear channel derivations — bipolar montages and arbitrary weighted recombinations.
 *
 * ChannelDerivation builds an explicit derivation matrix @c D so that
 * @c derived = D · raw, where each output row is a weighted linear
 * combination of one or more input channels. Two common use cases are
 * supported directly: bipolar derivations of the form @c (anode − cathode)
 * (Fp1-F7, F7-T3, …), which are standard in clinical EEG, and arbitrary
 * sparse re-referencing schemes such as Hjorth Laplacian, double-banana,
 * or custom multi-channel sums.
 *
 * The derivation matrix is built once and then applied to every incoming
 * block; channel labels and types are updated consistently so downstream
 * processing (averaging, source localisation) sees a coherent montage.
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
