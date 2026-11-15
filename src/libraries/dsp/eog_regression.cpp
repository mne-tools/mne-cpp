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
#include <fiff/fiff_ch_name.h>
#include <base/log.h>

#include <vector>
#include <base/string_utils.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Cholesky>

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
    m_vecEogIndices.clear();
    m_vecTargetIndices.clear();
    m_bFitted = false;

    if (eogChannels.empty()) {
        for (int i = 0; i < std::ssize(info.chs); ++i) {
            if (info.chs[i].kind == FIFFV_EOG_CH && !mne::has_channel(info.bads, info.ch_names[i]))
                m_vecEogIndices.push_back(i);
        }
    } else {
        for (const std::string& name : eogChannels) {
            const int idx = static_cast<int>(mne::channel_index(info.ch_names, name));
            if (idx >= 0)
                m_vecEogIndices.push_back(idx);
            else
                Log::warning("[EogRegression::fit] EOG channel not found: {}", name);
        }
    }
    if (m_vecEogIndices.empty()) {
        Log::warning("[EogRegression::fit] No EOG channels found. Data will not be modified.");
        return;
    }

    // mne's default picks: the good data channels
    for (const int i : info.pick_data_channels(info.bads))
        m_vecTargetIndices.push_back(i);

    // Least squares on the time-demeaned signals, as mne.preprocessing.EOGRegression
    const MatrixXd E = demeanedRows(data, m_vecEogIndices);
    const MatrixXd T = demeanedRows(data, m_vecTargetIndices);
    m_matBeta = (E * E.transpose()).ldlt().solve(E * T.transpose()).transpose();
    m_bFitted = true;
}

//=============================================================================================================

void EogRegression::apply(MatrixXd& data,
                          const FiffInfo& info) const
{
    Q_UNUSED(info)

    if (!m_bFitted) {
        Log::warning("[EogRegression::apply] Not fitted yet. Call fit() first.");
        return;
    }
    const MatrixXd E = demeanedRows(data, m_vecEogIndices);
    for (int i = 0; i < std::ssize(m_vecTargetIndices); ++i)
        data.row(m_vecTargetIndices[i]) -= m_matBeta.row(i) * E;
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
