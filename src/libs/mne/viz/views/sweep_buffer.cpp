//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sweep_buffer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of SweepBuffer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sweep_buffer.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void SweepBuffer::resize(int channels, int window, bool zero)
{
    m_window = window;
    m_raw.conservativeResize(channels, window);
    m_filtered.conservativeResize(channels, window);
    m_firstRaw.conservativeResize(channels);
    m_firstFiltered.conservativeResize(channels);

    if (zero) {
        m_raw.setZero();
        m_filtered.setZero();
        m_firstRaw.setZero();
        m_firstFiltered.setZero();
    }

    if (m_writePosition > window) {
        m_sweepStart += m_writePosition;
        m_writePosition = 0;
    }
}

//=============================================================================================================

const SweepBuffer::Matrix& SweepBuffer::layer(bool filtered) const
{
    if (m_frozen)
        return filtered ? m_frozenFiltered : m_frozenRaw;
    return filtered ? m_filtered : m_raw;
}

//=============================================================================================================

Eigen::MatrixXd SweepBuffer::last_block(bool filtered) const
{
    const Matrix& data = filtered ? m_filtered : m_raw;

    Eigen::MatrixXd block(data.rows(), m_blockSize);
    if (m_blockSize > 0) {
        for_wrapped_span(m_writePosition - m_blockSize, m_blockSize, static_cast<int>(data.cols()), [&](int column, int offset, int length) {
            block.middleCols(offset, length) = data.middleCols(column, length);
        });
    }
    return block;
}

//=============================================================================================================

double SweepBuffer::first_value(int row, bool filtered) const
{
    const Eigen::VectorXd& values = filtered ? m_firstFiltered : m_firstRaw;
    if (row < 0 || row >= values.size())
        return 0.0;
    return values[row];
}

//=============================================================================================================

int SweepBuffer::sample_at_column(int column) const
{
    if (column < 0 || column >= m_window)
        return -1;

    // Left of the write position is the current sweep, from it on the previous one
    const int write_position = m_frozen ? m_frozenWritePosition : m_writePosition;
    const int sample = m_sweepStart + column - (column < write_position ? 0 : m_window);
    return sample >= 0 ? sample : -1;
}

//=============================================================================================================

void SweepBuffer::set_frozen(bool frozen)
{
    m_frozen = frozen;
    if (frozen) {
        m_frozenRaw = m_raw;
        m_frozenFiltered = m_filtered;
        m_frozenWritePosition = m_writePosition;
    }
}

//=============================================================================================================

void SweepBuffer::clear()
{
    m_raw.setZero();
    m_filtered.setZero();
    m_frozenRaw.setZero();
    m_frozenFiltered.setZero();
    m_firstRaw.setZero();
    m_firstFiltered.setZero();
}

//=============================================================================================================

void SweepBuffer::start_sweep()
{
    m_sweepStart += m_writePosition;
    m_writePosition = 0;

    if (!m_frozen) {
        m_firstFiltered = m_filtered.col(0);
        m_firstRaw = m_raw.col(0);
    }
}
