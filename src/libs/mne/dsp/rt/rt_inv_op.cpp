//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     rt_inv_op.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Definition of the RtInvOp Class.
 *
 * With the whitener W, the scaled source prior R = s·R₀ and the whitened weighted gain G̃ = W·G·R^½ = U·S·Vᵀ,
 * the batch kernel R^½·V·S(S² + λ²)⁻¹·Uᵀ·W·P equals s·R₀·Gᵀ·Wᵀ·C⁻¹·W·P with C = s·W·(G·R₀·Gᵀ)·Wᵀ + λ²·I, so
 * only C, channels by channels, has to be decomposed per covariance. The row norms of R^½·V·f(S)·Uᵀ that
 * normalize dSPM and sLORETA are those of s·R₀·Gᵀ·Wᵀ·E·f(D) with C = E·D·Eᵀ.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_inv_op.h"

#include <mne/base/log.h>
#include <mne/fiff/fiff_ch_name.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/math/linalg.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtInvOp::RtInvOp(const FiffInfo& info, MNEForwardSolution forward)
: RtInvOp(info, std::move(forward), Settings{})
{
}

//=============================================================================================================

RtInvOp::RtInvOp(const FiffInfo& info, MNEForwardSolution forward, Settings settings)
: m_info(info)
, m_forward(std::move(forward))
, m_settings(settings)
, m_fixed(m_forward.isFixedOrient())
{
    // Loose constraints are relative to the cortical normal, as in MNEInverseOperator::make_inverse_operator
    if (!m_fixed && m_settings.loose < 1.0f) {
        m_forward.convert_to_surf_ori();
    }
    if (!m_fixed) {
        m_orient = m_forward.compute_orient_prior(m_settings.loose).data.col(0);
    }
}

//=============================================================================================================

bool RtInvOp::update(const FiffCov& noiseCov)
{
    const InvEstimateMethod method = m_settings.method;
    if (method != InvEstimateMethod::MNE && method != InvEstimateMethod::dSPM && method != InvEstimateMethod::sLORETA) {
        Log::warning("[RtInvOp::update] Only MNE, dSPM and sLORETA are supported.");
        return false;
    }

    // The channels MNEForwardSolution::prepare_forward selects
    StringList names;
    for (const FiffChInfo& ch : m_info.chs) {
        if (!has_channel(m_info.bads, ch.ch_name) && !has_channel(noiseCov.bads, ch.ch_name) && has_channel(noiseCov.names, ch.ch_name) && has_channel(m_forward.info.ch_names, ch.ch_name)) {
            names.push_back(ch.ch_name);
        }
    }
    if (names.empty()) {
        Log::warning("[RtInvOp::update] No channel is in the measurement info, the covariance and the forward solution.");
        return false;
    }
    if (names != m_names) {
        prepare_channels(names);
    }

    FiffCov cov = noiseCov.prepare_noise_cov(m_info, m_names);
    VectorXd scale = VectorXd::Zero(cov.eig.size());
    int rank = 0;
    for (Index i = 0; i < scale.size(); ++i) {
        if (cov.eig[i] > 0) {
            scale[i] = 1.0 / std::sqrt(cov.eig[i]);
            ++rank;
        }
    }
    if (rank == 0) {
        Log::warning("[RtInvOp::update] The noise covariance has no positive eigenvalue.");
        return false;
    }
    // Rows of eigvec are the eigenvectors
    const MatrixXd whitener = scale.asDiagonal() * cov.eigvec;
    MatrixXd proj;
    FiffProj::make_projector(m_info.projs, cov.names, proj);
    if (proj.rows() != whitener.cols()) {
        proj = MatrixXd::Identity(whitener.cols(), whitener.cols());
    }

    // Source prior scaled so that the trace of the whitened G·R·Gᵀ is the rank
    const MatrixXd whitenedGainCov = whitener * m_gainCov * whitener.transpose();
    const double s = rank / whitenedGainCov.trace();
    MatrixXd c = s * whitenedGainCov;
    c.diagonal().array() += m_settings.lambda2;
    const SelfAdjointEigenSolver<MatrixXd> eig(c);
    const MatrixXd& e = eig.eigenvectors();
    const VectorXd& d = eig.eigenvalues();

    const MatrixXd ew = e.transpose() * whitener;
    m_kernel = s * ew.transpose() * d.cwiseInverse().asDiagonal() * ew * proj;

    m_noiseNorm.resize(0);
    if (method != InvEstimateMethod::MNE) {
        const VectorXd f = method == InvEstimateMethod::dSPM ? VectorXd(d.cwiseInverse()) : VectorXd(d.cwiseSqrt().cwiseInverse() / std::sqrt(m_settings.lambda2));
        VectorXd norms = ((f.asDiagonal() * ew) * m_gain).colwise().norm().transpose();
        norms = s * norms.cwiseProduct(m_sourcePrior);
        if (!m_fixed) {
            norms = Linalg::combine_xyz(norms.transpose()).cwiseSqrt();
        }
        m_noiseNorm = norms.cwiseInverse();
    }
    m_scale = s;
    m_noiseCov = std::move(cov);
    return true;
}

