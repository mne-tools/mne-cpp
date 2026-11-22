//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     eog_regression.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of EogRegression.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eog_regression.h"

#include <mne/fiff/fiff_ch_name.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Cholesky>

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

namespace
{

MatrixXd demeanedRows(const MatrixXd& data, const std::vector<int>& rows)
{
    MatrixXd out(rows.size(), data.cols());
    for (int i = 0; i < std::ssize(rows); ++i)
        out.row(i) = data.row(rows[i]).array() - data.row(rows[i]).mean();
    return out;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void EogRegression::fit(const MatrixXd& data,
                        const FiffInfo& info,
                        const StringList& eogChannels)
{
    m_bFitted = false;
    m_eogNames.clear();
    m_targetNames.clear();
    std::vector<int> eog;
    std::vector<int> targets;

    if (eogChannels.empty()) {
        for (int i = 0; i < std::ssize(info.chs); ++i) {
            if (info.chs[i].kind == FIFFV_EOG_CH && !mne::has_channel(info.bads, info.ch_names[i]))
                eog.push_back(i);
        }
    } else {
        for (const std::string& name : eogChannels) {
            const int idx = static_cast<int>(mne::channel_index(info.ch_names, name));
            if (idx >= 0)
                eog.push_back(idx);
            else
                Log::warning("[EogRegression::fit] EOG channel not found: {}", name);
        }
    }
    if (eog.empty()) {
        Log::warning("[EogRegression::fit] No EOG channels found. Data will not be modified.");
        return;
    }

    // mne's default picks: the good data channels
    for (const int i : info.pick_data_channels(info.bads))
        targets.push_back(i);

    // Least squares on the time-demeaned signals, as mne.preprocessing.EOGRegression
    const MatrixXd E = demeanedRows(data, eog);
    const MatrixXd T = demeanedRows(data, targets);
    m_matBeta = (E * E.transpose()).ldlt().solve(E * T.transpose()).transpose();
    for (const int i : eog)
        m_eogNames.push_back(info.ch_names[i]);
    for (const int i : targets)
        m_targetNames.push_back(info.ch_names[i]);
    m_bFitted = true;
}

//=============================================================================================================

void EogRegression::apply(MatrixXd& data,
                          const FiffInfo& info) const
{
    if (!m_bFitted) {
        Log::warning("[EogRegression::apply] Not fitted yet. Call fit() first.");
        return;
    }
    std::vector<int> eog;
    std::vector<int> targets;
    for (const auto& [names, rows] : {std::pair(&m_eogNames, &eog), std::pair(&m_targetNames, &targets)}) {
        for (const std::string& name : *names) {
            const int row = static_cast<int>(mne::channel_index(info.ch_names, name));
            if (row < 0 || row >= data.rows()) {
                Log::warning("[EogRegression::apply] The fitted channel {} is missing from the data.", name);
                return;
            }
            rows->push_back(row);
        }
    }
    const MatrixXd E = demeanedRows(data, eog);
    for (int i = 0; i < std::ssize(targets); ++i)
        data.row(targets[i]) -= m_matBeta.row(i) * E;
}

//=============================================================================================================

void EogRegression::fitApply(MatrixXd& data,
                             const FiffInfo& info,
                             const StringList& eogChannels)
{
    EogRegression reg;
    reg.fit(data, info, eogChannels);
    reg.apply(data, info);
}
