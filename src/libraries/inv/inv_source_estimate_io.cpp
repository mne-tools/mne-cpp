//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_source_estimate_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of the CSV / matrix exporters and importer in @ref mne::InvSourceEstimateIO.
 *
 * Numbers are written in the shortest form that reads back to the same
 * value, so a CSV round trip is exact. The reader refuses a file whose rows
 * do not all carry one value per vertex instead of silently dropping them.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_source_estimate_io.h"
#include <base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <charconv>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

void put_number(std::ofstream& out, double value)
{
    std::array<char, 32> buffer{};
    const auto [end, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    out.write(buffer.data(), end - buffer.data());
}

//=============================================================================================================
/**
 * Splits @p line at @p delim, skipping empty fields, and parses every field
 * as a number. Returns false if a field is not a number.
 */
template<typename T>
bool parse_fields(std::string_view line, char delim, std::vector<T>& values)
{
    values.clear();
    while (!line.empty()) {
        const std::size_t cut = line.find(delim);
        std::string_view field = line.substr(0, cut);
        line = (cut == std::string_view::npos) ? std::string_view{} : line.substr(cut + 1);
        while (!field.empty() && (field.front() == ' ' || field.front() == '\t' || field.front() == '\r')) {
            field.remove_prefix(1);
        }
        while (!field.empty() && (field.back() == ' ' || field.back() == '\t' || field.back() == '\r')) {
            field.remove_suffix(1);
        }
        if (field.empty()) {
            continue;
        }
        T value{};
        const auto [ptr, ec] = std::from_chars(field.data(), field.data() + field.size(), value);
        if (ec != std::errc{} || ptr != field.data() + field.size()) {
            return false;
        }
        values.push_back(value);
    }
    return true;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool InvSourceEstimateIO::write_csv(const InvSourceEstimate& stc,
                                    const std::filesystem::path& path,
                                    char delim)
{
    if (stc.empty()) {
        mne::Log::warning("[InvSourceEstimateIO::write_csv] Source estimate is empty.");
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        mne::Log::warning("[InvSourceEstimateIO::write_csv] Cannot open: {}", path.string());
        return false;
    }

    // Header: time, vertex_0, vertex_1, ...
    out << "time";
    for (Index v = 0; v < stc.vertices.size(); ++v) {
        out << delim << stc.vertices(v);
    }
    out << '\n';

    // Data rows: one per time point
    for (Index t = 0; t < stc.data.cols(); ++t) {
        put_number(out, static_cast<double>(stc.tmin) + static_cast<double>(t) * static_cast<double>(stc.tstep));
        for (Index v = 0; v < stc.data.rows(); ++v) {
            out << delim;
            put_number(out, stc.data(v, t));
        }
        out << '\n';
    }

    return static_cast<bool>(out.flush());
}

//=============================================================================================================

InvSourceEstimate InvSourceEstimateIO::read_csv(const std::filesystem::path& path,
                                                char delim)
{
    std::ifstream in(path);
    if (!in) {
        mne::Log::warning("[InvSourceEstimateIO::read_csv] Cannot open: {}", path.string());
        return {};
    }

    // Header: "time" followed by the vertex indices
    std::string line;
    std::getline(in, line);
    const std::string_view header(line);
    const std::size_t first = header.find(delim);
    std::vector<int> header_vertices;
    if (first == std::string_view::npos || !parse_fields(header.substr(first + 1), delim, header_vertices) || header_vertices.empty()) {
        mne::Log::warning("[InvSourceEstimateIO::read_csv] Invalid header.");
        return {};
    }
    const auto n_vertices = static_cast<Index>(header_vertices.size());

    // Data rows: time followed by one value per vertex
    std::vector<double> values;
    std::vector<double> fields;
    std::vector<double> times;
    while (std::getline(in, line)) {
        if (!parse_fields(std::string_view(line), delim, fields)) {
            mne::Log::warning("[InvSourceEstimateIO::read_csv] Malformed row {}.", times.size() + 1);
            return {};
        }
        if (fields.empty()) {
            continue;
        }
        if (static_cast<Index>(fields.size()) != n_vertices + 1) {
            mne::Log::warning("[InvSourceEstimateIO::read_csv] Row {} has {} values, expected {}.",
                              times.size() + 1, fields.size(), n_vertices + 1);
            return {};
        }
        times.push_back(fields.front());
        values.insert(values.end(), fields.begin() + 1, fields.end());
    }

    if (times.empty()) {
        return {};
    }

    const MatrixXd data = Map<const MatrixXd>(values.data(), n_vertices, static_cast<Index>(times.size()));
    const auto tmin = static_cast<float>(times.front());
    const float tstep = (times.size() > 1) ? static_cast<float>(times[1] - times[0]) : 0.001f;

    return InvSourceEstimate(data, Map<const VectorXi>(header_vertices.data(), n_vertices), tmin, tstep);
}

//=============================================================================================================

bool InvSourceEstimateIO::write_matrix(const InvSourceEstimate& stc,
                                       const std::filesystem::path& path)
{
    if (stc.empty()) {
        mne::Log::warning("[InvSourceEstimateIO::write_matrix] Source estimate is empty.");
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        mne::Log::warning("[InvSourceEstimateIO::write_matrix] Cannot open: {}", path.string());
        return false;
    }

    for (Index v = 0; v < stc.data.rows(); ++v) {
        for (Index t = 0; t < stc.data.cols(); ++t) {
            if (t > 0) {
                out << '\t';
            }
            put_number(out, stc.data(v, t));
        }
        out << '\n';
    }

    return static_cast<bool>(out.flush());
}
