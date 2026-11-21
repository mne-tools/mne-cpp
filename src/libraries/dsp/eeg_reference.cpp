//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     eeg_reference.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    EEG re-referencing functions implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eeg_reference.h"

#include <fiff/fiff_ch_name.h>
#include <base/log.h>
#include <base/string_utils.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

/**
 * @brief Collect indices of EEG channels that are not in the bads list.
 */
static std::vector<int> findGoodEegIndices(const FiffInfo& info)
{
    std::vector<int> indices;
    for (int i = 0; i < info.nchan; ++i) {
        if (info.chs[i].kind == FIFFV_EEG_CH && !info.isBad(info.ch_names[i])) {
            indices.push_back(i);
        }
    }
    return indices;
}

//=============================================================================================================
/**
 * @brief Collect indices of all EEG channels (including bads).
 */
static std::vector<int> findAllEegIndices(const FiffInfo& info)
{
    std::vector<int> indices;
    for (int i = 0; i < info.nchan; ++i) {
        if (info.chs[i].kind == FIFFV_EEG_CH) {
            indices.push_back(i);
        }
    }
    return indices;
}

//=============================================================================================================
// FUNCTION IMPLEMENTATIONS
//=============================================================================================================

void mne::setEegReference(MatrixXd& data,
                          FiffInfo& info,
                          const StringList& refChannels,
                          bool projection)
{
    const bool useAverage = refChannels.empty() || (refChannels.size() == 1 && mne::iequals(refChannels.front(), "average"));

    if (projection) {
        if (!useAverage) {
            Log::warning("setEegReference: projection is only supported for the average reference.");
            return;
        }
        for (const FiffProj& proj : std::as_const(info.projs)) {
            if (proj.kind == FIFFV_PROJ_ITEM_EEG_AVREF)
                return;
        }
        StringList names;
        for (int idx : findGoodEegIndices(info))
            names.push_back(info.ch_names[idx]);
        if (names.empty()) {
            Log::warning("setEegReference: no good EEG channels for an average reference projector.");
            return;
        }
        FiffNamedMatrix vec(1, static_cast<int>(names.size()), StringList(), names,
                            MatrixXd::Constant(1, names.size(), 1.0 / std::sqrt(static_cast<double>(names.size()))));
        info.projs.push_back(FiffProj(FIFFV_PROJ_ITEM_EEG_AVREF, false, "Average EEG reference", vec));
        return;
    }

    if (data.rows() != info.nchan) {
        Log::warning("setEegReference: data row count ({}) does not match info.nchan ({}).", static_cast<long long>(data.rows()), info.nchan);
        return;
    }

    if (data.cols() == 0 || data.rows() == 0) {
        return;
    }

    // As mne.set_eeg_reference, bad EEG channels are neither reference nor re-referenced
    const std::vector<int> goodEegIdx = findGoodEegIndices(info);
    if (findAllEegIndices(info).empty()) {
        Log::warning("setEegReference: no EEG channels found in info.");
        return;
    }

    // Compute reference signal
    RowVectorXd refSignal;

    if (useAverage) {
        // Average reference: mean of all good EEG channels
        if (goodEegIdx.empty()) {
            Log::warning("setEegReference: all EEG channels are marked as bad.");
            return;
        }
        refSignal = RowVectorXd::Zero(data.cols());
        for (int idx : goodEegIdx) {
            refSignal += data.row(idx);
        }
        refSignal /= static_cast<double>(goodEegIdx.size());
    } else {
        // Specific channel(s): mean of named channels
        std::vector<int> refIdx;
        for (const std::string& name : refChannels) {
            int idx = mne::channel_index(info.ch_names, name);
            if (idx < 0) {
                Log::warning("setEegReference: reference channel '{}' not found.", name);
                return;
            }
            refIdx.push_back(idx);
        }
        refSignal = RowVectorXd::Zero(data.cols());
        for (int idx : refIdx) {
            refSignal += data.row(idx);
        }
        refSignal /= static_cast<double>(refIdx.size());
    }

    for (int idx : goodEegIdx) {
        data.row(idx) -= refSignal;
    }
}

//=============================================================================================================

