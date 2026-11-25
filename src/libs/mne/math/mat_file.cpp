//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mat_file.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of write_mat_file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mat_file.h"

#include <mne/base/endian.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <format>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

// MAT-file Level 5 data types and array class
constexpr std::uint32_t miINT8 = 1;
constexpr std::uint32_t miINT32 = 5;
constexpr std::uint32_t miUINT32 = 6;
constexpr std::uint32_t miDOUBLE = 9;
constexpr std::uint32_t miMATRIX = 14;
constexpr std::uint32_t mxDOUBLE_CLASS = 6;

template<typename T>
void writeLittleEndian(std::ostream& out, T value)
{
    value = to_little_endian(value);
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

// Data elements are aligned to 8 bytes
std::uint64_t padded(std::uint64_t numBytes)
{
    return numBytes + (8 - numBytes % 8) % 8;
}

void writePad(std::ostream& out, std::uint64_t numBytes)
{
    const char zeros[8] = {};
    out.write(zeros, static_cast<std::streamsize>(padded(numBytes) - numBytes));
}

void writeTag(std::ostream& out, std::uint32_t dataType, std::uint32_t numBytes)
{
    writeLittleEndian(out, dataType);
    writeLittleEndian(out, numBytes);
}

void writeName(std::ostream& out, std::string_view name)
{
    const auto size = static_cast<std::uint32_t>(name.size());
    if (size <= 4) {
        // Small data element: size and type share the tag, the bytes fill the other 4
        writeLittleEndian(out, (size << 16) | miINT8);
        const char zeros[4] = {};
        out.write(name.data(), size);
        out.write(zeros, 4 - size);
    } else {
        writeTag(out, miINT8, size);
        out.write(name.data(), size);
        writePad(out, size);
    }
}

std::uint64_t nameElementSize(std::string_view name)
{
    return name.size() <= 4 ? 8 : 8 + padded(name.size());
}

void writeMatrix(std::ostream& out, const MatVariable& variable, std::uint32_t payload, std::uint32_t dataSize)
{
    const Eigen::MatrixXd& matrix = variable.value;
    writeTag(out, miMATRIX, payload);

    writeTag(out, miUINT32, 8);
    writeLittleEndian(out, mxDOUBLE_CLASS);
    writeLittleEndian(out, std::uint32_t{0});

    writeTag(out, miINT32, 8);
    writeLittleEndian(out, static_cast<std::int32_t>(matrix.rows()));
    writeLittleEndian(out, static_cast<std::int32_t>(matrix.cols()));

    writeName(out, variable.name);

    writeTag(out, miDOUBLE, dataSize);
    std::vector<double> column(static_cast<std::size_t>(matrix.rows()));
    for (Eigen::Index c = 0; c < matrix.cols(); ++c) {
        for (Eigen::Index r = 0; r < matrix.rows(); ++r) {
            column[static_cast<std::size_t>(r)] = to_little_endian(matrix(r, c));
        }
        out.write(reinterpret_cast<const char*>(column.data()), static_cast<std::streamsize>(column.size() * sizeof(double)));
    }
    writePad(out, dataSize);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Expected<void> mne::write_mat_file(const std::filesystem::path& path, std::string_view creator, std::initializer_list<MatVariable> variables)
{
    constexpr std::uint64_t maxElement = std::numeric_limits<std::uint32_t>::max();
    for (const MatVariable& variable : variables) {
        if (variable.name.empty()) {
            return std::unexpected(Error(ErrorCode::Format, "write_mat_file: a variable has no name"));
        }
        const auto values = static_cast<std::uint64_t>(variable.value.size());
        if (values * 8 > maxElement - 64 - padded(variable.name.size())) {
            return std::unexpected(Error(ErrorCode::Format, std::format("write_mat_file: {} is too large for a MAT-file", variable.name)));
        }
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return std::unexpected(Error(ErrorCode::Io, std::format("cannot open {}", utf8_from_path(path))));
    }

    // 116 bytes of text, 8 bytes subsystem offset, version 0x0100 and the endian indicator 'IM'
    std::string header = std::format("MATLAB 5.0 MAT-file, created by {}", creator);
    header.resize(116, ' ');
    out.write(header.data(), 116);
    const char zeros[8] = {};
    out.write(zeros, 8);
    writeLittleEndian(out, std::uint16_t{0x0100});
    writeLittleEndian(out, std::uint16_t{0x4D49});

    for (const MatVariable& variable : variables) {
        const std::uint64_t dataSize = static_cast<std::uint64_t>(variable.value.size()) * 8;
        const std::uint64_t payload = 16 + 16 + nameElementSize(variable.name) + 8 + padded(dataSize);
        writeMatrix(out, variable, static_cast<std::uint32_t>(payload), static_cast<std::uint32_t>(dataSize));
    }

    if (!out.flush()) {
        return std::unexpected(Error(ErrorCode::Io, std::format("cannot write {}", utf8_from_path(path))));
    }
    return {};
}
