//=============================================================================================================
/**
 * @file     eeg_reference.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    EEG re-referencing functions implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eeg_reference.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QSet>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

/**
 * @brief Collect indices of EEG channels that are not in the bads list.
 */
static QVector<int> findGoodEegIndices(const FiffInfo& info)
{
    QVector<int> indices;
    const QSet<QString> badSet(info.bads.begin(), info.bads.end());
    for (int i = 0; i < info.nchan; ++i) {
        if (info.chs[i].kind == FIFFV_EEG_CH && !badSet.contains(info.ch_names[i])) {
            indices.append(i);
        }
    }
    return indices;
}

//=============================================================================================================
/**
 * @brief Collect indices of all EEG channels (including bads).
 */
static QVector<int> findAllEegIndices(const FiffInfo& info)
{
    QVector<int> indices;
    for (int i = 0; i < info.nchan; ++i) {
        if (info.chs[i].kind == FIFFV_EEG_CH) {
            indices.append(i);
        }
    }
    return indices;
}

//=============================================================================================================
// FUNCTION IMPLEMENTATIONS
//=============================================================================================================

void UTILSLIB::setEegReference(MatrixXd& data,
                                const FiffInfo& info,
                                const QStringList& refChannels,
                                bool projection)
{
    if (projection) {
        qWarning("setEegReference: projection mode is not yet implemented. "
                 "Falling back to direct data modification.");
    }

    if (data.rows() != info.nchan) {
        qWarning("setEegReference: data row count (%lld) does not match info.nchan (%d).",
                 static_cast<long long>(data.rows()), info.nchan);
        return;
    }

    if (data.cols() == 0 || data.rows() == 0) {
        return;
    }

    // Determine which EEG channels to subtract from
    const QVector<int> allEegIdx = findAllEegIndices(info);
    if (allEegIdx.isEmpty()) {
        qWarning("setEegReference: no EEG channels found in info.");
        return;
    }

    // Compute reference signal
    RowVectorXd refSignal;

    const bool useAverage = refChannels.isEmpty()
                            || (refChannels.size() == 1
                                && refChannels.first().compare(QLatin1String("average"), Qt::CaseInsensitive) == 0);

    if (useAverage) {
        // Average reference: mean of all good EEG channels
        const QVector<int> goodEegIdx = findGoodEegIndices(info);
        if (goodEegIdx.isEmpty()) {
            qWarning("setEegReference: all EEG channels are marked as bad.");
            return;
        }
        refSignal = RowVectorXd::Zero(data.cols());
        for (int idx : goodEegIdx) {
            refSignal += data.row(idx);
        }
        refSignal /= static_cast<double>(goodEegIdx.size());
    } else {
        // Specific channel(s): mean of named channels
        QVector<int> refIdx;
        for (const QString& name : refChannels) {
            int idx = info.ch_names.indexOf(name);
            if (idx < 0) {
                qWarning("setEegReference: reference channel '%s' not found.", qPrintable(name));
                return;
            }
            refIdx.append(idx);
        }
        refSignal = RowVectorXd::Zero(data.cols());
        for (int idx : refIdx) {
            refSignal += data.row(idx);
        }
        refSignal /= static_cast<double>(refIdx.size());
    }

    // Subtract reference from all EEG channels
    for (int idx : allEegIdx) {
        data.row(idx) -= refSignal;
    }
}

//=============================================================================================================

void UTILSLIB::addReferenceChannels(MatrixXd& data,
                                     FiffInfo& info,
                                     const QStringList& chNames)
{
    if (chNames.isEmpty()) {
        return;
    }

    const Eigen::Index nTimes = data.cols();
    const Eigen::Index nOldCh = data.rows();
    const int nNew = chNames.size();

    // Expand data matrix with zero rows
    MatrixXd newData(nOldCh + nNew, nTimes);
    if (nOldCh > 0 && nTimes > 0) {
        newData.topRows(nOldCh) = data;
    }
    newData.bottomRows(nNew).setZero();
    data = newData;

    // Find a template EEG channel for defaults
    FiffChInfo templateCh;
    bool haveTemplate = false;
    for (int i = 0; i < info.nchan; ++i) {
        if (info.chs[i].kind == FIFFV_EEG_CH) {
            templateCh = info.chs[i];
            haveTemplate = true;
            break;
        }
    }

    for (int n = 0; n < nNew; ++n) {
        FiffChInfo ch;
        if (haveTemplate) {
            ch = templateCh;
        }
        ch.ch_name = chNames[n];
        ch.kind = FIFFV_EEG_CH;
        ch.scanNo = info.nchan + n + 1;
        ch.logNo = ch.scanNo;

        info.chs.append(ch);
        info.ch_names.append(chNames[n]);
    }
    info.nchan += nNew;
}

