//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     rt_hpi.cpp
 * @author   Chiran Doshi <chiran.doshi@childrens.harvard.edu>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March, 2015
 * @brief    Definition of the RtHpi Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_hpi.h"

#include <base/log.h>
#include <fiff/fiff_constants.h>
#include <inv/hpi/inv_hpi_model_parameters.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <numeric>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtHpi::RtHpi(std::shared_ptr<FiffInfo> info, Settings settings)
: m_info(std::move(info))
, m_settings(std::move(settings))
, m_updater(m_info)
, m_fit(m_updater.getSensors())
, m_projectors(MatrixXd::Identity(m_info->nchan, m_info->nchan))
, m_initial(m_info->dev_head_t)
{
    m_initial.from = FIFFV_COORD_DEVICE;
    m_initial.to = FIFFV_COORD_HEAD;
    set_window(m_settings.window, m_settings.hop);
    // The first fit starts from the head position of the measurement info
    m_last.devHeadTrans = m_initial;
    m_last.errorDistances.assign(m_settings.coil_frequencies.size(), 0.0);
}

//=============================================================================================================

std::optional<HpiFitResult> RtHpi::update(const MatrixXd& data)
{
    if (data.rows() != m_ring.rows()) {
        Log::warning("[RtHpi::update] Ignoring a block of {} channels; the measurement info has {}.", data.rows(), m_ring.rows());
        return std::nullopt;
    }
    const Index window = m_ring.cols();
    const Index n = data.cols();
    const Index kept = std::min(n, window);
    for (Index done = 0; done < kept;) {
        const Index chunk = std::min(kept - done, window - m_next);
        m_ring.middleCols(m_next, chunk) = data.middleCols(n - kept + done, chunk);
        m_next = (m_next + chunk) % window;
        done += chunk;
    }
    m_filled = std::min(window, m_filled + n);
    m_sinceFit += n;
    if (m_filled < window || m_sinceFit < m_settings.hop) {
        return std::nullopt;
    }
    if (m_settings.coil_frequencies.size() < 3) {
        Log::warning("[RtHpi::update] At least three coil frequencies are needed to fit the head position.");
        return std::nullopt;
    }
    m_sinceFit = 0;

    // The oldest sample sits at m_next
    MatrixXd samples(m_ring.rows(), window);
    samples.leftCols(window - m_next) = m_ring.rightCols(window - m_next);
    samples.rightCols(m_next) = m_ring.leftCols(m_next);
    m_updater.prepareDataAndProjectors(samples, m_projectors);

    const InvHpiModelParameters model(m_settings.coil_frequencies, m_info->sfreq, m_info->linefreq, false);
    HpiFitResult result = m_last;
    m_fit.fit(m_updater.getProjectedData(), m_updater.getProjectors(), model, m_updater.getHpiDigitizer(), std::exchange(m_order, false), result);
    if (!result.hpiFreqs.empty()) {
        m_settings.coil_frequencies = result.hpiFreqs;
    }
    if (accepted(result)) {
        m_last = result;
    }
    return result;
}

//=============================================================================================================

bool RtHpi::accepted(const HpiFitResult& fit) const
{
    if (fit.errorDistances.empty()) {
        return false;
    }
    const double mean = std::accumulate(fit.errorDistances.begin(), fit.errorDistances.end(), 0.0) / static_cast<double>(fit.errorDistances.size());
    return mean < m_settings.max_error;
}

//=============================================================================================================

void RtHpi::set_max_error(double maxError)
{
    m_settings.max_error = maxError;
}

//=============================================================================================================

const FiffCoordTrans& RtHpi::head_transform() const
{
    return m_last.errorDistances.empty() ? m_initial : m_last.devHeadTrans;
}

//=============================================================================================================

void RtHpi::set_projectors(MatrixXd projectors)
{
    m_projectors = std::move(projectors);
}

//=============================================================================================================

void RtHpi::set_coil_frequencies(std::vector<int> frequencies)
{
    m_settings.coil_frequencies = std::move(frequencies);
}

//=============================================================================================================

const std::vector<int>& RtHpi::coil_frequencies() const
{
    return m_settings.coil_frequencies;
}

//=============================================================================================================

void RtHpi::order_coil_frequencies()
{
    m_order = true;
}

//=============================================================================================================

void RtHpi::set_info(std::shared_ptr<FiffInfo> info)
{
    m_info = std::move(info);
    m_updater.checkForUpdate(m_info);
    m_fit.checkForUpdate(m_updater.getSensors());
}

//=============================================================================================================

void RtHpi::set_window(int window, int hop)
{
    m_settings.window = std::max(1, window);
    m_settings.hop = std::clamp(hop, 1, m_settings.window);
    m_ring = MatrixXd::Zero(m_info->nchan, m_settings.window);
    m_next = 0;
    m_filled = 0;
    m_sinceFit = 0;
}

//=============================================================================================================

void RtHpi::reset()
{
    set_window(m_settings.window, m_settings.hop);
    m_last = HpiFitResult();
}
