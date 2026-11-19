//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_eigen.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Optional header-only Eigen adapters for mne_acq: zero-copy maps, read() and write() with Eigen matrices.
 *
 * mne_acq itself depends only on the C++20 standard library. Code that uses Eigen includes this header
 * (and provides Eigen's include path); nothing here is compiled into the library.
 */

#ifndef ACQ_EIGEN_H
#define ACQ_EIGEN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq.h"

#include <Eigen/Core>

#include <cassert>
#include <chrono>
#include <complex>
#include <cstddef>
#include <type_traits>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/** Strided read-only channels x samples map of one part of a view. */
template<typename T>
using AcqEigenMap = Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>, Eigen::Unaligned,
                               Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>;

//=============================================================================================================
/**
 * Maps part @p i of @p view as a channels x samples matrix of the native type, without copy.
 *
 * @tparam T   std::int16_t, std::int32_t, float, double or std::complex<float>; must match view.format().
 *             Packed int24 and complex int16 have no Eigen scalar type; use decode().
 * @param[in] view   An acquired view.
 * @param[in] i      Part index. @pre i < view.partCount()
 *
 * @return Strided map into the ring, valid until the view is released.
 */
template<typename T>
[[nodiscard]] AcqEigenMap<T> map(const AcqBlockView& view, std::size_t i) noexcept
{
    static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, std::complex<float>>,
                  "map<T>() needs an Eigen scalar type; use decode() for int24 and complex int16");
    assert(AcqSampleTraits<T>::format == view.format());
    const AcqBlockPart& p = view.part(i);
    constexpr auto size = static_cast<std::ptrdiff_t>(sizeof(T));
    return AcqEigenMap<T>(reinterpret_cast<const T*>(p.data), static_cast<Eigen::Index>(view.channelCount()),
                          static_cast<Eigen::Index>(p.samples),
                          Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>(p.sampleStride / size, p.channelStride / size));
}

/**
 * @param[in] m   An Eigen matrix.
 *
 * @return An AcqMatrixRef over its storage.
 */
template<typename T>
[[nodiscard]] AcqMatrixRef<T> ref(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& m) noexcept
{
    return AcqMatrixRef<T>::columnMajor(m.data(), static_cast<std::size_t>(m.rows()), static_cast<std::size_t>(m.cols()));
}

/**
 * @param[in] m   An Eigen matrix.
 *
 * @return A read-only AcqMatrixRef over its storage.
 */
template<typename T>
[[nodiscard]] AcqMatrixRef<const T> ref(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& m) noexcept
{
    return AcqMatrixRef<const T>::columnMajor(m.data(), static_cast<std::size_t>(m.rows()),
                                              static_cast<std::size_t>(m.cols()));
}

//=============================================================================================================
/**
 * Reads samples into an Eigen matrix, resizing it only when its shape differs (so a reused matrix does
 * not allocate).
 *
 * @param[in] reader    The reader.
 * @param[in] samples   Samples per channel.
 * @param[out] out      Destination, channels x samples.
 * @param[in] timeout   Maximum wait.
 * @param[in] scaling   Raw or physical values.
 *
 * @return Position and loss of the block, or the reader's error.
 */
template<typename T>
[[nodiscard]] AcqExpected<AcqReadResult> read(AcqReader& reader, std::size_t samples,
                                              Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& out,
                                              std::chrono::nanoseconds timeout,
                                              AcqScaling scaling = AcqScaling::Physical)
{
    const auto rows = static_cast<Eigen::Index>(reader.channelCount());
    const auto cols = static_cast<Eigen::Index>(samples);
    if (out.rows() != rows || out.cols() != cols) {
        out.resize(rows, cols);
    }
    return reader.read(ref(out), timeout, scaling);
}

//=============================================================================================================
/**
 * Writes all columns of an Eigen matrix to an output stream: reserves, encodes, commits.
 *
 * @param[in] writer    The writer of the output stream.
 * @param[in] in        Source, channels x samples.
 * @param[in] timeout   Maximum wait for space.
 * @param[in] scaling   Raw or physical values.
 *
 * @return Clipping count, or the writer's or encoder's error.
 */
template<typename T>
[[nodiscard]] AcqExpected<AcqEncodeResult> write(AcqWriter& writer,
                                                 const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& in,
                                                 std::chrono::nanoseconds timeout,
                                                 AcqScaling scaling = AcqScaling::Physical)
{
    auto view = writer.beginWrite(static_cast<std::size_t>(in.cols()), timeout);
    if (!view) {
        return view.error();
    }
    AcqConvertOptions options;
    options.scaling = scaling;
    auto result = encode(ref(in), *view, options);
    writer.commit(result ? view->sampleCount() : 0);
    return result;
}

} // namespace mne

#endif // ACQ_EIGEN_H