//=============================================================================================================

void UTILSLIB::setBipolarReference(MatrixXd& data,
                                    FiffInfo& info,
                                    const QStringList& anodes,
                                    const QStringList& cathodes,
                                    bool dropOriginals)
{
    if (anodes.size() != cathodes.size()) {
        qWarning("setBipolarReference: anodes and cathodes must have the same length "
                 "(%d vs %d).", anodes.size(), cathodes.size());
        return;
    }

    if (anodes.isEmpty()) {
        return;
    }

    if (data.rows() != info.nchan) {
        qWarning("setBipolarReference: data row count (%lld) does not match info.nchan (%d).",
                 static_cast<long long>(data.rows()), info.nchan);
        return;
    }

    const int nPairs = anodes.size();
    const Eigen::Index nTimes = data.cols();

    // Resolve anode and cathode indices
    QVector<int> anodeIdx(nPairs), cathodeIdx(nPairs);
    for (int i = 0; i < nPairs; ++i) {
        anodeIdx[i] = info.ch_names.indexOf(anodes[i]);
        if (anodeIdx[i] < 0) {
            qWarning("setBipolarReference: anode channel '%s' not found.", qPrintable(anodes[i]));
            return;
        }
        cathodeIdx[i] = info.ch_names.indexOf(cathodes[i]);
        if (cathodeIdx[i] < 0) {
            qWarning("setBipolarReference: cathode channel '%s' not found.", qPrintable(cathodes[i]));
            return;
        }
    }

    // Collect the set of original channels involved
    QSet<int> bipolarOriginals;
    for (int i = 0; i < nPairs; ++i) {
        bipolarOriginals.insert(anodeIdx[i]);
        bipolarOriginals.insert(cathodeIdx[i]);
    }

    // Build bipolar data rows
    MatrixXd bipolarData(nPairs, nTimes);
    for (int i = 0; i < nPairs; ++i) {
        bipolarData.row(i) = data.row(anodeIdx[i]) - data.row(cathodeIdx[i]);
    }

    // Build bipolar channel info
    QList<FiffChInfo> bipolarChs;
    QStringList bipolarNames;
    for (int i = 0; i < nPairs; ++i) {
        FiffChInfo ch = info.chs[anodeIdx[i]];
        ch.ch_name = anodes[i] + "-" + cathodes[i];
        ch.logNo = i + 1;
        bipolarChs.append(ch);
        bipolarNames.append(ch.ch_name);
    }

    if (dropOriginals) {
        // Keep only non-bipolar-original rows + append bipolar rows
        QVector<int> keepIdx;
        for (int i = 0; i < static_cast<int>(data.rows()); ++i) {
            if (!bipolarOriginals.contains(i)) {
                keepIdx.append(i);
            }
        }

        const int nKeep = keepIdx.size();
        MatrixXd newData(nKeep + nPairs, nTimes);
        QList<FiffChInfo> newChs;
        QStringList newNames;

        for (int i = 0; i < nKeep; ++i) {
            newData.row(i) = data.row(keepIdx[i]);
            newChs.append(info.chs[keepIdx[i]]);
            newNames.append(info.ch_names[keepIdx[i]]);
        }
        for (int i = 0; i < nPairs; ++i) {
            newData.row(nKeep + i) = bipolarData.row(i);
            newChs.append(bipolarChs[i]);
            newNames.append(bipolarNames[i]);
        }

        // Update scan numbers
        for (int i = 0; i < newChs.size(); ++i) {
            newChs[i].scanNo = i + 1;
        }

        data = newData;
        info.chs = newChs;
        info.ch_names = newNames;
        info.nchan = static_cast<int>(newData.rows());
    } else {
        // Keep all original rows, append bipolar rows at the end
        MatrixXd newData(data.rows() + nPairs, nTimes);
        newData.topRows(data.rows()) = data;
        newData.bottomRows(nPairs) = bipolarData;

        for (int i = 0; i < nPairs; ++i) {
            bipolarChs[i].scanNo = info.nchan + i + 1;
        }

        info.chs.append(bipolarChs);
        info.ch_names.append(bipolarNames);
        info.nchan += nPairs;
        data = newData;
    }
}
