//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_convert.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    decode() native samples into caller-owned matrices and encode() matrices into output rings.
 *
 * One memory pass: format conversion, layout transposition and per-channel scaling are fused and
 * cache-blocked, written as fixed-width loops the compiler vectorises (no intrinsics, no Eigen, so the
 * same code builds for embedded targets). Converters never allocate.
 *
 * Arithmetic is defined, so results are bit-exact against the scalar reference used in the tests:
 * float targets compute float(raw) * float(scale) + float(offset) without FMA contraction, double
 * targets the same in double (use double for int32 data beyond 2^24). encode() computes
 * (value - offset) / scale in double, saturates to the channel range and to the native type, and
 * rounds to nearest, ties to even; NaN encodes as 0. Both count what they could not represent.
 * Channels with AcqChannelInfo::digitalBits = d use raw = signExtend(low W-d bits) << d on decode (W = word width)
 * and keep the top d bits untouched on encode (value quantised to W-d bits).
 *
 * A channel range lets several threads convert disjoint channel blocks of the same view in parallel.
 */

#ifndef ACQ_CONVERT_H
#define ACQ_CONVERT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_block_view.h"
#include "acq_error.h"
#include "acq_sample_format.h"
#include "acq_stream_info.h"

#include <complex>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Whether converters apply the per-channel scale and offset.
 */
enum class AcqScaling : std::uint8_t
{
    Raw = 0,     /**< Numeric sample value, no scaling. */
    Physical = 1 /**< raw * scale + offset, in the channel's AcqUnit. */
};

//=============================================================================================================
/**
 * Non-owning channels x samples matrix with element strides; describes Eigen (column-major), NumPy,
 * std::vector or device memory alike.
 *
 * @tparam T   float, double or std::complex<float>, optionally const.
 *
 * @snippet ex_acq_oddball/main.cpp acq_epoch_stream
 */
template<typename T>
struct AcqMatrixRef
{
    T* data = nullptr;            /**< Element (0, 0). */
    std::size_t rows = 0;         /**< Channels. */
    std::size_t cols = 0;         /**< Samples. */
    std::ptrdiff_t rowStride = 1; /**< Elements between consecutive channels. */
    std::ptrdiff_t colStride = 0; /**< Elements between consecutive samples. */

    /**
     * @param[in] data   Element (0, 0).
     * @param[in] rows   Channels.
     * @param[in] cols   Samples.
     *
     * @return A view of column-major storage (Eigen default, Fortran order).
     */
    [[nodiscard]] static constexpr AcqMatrixRef columnMajor(T* data, std::size_t rows, std::size_t cols) noexcept
    {
        return {data, rows, cols, 1, static_cast<std::ptrdiff_t>(rows)};
    }

    /**
     * @param[in] data   Element (0, 0).
     * @param[in] rows   Channels.
     * @param[in] cols   Samples.
     *
     * @return A view of row-major storage (C order, one contiguous row per channel).
     */
    [[nodiscard]] static constexpr AcqMatrixRef rowMajor(T* data, std::size_t rows, std::size_t cols) noexcept
    {
        return {data, rows, cols, static_cast<std::ptrdiff_t>(cols), 1};
    }
};

//=============================================================================================================
/**
 * Contiguous channel subset [first, first + count) of a view; the matrix rows map to these channels.
 */
struct AcqChannelRange
{
    std::size_t first = 0;                                       /**< First channel. */
    std::size_t count = std::numeric_limits<std::size_t>::max(); /**< Number of channels; max = to the end. */
};

//=============================================================================================================
/**
 * Placement of a conversion inside the matrix.
 */
struct AcqConvertOptions
{
    AcqScaling scaling = AcqScaling::Physical; /**< Raw or physical values. */
    AcqChannelRange channels;                  /**< Channels of the view mapped to rows 0..count-1. */
    std::size_t firstColumn = 0;               /**< Matrix column of the view's first sample. */
};

//=============================================================================================================
/**
 * Decodes all samples of @p view into @p out.
 *
 * @param[in] view      Source view.
 * @param[out] out      Destination; rows >= channel count, cols >= firstColumn + view.sampleCount().
 * @param[in] options   Scaling, channel subset, first column.
 *
 * @return ShapeMismatch, UnsupportedFormat (complex view into a real matrix), DeviceMemory, or success.
 */
ACQSHARED_EXPORT AcqError decode(const AcqBlockView& view, const AcqMatrixRef<float>& out,
                                 const AcqConvertOptions& options = {}) noexcept;

/** @copydoc decode(const AcqBlockView&, const AcqMatrixRef<float>&, const AcqConvertOptions&) */
ACQSHARED_EXPORT AcqError decode(const AcqBlockView& view, const AcqMatrixRef<double>& out,
                                 const AcqConvertOptions& options = {}) noexcept;

