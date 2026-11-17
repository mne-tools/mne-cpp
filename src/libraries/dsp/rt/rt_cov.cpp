//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_cov.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Definition of the RtCov Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_cov.h"

#include <base/log.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtCov::RtCov(const FiffInfo& info)
: m_info(info)
{
    for (int i = 0; i < std::ssize(m_info.chs); ++i) {
        if (m_info.chs[i].kind == FIFFV_MEG_CH || m_info.chs[i].kind == FIFFV_EEG_CH) {
            m_picks.push_back(i);
        }
    }
}

//=============================================================================================================

std::optional<FiffCov> RtCov::estimate_covariance(const MatrixXd& data, int maxSamples)
{
    if (m_info.chs.empty()) {
        Log::warning("[RtCov::estimate_covariance] The measurement info has no channels.");
        return std::nullopt;
    }
    // Without MEG or EEG channels, or with fewer rows than picks, every row is used
    const bool picked = !m_picks.empty() && std::ssize(m_picks) < data.rows();
    const MatrixXd block = picked ? MatrixXd(data(m_picks, placeholders::all)) : data;
    if (m_samples == 0) {
        m_sum = block.rowwise().sum();
        m_outer = block * block.transpose();
    } else {
        m_sum += block.rowwise().sum();
        m_outer.noalias() += block * block.transpose();
    }
    m_samples += static_cast<int>(block.cols());
    if (m_samples < maxSamples || m_samples < 2) {
        return std::nullopt;
    }

    FiffCov cov;
    const VectorXd mean = m_sum / static_cast<double>(m_samples);
    cov.data = (m_outer - static_cast<double>(m_samples) * mean * mean.transpose()) / static_cast<double>(m_samples - 1);
    cov.kind = FIFFV_MNE_NOISE_COV;
    cov.diag = false;
    cov.dim = static_cast<int>(cov.data.rows());
    if (picked) {
        for (const int pick : m_picks) {
            cov.names.push_back(m_info.ch_names[static_cast<std::size_t>(pick)]);
        }
    } else {
        cov.names = m_info.ch_names;
    }
    cov.projs = m_info.projs;
    cov.bads = m_info.bads;
    cov.nfree = m_samples;
    m_samples = 0;
    return cov.regularize(m_info, 0.05, 0.05, 0.1, true, {});
}