void mne::addReferenceChannels(MatrixXd& data,
                               FiffInfo& info,
                               const StringList& chNames)
{
    if (chNames.empty()) {
        return;
    }

    const Eigen::Index nTimes = data.cols();
    const Eigen::Index nOldCh = data.rows();
    const int nNew = static_cast<int>(chNames.size());

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

        info.chs.push_back(ch);
        info.ch_names.push_back(chNames[n]);
    }
    info.nchan += nNew;
}

//=============================================================================================================

void mne::setBipolarReference(MatrixXd& data,
                              FiffInfo& info,
                              const StringList& anodes,
                              const StringList& cathodes,
                              bool dropOriginals)
{
    if (anodes.size() != cathodes.size()) {
        Log::warning("setBipolarReference: anodes and cathodes must have the same length ({} vs {}).", static_cast<long long>(anodes.size()), static_cast<long long>(cathodes.size()));
        return;
    }

    if (anodes.empty()) {
        return;
    }

    if (data.rows() != info.nchan) {
        Log::warning("setBipolarReference: data row count ({}) does not match info.nchan ({}).", static_cast<long long>(data.rows()), info.nchan);
        return;
    }

    const int nPairs = static_cast<int>(anodes.size());
    const Eigen::Index nTimes = data.cols();

    // Resolve anode and cathode indices
    std::vector<int> anodeIdx(nPairs), cathodeIdx(nPairs);
    for (int i = 0; i < nPairs; ++i) {
        anodeIdx[i] = mne::channel_index(info.ch_names, anodes[i]);
        if (anodeIdx[i] < 0) {
            Log::warning("setBipolarReference: anode channel '{}' not found.", anodes[i]);
            return;
        }
        cathodeIdx[i] = mne::channel_index(info.ch_names, cathodes[i]);
        if (cathodeIdx[i] < 0) {
            Log::warning("setBipolarReference: cathode channel '{}' not found.", cathodes[i]);
            return;
        }
    }

    // Collect the set of original channels involved
    std::vector<bool> bipolarOriginals(info.chs.size(), false);
    for (int i = 0; i < nPairs; ++i) {
        bipolarOriginals[anodeIdx[i]] = true;
        bipolarOriginals[cathodeIdx[i]] = true;
    }

    // Build bipolar data rows
    MatrixXd bipolarData(nPairs, nTimes);
    for (int i = 0; i < nPairs; ++i) {
        bipolarData.row(i) = data.row(anodeIdx[i]) - data.row(cathodeIdx[i]);
    }

    // Build bipolar channel info
    std::vector<FiffChInfo> bipolarChs;
    StringList bipolarNames;
    for (int i = 0; i < nPairs; ++i) {
        FiffChInfo ch = info.chs[anodeIdx[i]];
        ch.ch_name = (anodes[i] + "-" + cathodes[i]);
        ch.logNo = i + 1;
        bipolarChs.push_back(ch);
        bipolarNames.push_back(ch.ch_name);
    }

    if (dropOriginals) {
        // Keep only non-bipolar-original rows + append bipolar rows
        std::vector<int> keepIdx;
        for (int i = 0; i < static_cast<int>(data.rows()); ++i) {
            if (!bipolarOriginals[i]) {
                keepIdx.push_back(i);
            }
        }

        const int nKeep = static_cast<int>(keepIdx.size());
        MatrixXd newData(nKeep + nPairs, nTimes);
        std::vector<FiffChInfo> newChs;
        StringList newNames;

        for (int i = 0; i < nKeep; ++i) {
            newData.row(i) = data.row(keepIdx[i]);
            newChs.push_back(info.chs[keepIdx[i]]);
            newNames.push_back(info.ch_names[keepIdx[i]]);
        }
        for (int i = 0; i < nPairs; ++i) {
            newData.row(nKeep + i) = bipolarData.row(i);
            newChs.push_back(bipolarChs[i]);
            newNames.push_back(bipolarNames[i]);
        }

        // Update scan numbers
        for (int i = 0; i < std::ssize(newChs); ++i) {
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

        info.chs.insert(info.chs.end(), bipolarChs.begin(), bipolarChs.end());
        info.ch_names.insert(info.ch_names.end(), bipolarNames.begin(), bipolarNames.end());
        info.nchan += nPairs;
        data = newData;
    }
}
