//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     network.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           mfarisyahya <mfarisyahya@gmail.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August 2016
 * @brief    Implementation of @ref mne::Network - graph container for one connectivity result, including thresholding, frequency-band averaging and adjacency-matrix extraction.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "network.h"
#include <base/log.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Network::Network(std::string method, double threshold)
: m_method(std::move(method))
, m_weight_range(std::numeric_limits<double>::max(), 0.0)
, m_threshold(threshold)
{
    // Registered once for queued signal/slot connections, not on every construction
    static const bool registered = [] {
        qRegisterMetaType<mne::Network>("mne::Network");
        qRegisterMetaType<std::vector<mne::Network>>("std::vector<mne::Network>");
        return true;
    }();
    Q_UNUSED(registered)
}

//=============================================================================================================

void Network::reserve(std::size_t nodes, std::size_t edges)
{
    m_nodes.reserve(nodes);
    m_edges.reserve(edges);
}

//=============================================================================================================

int Network::add_node(const Vector3f& position)
{
    m_nodes.emplace_back(position);
    return static_cast<int>(m_nodes.size()) - 1;
}

//=============================================================================================================

void Network::add_edge(NetworkEdge edge)
{
    if (edge.source() == edge.target()) {
        return;
    }
    const auto n = static_cast<int>(m_nodes.size());
    if (edge.source() < 0 || edge.source() >= n || edge.target() < 0 || edge.target() >= n) {
        throw std::out_of_range("Network::add_edge - node index out of range");
    }

    const double magnitude = std::fabs(edge.weight());
    m_weight_range.first = std::min(m_weight_range.first, magnitude);
    m_weight_range.second = std::max(m_weight_range.second, magnitude);

    const bool active = magnitude >= m_threshold;
    edge.set_active(active);
    m_active += active ? 1 : 0;

    const int index = static_cast<int>(m_edges.size());
    m_nodes[edge.source()].m_edges.push_back(index);
    m_nodes[edge.target()].m_edges.push_back(index);
    m_edges.push_back(std::move(edge));
}

//=============================================================================================================

std::size_t Network::edge_count(EdgeSet set) const noexcept
{
    return set == EdgeSet::full ? m_edges.size() : m_active;
}

//=============================================================================================================

MatrixXd Network::connectivity_matrix(EdgeSet set, bool mirrored) const
{
    const auto n = static_cast<Index>(m_nodes.size());
    MatrixXd matrix = MatrixXd::Zero(n, n);

    for (const NetworkEdge& edge : m_edges) {
        if (set == EdgeSet::thresholded && !edge.active()) {
            continue;
        }
        matrix(edge.source(), edge.target()) = edge.weight();
        if (mirrored) {
            matrix(edge.target(), edge.source()) = edge.weight();
        }
    }

    return matrix;
}

//=============================================================================================================

bool Network::counts(const NetworkEdge& edge, int node, EdgeSet set, Direction direction) const noexcept
{
    if (set == EdgeSet::thresholded && !edge.active()) {
        return false;
    }
    switch (direction) {
        case Direction::in:
            return edge.target() == node;
        case Direction::out:
            return edge.source() == node;
        case Direction::both:
            break;
    }
    return true;
}

//=============================================================================================================

int Network::degree(int node, EdgeSet set, Direction direction) const
{
    const std::vector<int>& incident = m_nodes[node].m_edges;
    if (set == EdgeSet::full && direction == Direction::both) {
        return static_cast<int>(incident.size());
    }
    return static_cast<int>(std::ranges::count_if(incident, [&](int e) { return counts(m_edges[e], node, set, direction); }));
}

//=============================================================================================================

double Network::strength(int node, EdgeSet set, Direction direction) const
{
    double sum = 0.0;
    for (int e : m_nodes[node].m_edges) {
        if (counts(m_edges[e], node, set, direction)) {
            sum += m_edges[e].weight();
        }
    }
    return sum;
}

//=============================================================================================================

std::pair<int, int> Network::degree_range(EdgeSet set, Direction direction) const
{
    if (m_nodes.empty()) {
        return {0, 0};
    }
    std::pair<int, int> range(std::numeric_limits<int>::max(), 0);
    for (int i = 0; i < static_cast<int>(m_nodes.size()); ++i) {
        const int d = degree(i, set, direction);
        range.first = std::min(range.first, d);
        range.second = std::max(range.second, d);
    }
    return range;
}

//=============================================================================================================

int Network::total_degree(EdgeSet set) const
{
    return 2 * static_cast<int>(edge_count(set));
}

//=============================================================================================================

std::pair<double, double> Network::weight_range(EdgeSet set) const noexcept
{
    if (m_edges.empty()) {
        return {0.0, 0.0};
    }
    if (set == EdgeSet::thresholded) {
        return {m_threshold, m_weight_range.second};
    }
    return m_weight_range;
}

//=============================================================================================================

void Network::set_threshold(double threshold)
{
    m_threshold = threshold;
    m_active = 0;

    for (NetworkEdge& edge : m_edges) {
        const bool active = std::fabs(edge.weight()) >= m_threshold;
        edge.set_active(active);
        m_active += active ? 1 : 0;
    }
}

//=============================================================================================================

void Network::set_frequency_range(float lower, float upper)
{
    if (lower > upper) {
        mne::Log::warning("Network::set_frequency_range - Lower frequency is above the upper frequency. Weights are not recalculated.");
        return;
    }
    if (m_sfreq <= 0.0f) {
        mne::Log::warning("Network::set_frequency_range - Sampling frequency has not been set.");
        return;
    }
    if (upper > m_sfreq / 2.0f) {
        mne::Log::warning("Network::set_frequency_range - Upper frequency is above the Nyquist frequency.");
        return;
    }
    if (m_used_freq_bins <= 0) {
        mne::Log::warning("Network::set_frequency_range - Number of frequency bins has not been set.");
        return;
    }

    const double scale = m_fft_size / (m_sfreq / 2.0);
    const std::pair<int, int> bins(static_cast<int>(lower * scale), static_cast<int>(upper * scale));

    m_frequency_range = {lower, upper};
    m_weight_range = {std::numeric_limits<double>::max(), 0.0};

    for (NetworkEdge& edge : m_edges) {
        edge.set_frequency_bins(bins);
        const double magnitude = std::fabs(edge.weight());
        m_weight_range.first = std::min(m_weight_range.first, magnitude);
        m_weight_range.second = std::max(m_weight_range.second, magnitude);
    }
}

//=============================================================================================================

void Network::normalize()
{
    const double max = m_weight_range.second;
    if (max == 0.0) {
        mne::Log::warning("Network::normalize - All weights are 0; nothing to normalize.");
        return;
    }

    for (NetworkEdge& edge : m_edges) {
        edge.set_weight(edge.weight() / max);
    }

    m_weight_range = {m_weight_range.first / max, 1.0};
    m_threshold /= max;
}