/** @copydoc decode(const AcqBlockView&, const AcqMatrixRef<float>&, const AcqConvertOptions&) Real formats yield a zero imaginary part. */
ACQSHARED_EXPORT AcqError decode(const AcqBlockView& view, const AcqMatrixRef<std::complex<float>>& out,
                                 const AcqConvertOptions& options = {}) noexcept;

//=============================================================================================================
/**
 * Result of encode().
 *
 * @snippet ex_acq_scope/main.cpp acq_scope_stimulate
 */
struct AcqEncodeResult
{
    std::uint64_t clipped = 0; /**< Elements saturated to the channel range or native type, or NaN. */
};

//=============================================================================================================
/**
 * Encodes @p in into the reserved space of @p view (an output stream, or an input ring written by a
 * backend that receives physical values).
 *
 * @param[in] in        Source; rows = channel count, cols >= firstColumn + view.sampleCount().
 * @param[in] view      Destination view from AcqWriter::beginWrite().
 * @param[in] options   Scaling, channel subset, first source column.
 *
 * @return Clipping count, or ShapeMismatch / UnsupportedFormat / DeviceMemory.
 */
ACQSHARED_EXPORT AcqExpected<AcqEncodeResult> encode(const AcqMatrixRef<const float>& in, const AcqWriteView& view,
                                                     const AcqConvertOptions& options = {}) noexcept;

/** @copydoc encode(const AcqMatrixRef<const float>&, const AcqWriteView&, const AcqConvertOptions&) */
ACQSHARED_EXPORT AcqExpected<AcqEncodeResult> encode(const AcqMatrixRef<const double>& in, const AcqWriteView& view,
                                                     const AcqConvertOptions& options = {}) noexcept;

/** @copydoc encode(const AcqMatrixRef<const float>&, const AcqWriteView&, const AcqConvertOptions&) For real formats the imaginary part must be 0, otherwise the element counts as clipped. */
ACQSHARED_EXPORT AcqExpected<AcqEncodeResult> encode(const AcqMatrixRef<const std::complex<float>>& in,
                                                     const AcqWriteView& view,
                                                     const AcqConvertOptions& options = {}) noexcept;

//=============================================================================================================
/**
 * Extracts the AcqStreamInfo::bitFields of @p view; row f of @p out holds field f for every sample.
 *
 * @param[in] view          Source view of an integer stream.
 * @param[in] fields        Bit fields of the stream's info version.
 * @param[out] out          fields.size() x at least firstColumn + view.sampleCount().
 * @param[in] firstColumn   Matrix column of the view's first sample.
 *
 * @return ShapeMismatch, UnsupportedFormat (non-integer format), InvalidParameter (a field outside its
 *         channel's digitalBits), DeviceMemory, or success.
 */
ACQSHARED_EXPORT AcqError decodeBits(const AcqBlockView& view, std::span<const AcqBitField> fields,
                                     const AcqMatrixRef<std::uint32_t>& out, std::size_t firstColumn = 0) noexcept;

//=============================================================================================================
/**
 * Writes bit fields into the reserved samples of @p view, leaving the analog bits untouched; call before
 * or after encode() of the same view. Values wider than a field are truncated and counted as clipped.
 *
 * @param[in] in            fields.size() x at least firstColumn + view.sampleCount().
 * @param[in] fields        Bit fields of the stream's info version.
 * @param[in] view          Destination view.
 * @param[in] firstColumn   Matrix column of the view's first sample.
 *
 * @return Clipping count, or ShapeMismatch / UnsupportedFormat / InvalidParameter / DeviceMemory.
 */
ACQSHARED_EXPORT AcqExpected<AcqEncodeResult> encodeBits(const AcqMatrixRef<const std::uint32_t>& in,
                                                         std::span<const AcqBitField> fields,
                                                         const AcqWriteView& view,
                                                         std::size_t firstColumn = 0) noexcept;

//=============================================================================================================
/**
 * Decodes one raw run, for adopted device buffers that are not read through an AcqReader.
 *
 * @param[in] part      Source run.
 * @param[in] format    Native format of the run.
 * @param[in] scales    Per-channel scale (size = out.rows); ignored for AcqScaling::Raw.
 * @param[in] offsets   Per-channel offset (size = out.rows); ignored for AcqScaling::Raw.
 * @param[out] out      Destination, out.rows channels x part.samples.
 * @param[in] scaling   Raw or physical values.
 *
 * @return ShapeMismatch, UnsupportedFormat, or success.
 */
ACQSHARED_EXPORT AcqError decodePart(const AcqBlockPart& part, AcqSampleFormat format, std::span<const double> scales,
                                     std::span<const double> offsets, const AcqMatrixRef<float>& out,
                                     AcqScaling scaling) noexcept;

} // namespace mne

#endif // ACQ_CONVERT_H
