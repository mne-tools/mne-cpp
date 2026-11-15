//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_source_estimate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::InvSourceEstimate including STC / W binary I/O and label-based subset access.
 *
 * Provides the constructors, assignment operators, deep-copy helpers and
 * the @c read / @c write methods for the STC and W binary formats. STC
 * serialisation matches the MNE-Python layout (big-endian 4-byte tmin /
 * tstep, vertex index list, then the row-major data matrix) so round-trip
 * parity with mne-python and mne-c is preserved. Label access uses the
 * vertex index list to materialise per-ROI sub-matrices without copying
 * the full data block.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_source_estimate.h"

#include <base/string_utils.h>

#include <QtGlobal>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;
using mne::ErrorCode;
using mne::Expected;
using mne::fail;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

// Big-endian 4-byte values, as MNE-C and mne-python store .stc and .w files.
template<typename T>
bool readBig(std::istream& in, T& value)
{
    static_assert(sizeof(T) == 4);
    std::array<char, 4> bytes{};
    if (!in.read(bytes.data(), 4)) {
        return false;
    }
    if constexpr (std::endian::native == std::endian::little) {
        std::ranges::reverse(bytes);
    }
    value = std::bit_cast<T>(bytes);
    return true;
}

template<typename T>
void writeBig(std::ostream& out, T value)
{
    static_assert(sizeof(T) == 4);
    auto bytes = std::bit_cast<std::array<char, 4>>(value);
    if constexpr (std::endian::native == std::endian::little) {
        std::ranges::reverse(bytes);
    }
    out.write(bytes.data(), 4);
}

// 3-byte big-endian unsigned integers of .w files
bool readBig24(std::istream& in, std::int32_t& value)
{
    std::array<unsigned char, 3> bytes{};
    if (!in.read(reinterpret_cast<char*>(bytes.data()), 3)) {
        return false;
    }
    value = (std::int32_t{bytes[0]} << 16) | (std::int32_t{bytes[1]} << 8) | std::int32_t{bytes[2]};
    return true;
}

void writeBig24(std::ostream& out, std::int32_t value)
{
    const std::array<char, 3> bytes{static_cast<char>((value >> 16) & 0xFF), static_cast<char>((value >> 8) & 0xFF), static_cast<char>(value & 0xFF)};
    out.write(bytes.data(), 3);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvSourceEstimate::InvSourceEstimate(MatrixXd data, VectorXi vertices, float tmin, float tstep)
: data(std::move(data))
, vertices(std::move(vertices))
, tmin(tmin)
, tstep(tstep)
{
    update_times();
}

//=============================================================================================================

InvSourceEstimate::InvSourceEstimate(const std::filesystem::path& path)
: InvSourceEstimate(read(path).value())
{
}

//=============================================================================================================

void InvSourceEstimate::clear()
{
    *this = InvSourceEstimate();
}

//=============================================================================================================

InvSourceEstimate InvSourceEstimate::reduce(int start, int n) const
{
    InvSourceEstimate reduced = *this;
    reduced.data = data.middleCols(start, n);
    reduced.times = times.segment(start, n);
    reduced.tmin = reduced.times(0);
    return reduced;
}

//=============================================================================================================

Expected<InvSourceEstimate> InvSourceEstimate::read(const std::filesystem::path& path)
{
    const std::string file = mne::utf8FromPath(path);
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return fail(ErrorCode::Io, "cannot read {}", file);
    }
    // Times in ms, the vertex numbers, then the data time point by time point
    InvSourceEstimate stc;
    std::uint32_t nVertices = 0;
    if (!readBig(in, stc.tmin) || !readBig(in, stc.tstep) || !readBig(in, nVertices)) {
        return fail(ErrorCode::Format, "{} is not a source estimate", file);
    }
    stc.tmin /= 1000.0f;
    stc.tstep /= 1000.0f;
    stc.vertices.resize(static_cast<Index>(nVertices));
    for (Index i = 0; i < stc.vertices.size(); ++i) {
        if (!readBig(in, stc.vertices[i])) {
            return fail(ErrorCode::Format, "{} ends inside its vertex list", file);
        }
    }
    std::uint32_t nTimes = 0;
    if (!readBig(in, nTimes)) {
        return fail(ErrorCode::Format, "{} ends before its number of time points", file);
    }
    stc.data.resize(static_cast<Index>(nVertices), static_cast<Index>(nTimes));
    for (Index i = 0; i < stc.data.size(); ++i) {
        float value = 0.0f;
        if (!readBig(in, value)) {
            return fail(ErrorCode::Format, "{} ends inside its data", file);
        }
        stc.data.data()[i] = value;
    }
    stc.update_times();
    return stc;
}

//=============================================================================================================

