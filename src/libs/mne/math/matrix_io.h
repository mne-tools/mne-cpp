//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     matrix_io.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ricky <rickytjen@yahoo.com>;
 *           liminsun <liminsun@nmr.mgh.harvard.edu>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Dense Eigen matrices to and from whitespace-separated text.
 *
 * @ref mne::write_matrix and @ref mne::read_matrix round-trip any
 * dense Eigen expression through plain text, one row per line, so
 * intermediate results can be compared with NumPy (@c numpy.loadtxt and
 * @c numpy.savetxt read and write the same layout) and reference data can be
 * checked into a test-data repository. Values are written with enough digits
 * to read back bit-identical; lines containing @c # are comments.
 *
 * @snippet ex_math/main.cpp matrix_io_usage
 */

#ifndef MATHLIB_MATRIX_IO_H
#define MATHLIB_MATRIX_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <istream>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Writes a dense matrix as whitespace-separated text, one row per line.
 *
 * @param[in] matrix      Matrix, vector or any dense Eigen expression to write.
 * @param[in] path        Output file; an existing file is overwritten.
 * @param[in] description If not empty, a comment header with the dimensions and this description is written first.
 *
 * @return True on success, false if the file could not be written.
 */
template<typename Derived>
bool write_matrix(const Eigen::DenseBase<Derived>& matrix, const std::filesystem::path& path, std::string_view description = {})
{
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        mne::Log::warning("write_matrix: cannot open {} for writing", path.string());
        return false;
    }
    if (!description.empty()) {
        file << "# Dimensions (rows x cols): " << matrix.rows() << " x " << matrix.cols() << "\n";
        file << "# Description: " << description << "\n";
    }
    std::string line;
    std::array<char, 64> buffer{};
    for (Eigen::Index row = 0; row < matrix.rows(); ++row) {
        line.clear();
        for (Eigen::Index col = 0; col < matrix.cols(); ++col) {
            if (col > 0) {
                line += ' ';
            }
            // Shortest representation that reads back to the identical value
            const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), matrix(row, col));
            line.append(buffer.data(), result.ptr);
        }
        line += '\n';
        file << line;
    }
    return static_cast<bool>(file);
}

//=============================================================================================================

namespace detail
{

template<typename Scalar, int Rows, int Cols, int Options, int MaxRows, int MaxCols>
bool parse_matrix(Eigen::Matrix<Scalar, Rows, Cols, Options, MaxRows, MaxCols>& out, std::istream& file, std::string_view source)
{
    std::vector<Scalar> values;
    Eigen::Index rows = 0;
    Eigen::Index cols = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find('#') != std::string::npos) {
            continue;
        }
        Eigen::Index count = 0;
        const char* pos = line.data();
        const char* const end = line.data() + line.size();
        while (true) {
            while (pos != end && std::isspace(static_cast<unsigned char>(*pos))) {
                ++pos;
            }
            if (pos == end) {
                break;
            }
            Scalar value{};
            const auto [next, ec] = std::from_chars(pos, end, value);
            if (ec != std::errc{} || (next != end && !std::isspace(static_cast<unsigned char>(*next)))) {
                mne::Log::warning("read_matrix: non-numeric value in row {} of {}", rows + 1, source);
                return false;
            }
            values.push_back(value);
            ++count;
            pos = next;
        }
        if (count == 0) {
            continue;
        }
        if (rows > 0 && count != cols) {
            mne::Log::warning("read_matrix: row {} of {} has {} values, expected {}", rows + 1, source, count, cols);
            return false;
        }
        cols = count;
        ++rows;
    }

    Eigen::Index outRows = rows;
    Eigen::Index outCols = cols;
    if constexpr (Rows == 1) {
        outRows = 1;
        outCols = rows > 0 ? cols : 0;
    } else if constexpr (Cols == 1) {
        outCols = 1;
        outRows = cols > 0 ? rows : 0;
    }
    if ((Rows != Eigen::Dynamic && outRows != Rows) || (Cols != Eigen::Dynamic && outCols != Cols)) {
        mne::Log::warning("read_matrix: {} holds {} x {} values, which do not fit a {} x {} matrix", source, rows, cols, Rows, Cols);
        return false;
    }

    using RowMajor = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    const Eigen::Map<const RowMajor> parsed(values.data(), rows, cols);
    out.resize(outRows, outCols);
    if (out.size() > 0) {
        out = parsed.topLeftCorner(outRows, outCols);
    }
    return true;
}

} // namespace detail

//=============================================================================================================
/**
 * Reads a matrix written by @ref write_matrix (or @c numpy.savetxt). Lines containing @c # and blank lines are
 * skipped. A row-vector target receives the first row of the file, a column-vector target the first column.
 *
 * @param[out] out  Resized and filled with the values read; left unchanged on failure.
 * @param[in]  path Whitespace-separated text file.
 *
 * @return True on success, false if the file cannot be opened, holds a non-numeric value, has rows of
 *         unequal length or does not fit a fixed-size @p out.
 */
template<typename Scalar, int Rows, int Cols, int Options, int MaxRows, int MaxCols>
bool read_matrix(Eigen::Matrix<Scalar, Rows, Cols, Options, MaxRows, MaxCols>& out, const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) {
        mne::Log::warning("read_matrix: cannot open {}", path.string());
        return false;
    }
    return detail::parse_matrix(out, file, path.string());
}

//=============================================================================================================
/**
 * Reads a matrix in the same text layout from a stream, e.g. the captured output of a Python script.
 *
 * @param[out] out    Resized and filled with the values read; left unchanged on failure.
 * @param[in]  stream Whitespace-separated text, one row per line.
 *
 * @return True on success, false if the text holds a non-numeric value, has rows of unequal length or does
 *         not fit a fixed-size @p out.
 */
template<typename Scalar, int Rows, int Cols, int Options, int MaxRows, int MaxCols>
bool read_matrix(Eigen::Matrix<Scalar, Rows, Cols, Options, MaxRows, MaxCols>& out, std::istream& stream)
{
    return detail::parse_matrix(out, stream, "stream");
}

} // namespace mne

#endif // MATHLIB_MATRIX_IO_H
