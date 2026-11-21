//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     rt_averaging.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>
 * @since    0.1.3
 * @date     July, 2012
 * @brief    Definition of the RtAveraging class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_averaging.h"

#include <base/log.h>
#include <math/numerics.h>
#include <mne/mne_epoch_data_list.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtAveraging::RtAveraging(std::shared_ptr<const FiffInfo> info, Settings settings)
: m_info(std::move(info))
{
    settings.post_stim = std::max(settings.post_stim, 1);
    settings.pre_stim = std::max(settings.pre_stim, 0);
    settings.averages = std::max(settings.averages, 1);
    m_settings = std::move(settings);
    reset();
}

//=============================================================================================================

std::vector<int> RtAveraging::update(const Ref<const MatrixXd>& block)
{
    if (block.rows() != std::ssize(m_info->chs)) {
        Log::warning("[RtAveraging::update] Ignoring a block of {} channels; the measurement info has {}.", block.rows(), m_info->chs.size());
        return {};
    }
    const Index n = block.cols();
    const Index pre = m_settings.pre_stim;
    const Index length = pre + m_settings.post_stim;

    for (PendingEpoch& epoch : m_pending) {
        const Index take = std::min(length - epoch.filled, n);
        epoch.data.middleCols(epoch.filled, take) = block.leftCols(take);
        epoch.filled += take;
    }

    const int trigger = m_settings.trigger_channel;
    if (trigger >= 0 && trigger < block.rows() && n > 0) {
        if (m_samples == 0) {
            m_last_trigger = block(trigger, 0); // like mne.find_events, a level at the first sample is no onset
        }
        for (Index j = 0; j < n; ++j) {
            const double value = block(trigger, j);
            const bool onset = m_last_trigger < m_settings.trigger_threshold && value >= m_settings.trigger_threshold;
            m_last_trigger = value;
            if (!onset || m_samples + j < pre) {
                continue;
            }
            // The epoch starts at column j of the history followed by the block
            PendingEpoch& epoch = m_pending.emplace_back(static_cast<int>(std::lround(value)), MatrixXd(block.rows(), length), 0);
            if (j < pre) {
                epoch.data.leftCols(pre - j) = m_history.rightCols(pre - j);
                epoch.filled = pre - j;
            }
            const Index take = std::min(length - epoch.filled, n - std::max<Index>(j - pre, 0));
            epoch.data.middleCols(epoch.filled, take) = block.middleCols(std::max<Index>(j - pre, 0), take);
            epoch.filled += take;
        }
    }

    std::vector<int> changed;
    for (PendingEpoch& epoch : m_pending) {
        if (epoch.filled == length && add_epoch(epoch)) {
            changed.push_back(epoch.code);
        }
    }
    std::erase_if(m_pending, [length](const PendingEpoch& epoch) { return epoch.filled == length; });

    if (pre > 0) {
        if (n >= pre) {
            m_history = block.rightCols(pre);
        } else {
            // Columns are contiguous, so a forward copy shifts the history left in place
            std::copy(m_history.data() + n * m_history.rows(), m_history.data() + m_history.size(), m_history.data());
            m_history.rightCols(n) = block;
        }
    }
    m_samples += n;

    std::ranges::sort(changed);
    const auto [first, last] = std::ranges::unique(changed);
    changed.erase(first, last);
    return changed;
}

//=============================================================================================================

const FiffEvokedSet& RtAveraging::evoked() const
{
    return m_evoked;
}

//=============================================================================================================

const RtAveraging::Settings& RtAveraging::settings() const
{
    return m_settings;
}

//=============================================================================================================

void RtAveraging::set_settings(Settings settings)
{
    settings.post_stim = std::max(settings.post_stim, 1);
    settings.pre_stim = std::max(settings.pre_stim, 0);
    settings.averages = std::max(settings.averages, 1);
    const bool redefined = settings.pre_stim != m_settings.pre_stim || settings.post_stim != m_settings.post_stim || settings.trigger_channel != m_settings.trigger_channel || settings.trigger_threshold != m_settings.trigger_threshold;
    m_settings = std::move(settings);
    if (redefined) {
        reset();
        return;
    }
    for (auto& [code, average] : m_averages) {
        trim(average);
        publish(code, average);
    }
}

//=============================================================================================================

void RtAveraging::reset()
{
    const Index pre = m_settings.pre_stim;
    const Index length = pre + m_settings.post_stim;
    m_evoked.evoked.clear();
    m_evoked.info = *m_info;
    m_times = RowVectorXf::LinSpaced(length, static_cast<float>(-pre), static_cast<float>(length - pre - 1)) / m_info->sfreq;
    m_averages.clear();
    m_pending.clear();
    m_history = MatrixXd::Zero(std::ssize(m_info->chs), pre);
    m_samples = 0;
    m_last_trigger = 0.0;
}

//=============================================================================================================

bool RtAveraging::add_epoch(PendingEpoch& epoch)
{
    if (!m_settings.reject.empty() && MNEEpochDataList::checkForArtifact(epoch.data, *m_info, m_settings.reject)) {
        return false;
    }
    Average& average = m_averages[epoch.code];
    if (average.epochs.empty()) {
        average.sum = epoch.data;
    } else {
        average.sum += epoch.data;
    }
    average.epochs.push_back(std::move(epoch.data));
    trim(average);
    publish(epoch.code, average);
    return true;
}

//=============================================================================================================

void RtAveraging::trim(Average& average)
{
    while (std::ssize(average.epochs) > m_settings.averages) {
        average.sum -= average.epochs.front();
        average.epochs.pop_front();
        ++average.removed;
    }
    // Summing up afresh once per window keeps the moving sum from drifting
    if (average.removed >= m_settings.averages) {
        average.sum = average.epochs.front();
        for (auto epoch = std::next(average.epochs.begin()); epoch != average.epochs.end(); ++epoch) {
            average.sum += *epoch;
        }
        average.removed = 0;
    }
}

//=============================================================================================================

void RtAveraging::publish(int code, const Average& average)
{
    const auto index = std::distance(m_averages.begin(), m_averages.find(code));
    auto evoked = m_evoked.evoked.begin() + index;
    const std::string comment = std::to_string(code);
    if (evoked == m_evoked.evoked.end() || evoked->comment != comment) {
        FiffEvoked created;
        created.setInfo(*m_info);
        created.times = m_times;
        created.first = -m_settings.pre_stim;
        created.last = m_settings.post_stim - 1;
        created.comment = comment;
        evoked = m_evoked.evoked.insert(evoked, std::move(created));
    }
    evoked->nave = static_cast<int>(average.epochs.size());
    evoked->data = average.sum / static_cast<double>(average.epochs.size());
    const auto& baseline = m_settings.baseline;
    evoked->baseline = baseline.value_or(std::pair(-1.0f, -1.0f));
    if (baseline && baseline->first != baseline->second) {
        evoked->data = Numerics::rescale(evoked->data, evoked->times, *baseline, "mean");
    }
}
