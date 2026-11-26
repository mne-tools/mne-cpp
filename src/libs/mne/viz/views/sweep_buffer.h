//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sweep_buffer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SweepBuffer, the sweeping window of a real-time raw-data view.
 */

#ifndef VIZ_SWEEP_BUFFER_H
#define VIZ_SWEEP_BUFFER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * Calls @p fn (column, offset, length) for each piece of the span [start, start + length) wrapped into
 * [0, size): column is where the piece lies in the window, offset where it starts in the span.
 *
 * @param[in] start    First position of the span; any integer, taken modulo @p size.
 * @param[in] length   Length of the span.
 * @param[in] size     Size of the window, > 0.
 * @param[in] fn       Called per piece.
 *
 * @since 3.0.0
 */
template<typename Fn>
void for_wrapped_span(int start, int length, int size, Fn fn)
{
    start = ((start % size) + size) % size;
    for (int done = 0; done < length;) {
        const int piece = std::min(length - done, size - start);
        fn(start, done, piece);
        done += piece;
        start = 0;
    }
}

/**
 * @brief The sweeping window of a real-time raw-data view: new samples overwrite the oldest from the left.
 *
 * Holds a raw and a filtered layer of channels x window samples, written column by column at the write
 * position, which wraps to column 0 at the end of the window and starts a new sweep. Left of the write
 * position is the current sweep, from it on the previous one. Freezing keeps a copy of both layers and of
 * the write position for display while writing goes on.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SweepBuffer
{
public:
    /** A layer: channels x window samples, row-major so that a channel's samples are contiguous. */
    using Matrix = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    //=========================================================================================================
    /**
     * Resizes the layers, keeping the stored values that remain. If the write position lies beyond the new
     * window, a new sweep starts.
     *
     * @param[in] channels   Number of channels.
     * @param[in] window     Samples per sweep.
     * @param[in] zero       Zero the layers and the first values.
     */
    void resize(int channels, int window, bool zero);

    /** @return Samples per sweep (1024 until resized). */
    int window() const
    {
        return m_window;
    }

    //=========================================================================================================
    /**
     * Writes @p count samples at the write position: calls @p on_new_sweep when the write position wraps to
     * column 0 (after the sweep has started) and @p piece (column, offset, length) for each contiguous piece,
     * which writes the samples [offset, offset + length) of the block to the layers at column.
     *
     * @param[in] count          Samples in the block.
     * @param[in] on_new_sweep   Called when a new sweep starts.
     * @param[in] piece          Writes a piece of the block.
     */
    template<typename OnNewSweep, typename Piece>
    void write(int count, OnNewSweep on_new_sweep, Piece piece)
    {
        const int size = static_cast<int>(m_raw.cols());
        if (size == 0)
            return;
        for_wrapped_span(m_writePosition, count, size, [&](int column, int offset, int length) {
            if (column == 0 && m_writePosition == size) {
                start_sweep();
                on_new_sweep();
            }
            piece(column, offset, length);
            m_writePosition = column + length;
        });
        m_blockSize = std::min(count, size);
    }

    /** @return The raw layer being written. */
    Matrix& raw()
    {
        return m_raw;
    }

    /** @return The filtered layer being written. */
    Matrix& filtered()
    {
        return m_filtered;
    }

    //=========================================================================================================
    /**
     * Returns the layer to display: the frozen copy while frozen.
     *
     * @param[in] filtered   The filtered layer, else the raw one.
     *
     * @return The layer.
     */
    const Matrix& layer(bool filtered) const;

    //=========================================================================================================
    /**
     * Returns the last written block, unwrapped.
     *
     * @param[in] filtered   From the filtered layer, else from the raw one.
     *
     * @return Channels x block samples.
     */
    Eigen::MatrixXd last_block(bool filtered) const;

    //=========================================================================================================
    /**
     * Returns a channel's value in column 0 when the current sweep started (kept while frozen).
     *
     * @param[in] row        Channel.
     * @param[in] filtered   Of the filtered layer, else of the raw one.
     *
     * @return The value, 0 for an unknown channel.
     */
    double first_value(int row, bool filtered) const;

    //=========================================================================================================
    /**
     * Returns the absolute sample shown at a column of the window.
     *
     * @param[in] column   Column of the window.
     *
     * @return The sample, -1 outside the window or before the first sample.
     */
    int sample_at_column(int column) const;

    //=========================================================================================================
    /**
     * Freezes or thaws the display; freezing copies the layers and the write position.
     *
     * @param[in] frozen   Whether to freeze.
     */
    void set_frozen(bool frozen);

    /** @return Whether the display is frozen. */
    bool frozen() const
    {
        return m_frozen;
    }

    /** @return The column the next sample is written to (the window size at the end of a sweep). */
    int write_position() const
    {
        return m_writePosition;
    }

    /** @return The write position when the display was frozen. */
    int frozen_write_position() const
    {
        return m_frozenWritePosition;
    }

    /** @return The absolute sample at column 0 of the current sweep. */
    int sweep_start() const
    {
        return m_sweepStart;
    }

    /** @return The number of samples of the last written block (at most the window). */
    int block_size() const
    {
        return m_blockSize;
    }

    /** Zeroes the layers, their frozen copies and the first values. */
    void clear();

private:
    void start_sweep();

    int m_window = 1024;
    int m_writePosition = 0;
    int m_sweepStart = 0;
    int m_frozenWritePosition = 0;
    int m_blockSize = 0;
    bool m_frozen = false;

    Matrix m_raw;
    Matrix m_filtered;
    Matrix m_frozenRaw;
    Matrix m_frozenFiltered;
    Eigen::VectorXd m_firstRaw;
    Eigen::VectorXd m_firstFiltered;
};

} // namespace mne::viz

#endif // VIZ_SWEEP_BUFFER_H
