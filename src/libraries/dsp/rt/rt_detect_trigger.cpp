//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     rt_detect_trigger.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>
 * @since    0.1.3
 * @date     July, 2015
 * @brief    DetectTrigger definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_detect_trigger.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;

namespace
{

// Reports every sample of values that reaches the threshold, then skips the rest of the burst
std::vector<mne::TriggerFlank> find_flanks(const RowVectorXd& values, int offset, double threshold, int burstLength)
{
    std::vector<mne::TriggerFlank> flanks;
    for (Index j = 0; j < values.size(); ++j) {
        if (values(j) >= threshold) {
            flanks.push_back({offset + static_cast<int>(j), values(j)});
            j += burstLength;
        }
    }
    return flanks;
}

bool has_row(const MatrixXd& data, int channel)
{
    return channel >= 0 && channel < data.rows();
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::vector<MatrixXi> mne::to_event_matrix(const std::map<int, std::vector<TriggerFlank>>& triggers)
{
    std::vector<MatrixXi> events;
    events.reserve(triggers.size());
    for (const auto& [channel, flanks] : triggers) {
        MatrixXi& matrix = events.emplace_back(MatrixXi::Zero(std::ssize(flanks), 3));
        for (Index i = 0; i < matrix.rows(); ++i) {
            matrix(i, 0) = flanks[static_cast<std::size_t>(i)].sample;
            matrix(i, 2) = static_cast<int>(flanks[static_cast<std::size_t>(i)].value);
        }
    }
    return events;
}

//=============================================================================================================

std::vector<mne::TriggerFlank> mne::detect_trigger_flanks_max(const MatrixXd& data,
                                                              int channel,
                                                              int offset,
                                                              double threshold,
                                                              bool removeOffset,
                                                              int burstLength)
{
    if (!has_row(data, channel) || data.cols() == 0) {
        return {};
    }
    std::vector<TriggerFlank> flanks = find_flanks(removeOffset ? RowVectorXd(data.row(channel).array() - data(channel, 0))
                                                                : RowVectorXd(data.row(channel)),
                                                   offset, threshold, burstLength);
    for (TriggerFlank& flank : flanks) {
        flank.value = data(channel, flank.sample - offset);
    }
    return flanks;
}

//=============================================================================================================

std::map<int, std::vector<mne::TriggerFlank>> mne::detect_trigger_flanks_max(const MatrixXd& data,
                                                                             std::span<const int> channels,
                                                                             int offset,
                                                                             double threshold,
                                                                             bool removeOffset,
                                                                             int burstLength)
{
    std::map<int, std::vector<TriggerFlank>> triggers;
    for (const int channel : channels) {
        triggers[channel] = detect_trigger_flanks_max(data, channel, offset, threshold, removeOffset, burstLength);
    }
    return triggers;
}

//=============================================================================================================

std::vector<mne::TriggerFlank> mne::detect_trigger_flanks_grad(const MatrixXd& data,
                                                               int channel,
                                                               int offset,
                                                               double threshold,
                                                               FlankDirection direction,
                                                               int burstLength)
{
    if (!has_row(data, channel)) {
        return {};
    }
    RowVectorXd gradient = RowVectorXd::Zero(data.cols());
    if (data.cols() > 1) {
        gradient.tail(data.cols() - 1) = data.row(channel).tail(data.cols() - 1) - data.row(channel).head(data.cols() - 1);
    }
    if (direction == FlankDirection::Falling) {
        gradient = -gradient;
    }
    return find_flanks(gradient, offset, threshold, burstLength);
}

//=============================================================================================================

std::map<int, std::vector<mne::TriggerFlank>> mne::detect_trigger_flanks_grad(const MatrixXd& data,
                                                                              std::span<const int> channels,
                                                                              int offset,
                                                                              double threshold,
                                                                              FlankDirection direction,
                                                                              int burstLength)
{
    std::map<int, std::vector<TriggerFlank>> triggers;
    for (const int channel : channels) {
        triggers[channel] = detect_trigger_flanks_grad(data, channel, offset, threshold, direction, burstLength);
    }
    return triggers;
}
