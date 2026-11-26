//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     spectrum_table.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of SpectrumTable.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrum_table.h"
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void SpectrumTable::set_info(std::shared_ptr<FiffInfo> info)
{
    m_info = std::move(info);
    reset_selection();
}

//=============================================================================================================

void SpectrumTable::set_data(const MatrixXd& data)
{
    m_data = data;

    if (m_freqScale.size() == m_data.cols() || !m_info) {
        return;
    }

    const double freqRes = (m_info->sfreq / 2.0) / static_cast<double>(m_data.cols());
    m_freqScale.resize(m_data.cols());

    double freq = 0.0;
    for (Index i = 0; i < m_data.cols(); ++i) {
        m_freqScale[i] = m_scale == Scale::Log ? std::log10(freq + 1.0) : freq;
        freq += freqRes;
    }

    m_freqScale /= m_freqScale.maxCoeff();

    m_freqScaleBound = m_freqScale;
    m_lowerBin = 0;
    m_upperBin = static_cast<int>(m_freqScale.size()) - 1;
    m_initialized = true;
}

//=============================================================================================================

int SpectrumTable::channel(int row) const
{
    const auto it = m_selection.find(row);
    return it == m_selection.end() ? 0 : it->second;
}

//=============================================================================================================

std::string SpectrumTable::channel_name(int row) const
{
    const int ch = channel(row);
    if (!m_info || ch < 0 || ch >= std::ssize(m_info->chs)) {
        return {};
    }
    return m_info->chs[static_cast<std::size_t>(ch)].ch_name;
}

//=============================================================================================================

std::optional<RowVectorXd> SpectrumTable::spectrum(int row) const
{
    const MatrixXd& data = m_frozen ? m_frozenData : m_data;
    const int ch = channel(row);
    // Rows exist from set_info() on; the spectrum only once data has arrived
    if (ch < 0 || ch >= data.rows()) {
        return std::nullopt;
    }
    return RowVectorXd(data.row(ch));
}

//=============================================================================================================

void SpectrumTable::select(const std::vector<int>& channels)
{
    m_selection.clear();
    const auto available = m_info ? std::ssize(m_info->chs) : 0;

    int row = 0;
    for (int ch : channels) {
        if (ch < available) {
            m_selection.insert_or_assign(row++, ch);
        }
    }
}

//=============================================================================================================

void SpectrumTable::reset_selection()
{
    m_selection.clear();
    if (!m_info) {
        return;
    }
    for (int i = 0; i < std::ssize(m_info->chs); ++i) {
        m_selection.insert_or_assign(i, i);
    }
}

//=============================================================================================================

void SpectrumTable::toggle_frozen()
{
    m_frozen = !m_frozen;
    if (m_frozen) {
        m_frozenData = m_data;
    }
}

//=============================================================================================================

bool SpectrumTable::set_bounds(float lower, float upper)
{
    if (!m_initialized || !m_info) {
        return false;
    }

    const double nyquist = m_info->sfreq / 2.0;

    m_lowerBin = 0;
    m_upperBin = static_cast<int>(m_freqScale.size()) - 1;

    for (Index i = 0; i < m_freqScale.size(); ++i) {
        const auto freq = static_cast<float>(m_freqScale[i] * nyquist);
        if (freq < lower) {
            m_lowerBin = static_cast<int>(i);
        }
        if (freq > upper) {
            m_upperBin = static_cast<int>(i);
            break;
        }
    }

    const double low = m_freqScale[m_lowerBin];
    const double span = m_freqScale[m_upperBin] - low;
    m_freqScaleBound = (m_freqScale.array() - low) / span;
    return true;
}