//=============================================================================================================

bool RtInvOp::ready() const
{
    return m_kernel.size() > 0;
}

//=============================================================================================================

MatrixXd RtInvOp::apply(const MatrixXd& data, int nave) const
{
    if (!ready() || data.rows() != std::ssize(m_info.chs)) {
        return {};
    }
    MatrixXd sol = m_outputLeads.transpose() * (m_kernel * data(m_picks, placeholders::all));
    if (!m_fixed && !m_settings.pick_normal) {
        // Magnitude of the three components of each source
        MatrixXd magnitude = sol.reshaped(3, sol.size() / 3).colwise().norm().reshaped(sol.rows() / 3, sol.cols());
        sol = std::move(magnitude);
    }
    if (m_noiseNorm.size() > 0) {
        // Averaging divides the noise variance by nave
        sol = (std::sqrt(static_cast<double>(nave)) * m_noiseNorm).asDiagonal() * sol;
    }
    return sol;
}

//=============================================================================================================

void RtInvOp::prepare_channels(const StringList& names)
{
    m_names = names;
    const auto n = std::ssize(names);
    m_picks.resize(static_cast<std::size_t>(n));
    VectorXi infoIdx(n);
    std::vector<int> fwdIdx(static_cast<std::size_t>(n));
    for (Index i = 0; i < n; ++i) {
        const std::string& name = names[static_cast<std::size_t>(i)];
        m_picks[static_cast<std::size_t>(i)] = infoIdx[i] = channel_index(m_info.ch_names, name);
        fwdIdx[static_cast<std::size_t>(i)] = channel_index(m_forward.info.ch_names, name);
    }
    m_gain = m_forward.sol.data(fwdIdx, placeholders::all);

    if (m_settings.depth > 0) {
        m_sourcePrior = MNEForwardSolution::compute_depth_prior(m_gain, m_info.pick_info(infoIdx.transpose()), m_fixed, m_settings.depth, 10.0, MatrixXd(), true).data.col(0);
    } else {
        m_sourcePrior = VectorXd::Ones(m_gain.cols());
    }
    if (!m_fixed) {
        m_sourcePrior = m_sourcePrior.cwiseProduct(m_orient);
    }

    const MatrixXd weighted = m_gain * m_sourcePrior.cwiseSqrt().asDiagonal();
    m_gainCov = MatrixXd::Zero(n, n);
    m_gainCov.selfadjointView<Lower>().rankUpdate(weighted);
    m_gainCov = m_gainCov.selfadjointView<Lower>();

    // Only the normal component of each free-orientation source if that is all that is estimated
    const Index step = !m_fixed && m_settings.pick_normal ? 3 : 1;
    const Index components = m_gain.cols() / step;
    m_outputLeads.resize(n, components);
    for (Index k = 0; k < components; ++k) {
        const Index component = k * step + step - 1;
        m_outputLeads.col(k) = m_gain.col(component) * m_sourcePrior[component];
    }
}

//=============================================================================================================

const FiffCov& RtInvOp::noise_covariance() const
{
    return m_noiseCov;
}

//=============================================================================================================

const MatrixXd& RtInvOp::gain() const
{
    return m_gain;
}

//=============================================================================================================

VectorXd RtInvOp::source_covariance() const
{
    return m_scale * m_sourcePrior;
}
