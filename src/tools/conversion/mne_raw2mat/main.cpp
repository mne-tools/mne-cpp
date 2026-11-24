//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2001
 * @brief    Export raw FIFF data to MATLAB Level 5 MAT file format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/base/command_line.h>
#include <mne/base/endian.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <fstream>
#include <ostream>
#include <print>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

// MAT-file Level 5 constants
static const std::uint32_t miDOUBLE = 9;
static const std::uint32_t miINT32 = 5;
static const std::uint32_t miUINT32 = 6;
static const std::uint32_t miINT8 = 1;
static const std::uint32_t miMATRIX = 14;
static const std::uint32_t mxDOUBLE_CLASS = 6;

//=============================================================================================================
// MAT-file writing helpers
//=============================================================================================================

/** Writes @p value little-endian. */
template<typename T>
static void writeLittleEndian(std::ostream& ds, T value)
{
    value = mne::to_little_endian(value);
    ds.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

static void writePad(std::ostream& ds, int numBytes)
{
    // Pad to 8-byte boundary
    int pad = (8 - (numBytes % 8)) % 8;
    for (int i = 0; i < pad; ++i)
        writeLittleEndian<std::uint8_t>(ds, 0);
}

static void writeMatTag(std::ostream& ds, std::uint32_t dataType, std::uint32_t numBytes)
{
    writeLittleEndian(ds, dataType);
    writeLittleEndian(ds, numBytes);
}

static void writeSmallTag(std::ostream& ds, std::uint32_t dataType, std::uint32_t numBytes, const std::string& data)
{
    // Small Data Element format (data <= 4 bytes)
    if (numBytes <= 4) {
        std::uint32_t packed = (numBytes << 16) | dataType;
        writeLittleEndian(ds, packed);
        ds.write(data.data(), numBytes);
        int pad = 4 - static_cast<int>(numBytes);
        for (int i = 0; i < pad; ++i)
            writeLittleEndian<std::uint8_t>(ds, 0);
    } else {
        writeMatTag(ds, dataType, numBytes);
        ds.write(data.data(), numBytes);
        writePad(ds, static_cast<int>(numBytes));
    }
}

// Write a double matrix variable to MAT file
static void writeMatrixVariable(std::ostream& ds, const std::string& name, const MatrixXd& mat)
{
    const int nRows = static_cast<int>(mat.rows());
    const int nCols = static_cast<int>(mat.cols());
    const std::string& nameBytes = name;

    // Calculate sizes
    std::uint32_t flagsSize = 8; // 2 x uint32 flags
    std::uint32_t dimsSize = 8;  // 2 x int32 dims
    std::uint32_t nameSize = static_cast<std::uint32_t>(nameBytes.size());
    std::uint32_t namePadded = nameSize <= 4 ? 4 : nameSize + ((8 - (nameSize % 8)) % 8);
    std::uint32_t nameTagSize = nameSize <= 4 ? 8 : 8 + namePadded;
    std::uint32_t dataSize = static_cast<std::uint32_t>(nRows * nCols * 8); // doubles
    std::uint32_t dataPadded = dataSize + ((8 - (dataSize % 8)) % 8);

    // Array flags subelement
    std::uint32_t arrayFlagsSubSize = 8 + flagsSize;
    // Dims subelement
    std::uint32_t dimsSubSize = 8 + dimsSize;
    // Name subelement
    std::uint32_t nameSubSize = nameTagSize;
    // Data subelement
    std::uint32_t dataSubSize = 8 + dataPadded;

    std::uint32_t totalPayload = arrayFlagsSubSize + dimsSubSize + nameSubSize + dataSubSize;

    // miMATRIX tag
    writeMatTag(ds, miMATRIX, totalPayload);

    // Array Flags subelement
    writeMatTag(ds, miUINT32, flagsSize);
    writeLittleEndian(ds, static_cast<std::uint32_t>(mxDOUBLE_CLASS)); // flags (double class)
    writeLittleEndian(ds, static_cast<std::uint32_t>(0));              // reserved

    // Dimensions subelement
    writeMatTag(ds, miINT32, dimsSize);
    writeLittleEndian(ds, static_cast<std::int32_t>(nRows));
    writeLittleEndian(ds, static_cast<std::int32_t>(nCols));

    // Name subelement
    if (nameSize <= 4) {
        writeSmallTag(ds, miINT8, nameSize, nameBytes);
    } else {
        writeMatTag(ds, miINT8, nameSize);
        ds.write(nameBytes.data(), nameSize);
        writePad(ds, static_cast<int>(nameSize));
    }

    // Data subelement (column-major)
    writeMatTag(ds, miDOUBLE, dataSize);
    for (int c = 0; c < nCols; ++c) {
        for (int r = 0; r < nRows; ++r) {
            writeLittleEndian(ds, mat(r, c));
        }
    }
    writePad(ds, static_cast<int>(dataSize));
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Export raw FIFF data to MATLAB Level 5 MAT file format.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"raw"}, "Input raw FIFF file.", "file", {}});
    parser.add_option({{"out"}, "Output .mat file.", "file", {}});
    parser.add_option({{"from"}, "First sample.", "samp", "-1"});
    parser.add_option({{"to"}, "Last sample.", "samp", "-1"});
    parser.process(argc, argv, "mne_raw2mat", PROGRAM_VERSION);

    const std::string rawFile = parser.value("raw");
    const std::string outFile = parser.value("out");
    int fromSamp = mne::to_int(parser.value("from")).value_or(0);
    int toSamp = mne::to_int(parser.value("to")).value_or(0);

    if (rawFile.empty()) {
        Log::critical("--raw is required.");
        std::print("{}", parser.help_text("mne_raw2mat"));
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        std::print("{}", parser.help_text("mne_raw2mat"));
        return 1;
    }

    // Load raw data
    FiffRawData raw(mne::path_from_utf8(rawFile));
    if (raw.info.empty()) {
        Log::critical("Cannot read raw: {}", rawFile);
        return 1;
    }

    int nChan = raw.info.nchan;
    double sfreq = raw.info.sfreq;
    int first = (fromSamp >= 0) ? fromSamp : raw.first_samp;
    int last = (toSamp >= 0) ? toSamp : raw.last_samp;

    Log::info("Raw: {} channels, {:.1f} Hz, samples {}..{}", nChan, sfreq, first, last);

    // Read data
    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times, first, last)) {
        Log::critical("Cannot read data segment.");
        return 1;
    }

    const auto rowCount = static_cast<long long>(data.rows());
    const auto sampleCount = static_cast<long long>(data.cols());
    Log::info("Read {} channels x {} samples", rowCount, sampleCount);

    // Build channel names
    StringList chNames;
    for (int c = 0; c < nChan; ++c)
        chNames.push_back(raw.info.chs[c].ch_name);

    // Write MAT file (Level 5 format)
    std::ofstream ds(mne::path_from_utf8(outFile), std::ios::binary);
    if (!ds) {
        Log::critical("Cannot open output: {}", outFile);
        return 1;
    }

    // --- 128-byte header ---
    // Text field (116 bytes)
    std::string headerText = "MATLAB 5.0 MAT-file, created by mne_raw2mat";
    headerText.resize(116, ' ');
    ds.write(headerText.data(), 116);

    // Subsystem data offset (8 bytes, zeros)
    for (int i = 0; i < 8; ++i)
        writeLittleEndian<std::uint8_t>(ds, 0);

    // Version (2 bytes) + Endian indicator (2 bytes)
    writeLittleEndian(ds, static_cast<std::uint16_t>(0x0100)); // version
    writeLittleEndian(ds, static_cast<std::uint16_t>(0x4D49)); // 'MI' = little-endian

    // --- Write 'data' variable (channels x samples) ---
    writeMatrixVariable(ds, "data", data);

    // --- Write 'sfreq' variable (1x1) ---
    MatrixXd sfreqMat(1, 1);
    sfreqMat(0, 0) = sfreq;
    writeMatrixVariable(ds, "sfreq", sfreqMat);

    // --- Write 'times' variable (1 x samples) ---
    writeMatrixVariable(ds, "times", times);

    ds.close();
    Log::info("Written MAT file: {}", outFile);
    Log::info("Variables: data ({} x {}), sfreq (1x1), times (1 x {})", nChan, sampleCount, static_cast<long long>(times.cols()));

    return 0;
}
