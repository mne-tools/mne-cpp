//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     rt_cov.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Definition of the RtCov Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_cov.h"

#include <base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <numeric>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtCov::RtCov(const FiffInfo& info, double halfLife)
: m_info(info)
{
    for (int i = 0; i < std::ssize(m_info.chs); ++i) {
        if (m_info.chs[i].kind == FIFFV_MEG_CH || m_info.chs[i].kind == FIFFV_EEG_CH) {
            m_picks.push_back(i);
        }
    }
    if (m_picks.empty()) {
        m_picks.resize(m_info.chs.size());
        std::iota(m_picks.begin(), m_picks.end(), 0);
    }
    if (std::isfinite(halfLife)) {
        if (halfLife > 0.0 && m_info.sfreq > 0.0) {
            m_decay = std::exp2(-1.0 / (halfLife * m_info.sfreq));
        } else {
            Log::warning("[RtCov] A half-life needs a positive half-life and sampling rate; every sample counts the same.");
        }
    }
}

//=============================================================================================================

void RtCov::update(const MatrixXd& data)
{
    const Index n = data.cols();
    const auto nchan = std::ssize(m_info.chs);
    const auto used = std::ssize(m_picks);
    if (n == 0) {
        return;
    }
    if (used == 0 || (data.rows() != nchan && data.rows() != used)) {
        Log::warning("[RtCov::update] Ignoring a block of {} channels; expected {} or {}.", data.rows(), nchan, used);
        return;
    }
    MatrixXd block = data.rows() == used ? data : MatrixXd(data(m_picks, placeholders::all));

    // The newest sample has weight 1, each older one m_decay times the next
    VectorXd weights = VectorXd::Ones(n);
    for (Index t = n - 2; t >= 0 && m_decay != 1.0; --t) {
        weights(t) = weights(t + 1) * m_decay;
    }
    const double blockWeight = weights.sum();
    const VectorXd blockMean = block * weights / blockWeight;
    block.colwise() -= blockMean;
    if (m_decay != 1.0) {
        block *= weights.cwiseSqrt().asDiagonal();
    }

    if (m_weight == 0.0) {
        m_mean = blockMean;
        m_scatter = MatrixXd::Zero(used, used);
        m_scatter.selfadjointView<Lower>().rankUpdate(block);
        m_weight = blockWeight;
        m_weightSq = weights.squaredNorm();
        return;
    }
    // Age the running statistics by the block, then merge the block's centred statistics into them
    const double aging = std::pow(m_decay, static_cast<double>(n));
    m_weight *= aging;
    m_weightSq *= aging * aging;
    m_scatter *= aging;
    const VectorXd delta = blockMean - m_mean;
    const double total = m_weight + blockWeight;
    m_scatter.selfadjointView<Lower>().rankUpdate(block);
    m_scatter.selfadjointView<Lower>().rankUpdate(delta, m_weight * blockWeight / total);
    m_mean += delta * (blockWeight / total);
    m_weight = total;
    m_weightSq += weights.squaredNorm();
}

//=============================================================================================================

std::optional<FiffCov> RtCov::covariance() const
{
    // The reliability-weights correction of numpy.cov; zero for a single sample
    const double dof = m_weight > 0.0 ? m_weight - m_weightSq / m_weight : 0.0;
    if (dof <= 0.0) {
        return std::nullopt;
    }
    FiffCov cov;
    cov.data = MatrixXd(m_scatter.selfadjointView<Lower>()) / dof;
    cov.kind = FIFFV_MNE_NOISE_COV;
    cov.diag = false;
    cov.dim = static_cast<int>(cov.data.rows());
    cov.names.reserve(m_picks.size());
    for (const int pick : m_picks) {
        cov.names.push_back(m_info.ch_names[static_cast<std::size_t>(pick)]);
    }
    cov.projs = m_info.projs;
    cov.bads = m_info.bads;
    cov.nfree = static_cast<int>(std::lround(m_weight * m_weight / m_weightSq));
    return cov.regularize(m_info, 0.05, 0.05, 0.1, true, {});
}

//=============================================================================================================

void RtCov::reset()
{
    m_weight = 0.0;
    m_weightSq = 0.0;
}