Expected<void> InvSourceEstimate::write(const std::filesystem::path& path) const
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return fail(ErrorCode::Io, "cannot write {}", mne::utf8FromPath(path));
    }
    writeBig(out, 1000.0f * tmin);
    writeBig(out, 1000.0f * tstep);
    writeBig(out, static_cast<std::uint32_t>(vertices.size()));
    for (const int vertex : vertices) {
        writeBig(out, static_cast<std::uint32_t>(vertex));
    }
    writeBig(out, static_cast<std::uint32_t>(data.cols()));
    for (Index i = 0; i < data.size(); ++i) {
        writeBig(out, static_cast<float>(data.data()[i]));
    }
    if (!out.flush()) {
        return fail(ErrorCode::Io, "cannot write {}", mne::utf8FromPath(path));
    }
    return {};
}

//=============================================================================================================

Expected<void> InvSourceEstimate::write_hemisphere_pair(const std::filesystem::path& base) const
{
    if (nVerticesLh < 0 || nVerticesLh > vertices.size()) {
        return fail(ErrorCode::Format, "the left hemisphere has {} of {} vertices, so the estimate cannot be split into hemispheres", nVerticesLh, vertices.size());
    }
    if (data.rows() != vertices.size()) {
        return fail(ErrorCode::Format, "the data have {} rows for {} vertices", data.rows(), vertices.size());
    }
    // A name from a file dialog may already carry one of the suffixes
    std::string stem = mne::utf8FromPath(base);
    for (const std::string_view suffix : {"-lh.stc", "-rh.stc", ".stc"}) {
        if (stem.size() > suffix.size() && mne::equalsIgnoreCase(std::string_view(stem).substr(stem.size() - suffix.size()), suffix)) {
            stem.resize(stem.size() - suffix.size());
            break;
        }
    }
    const Index nRh = vertices.size() - nVerticesLh;
    const struct
    {
        const char* suffix;
        Index offset;
        Index count;
    } hemispheres[] = {{"-lh.stc", 0, nVerticesLh}, {"-rh.stc", nVerticesLh, nRh}};
    for (const auto& hemi : hemispheres) {
        InvSourceEstimate part(data.middleRows(hemi.offset, hemi.count), vertices.segment(hemi.offset, hemi.count), tmin, tstep);
        if (Expected<void> written = part.write(mne::pathFromUtf8(stem + hemi.suffix)); !written) {
            return written;
        }
    }
    return {};
}

//=============================================================================================================

Expected<InvSourceEstimate> InvSourceEstimate::read_w(const std::filesystem::path& path)
{
    const std::string file = mne::utf8FromPath(path);
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return fail(ErrorCode::Io, "cannot read {}", file);
    }
    // Two padding bytes, the number of vertices, then a 3-byte vertex and a 4-byte value each
    in.ignore(2);
    std::int32_t nVertices = 0;
    if (!readBig24(in, nVertices)) {
        return fail(ErrorCode::Format, "{} is not a .w file", file);
    }
    VectorXi vertices(nVertices);
    MatrixXd values(nVertices, 1);
    for (std::int32_t i = 0; i < nVertices; ++i) {
        float value = 0.0f;
        if (!readBig24(in, vertices[i]) || !readBig(in, value)) {
            return fail(ErrorCode::Format, "{} ends after {} of its {} vertices", file, i, nVertices);
        }
        values(i, 0) = value;
    }
    return InvSourceEstimate(std::move(values), std::move(vertices), 0.0f, 0.0f);
}

//=============================================================================================================

Expected<void> InvSourceEstimate::write_w(const std::filesystem::path& path) const
{
    if (data.size() == 0) {
        return fail(ErrorCode::Format, "the source estimate is empty");
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return fail(ErrorCode::Io, "cannot write {}", mne::utf8FromPath(path));
    }
    out.write("\0\0", 2);
    writeBig24(out, static_cast<std::int32_t>(vertices.size()));
    for (Index i = 0; i < vertices.size(); ++i) {
        writeBig24(out, vertices[i]);
        writeBig(out, static_cast<float>(data(i, 0)));
    }
    if (!out.flush()) {
        return fail(ErrorCode::Io, "cannot write {}", mne::utf8FromPath(path));
    }
    return {};
}

//=============================================================================================================

void InvSourceEstimate::update_times()
{
    times.resize(data.cols());
    for (Index i = 0; i < times.size(); ++i) {
        times[i] = tmin + static_cast<float>(i) * tstep;
    }
}

//=============================================================================================================

VectorXi InvSourceEstimate::indices_by_label(std::span<const FsLabel> labels, bool clustered) const
{
    std::vector<int> rows;
    // Without clustering the vertex numbers restart at the first right-hemisphere row
    int hemi = 0;
    for (Index i = 0; i < vertices.size(); ++i) {
        if (i > 0 && vertices(i) < vertices(i - 1)) {
            hemi = 1;
        }
        const bool inside = std::ranges::any_of(labels, [&](const FsLabel& label) {
            if (clustered) {
                return vertices(i) == label.label_id;
            }
            return label.hemi == hemi && (label.vertices.array() == vertices(i)).any();
        });
        if (inside) {
            rows.push_back(static_cast<int>(i));
        }
    }
    return Map<const VectorXi>(rows.data(), static_cast<Index>(rows.size()));
}
