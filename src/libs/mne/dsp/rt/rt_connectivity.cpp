//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     rt_connectivity.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July, 2018
 * @brief    Definition of the RtConnectivity Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_connectivity.h"

#include <mne/connectivity/compute_connectivity.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtConnectivity::RtConnectivity(ConnectivitySettings settings, int trials)
: m_settings(std::move(settings))
, m_trials(trials)
{
    m_settings.set_keep_trial_data(true);
    trim();
}

//=============================================================================================================

std::vector<Network> RtConnectivity::update(const MatrixXd& trial)
{
    if (!m_settings.empty() && (trial.rows() != m_settings.at(0).matData.rows() || trial.cols() != m_settings.at(0).matData.cols())) {
        m_settings.clearAllData();
    }
    m_settings.append(trial);
    trim();
    return compute_connectivity(m_settings);
}

//=============================================================================================================

std::vector<Network> RtConnectivity::networks()
{
    if (m_settings.empty()) {
        return {};
    }
    return compute_connectivity(m_settings);
}

//=============================================================================================================

void RtConnectivity::set_trial_count(int trials)
{
    m_trials = trials;
    trim();
}

//=============================================================================================================

ConnectivitySettings& RtConnectivity::settings()
{
    return m_settings;
}

//=============================================================================================================

void RtConnectivity::reset()
{
    m_settings.clearAllData();
}

//=============================================================================================================

void RtConnectivity::trim()
{
    if (m_trials > 0 && m_settings.size() > m_trials) {
        m_settings.removeFirst(m_settings.size() - m_trials);
    }
}
