//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2005
 * @brief    Implements mne_tufts2fiff: convert Tufts University EEG data to FIFF format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_file.h>
#include <mne/base/byte_stream.h>
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

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// Tufts raw file header structure
//=============================================================================================================

struct TuftsHeader
{
    std::int32_t nChannels;
    std::int32_t nSamples;
    float sFreq;
    std::int32_t dataOffset;
};

//=============================================================================================================

/**
 * Reads little-endian values from a file; reads past the end give 0 and seeks past the end stay at the end.
 */
class TuftsLittleEndianReader
{
public:
    explicit TuftsLittleEndianReader(const std::string& path)
    : m_stream(mne::FileByteStream::open(mne::path_from_utf8(path), mne::FileByteStream::Mode::Read))
    {
    }

    bool isOpen() const
    {
        return m_stream != nullptr;
    }

    void seek(std::uint64_t position)
    {
        if (!m_stream->seek(position))
            m_stream->seek(m_stream->size());
    }

    template<typename T>
    T read()
    {
        T value{};
        if (m_stream->read(&value, sizeof(T)) != sizeof(T))
            return T{};
        if constexpr (std::endian::native == std::endian::big)
            value = mne::byte_swap(value);
        return value;
    }

private:
    std::unique_ptr<mne::FileByteStream> m_stream;
};

//=============================================================================================================

/** @return @p text as a float, or nothing if it is not a number or out of float range. */
static std::optional<float> tuftsToFloat(std::string_view text)
{
    const std::optional<double> value = mne::to_double(text);
    if (!value)
        return std::nullopt;
    if (std::isinf(*value) || std::isnan(*value))
        return static_cast<float>(*value);
    if (std::abs(*value) > static_cast<double>(std::numeric_limits<float>::max()))
        return std::nullopt;
    const float result = static_cast<float>(*value);
    if (*value != 0.0 && result == 0.0f)
        return std::nullopt;
    return result;
}

//=============================================================================================================

static bool readTuftsHeader(TuftsLittleEndianReader& in, TuftsHeader& header)
{
    // Tufts raw format header
    header.nChannels = in.read<std::int16_t>();

    [[maybe_unused]] const std::int16_t dummy = in.read<std::int16_t>(); // epoch size or flags

    header.nSamples = in.read<std::int32_t>();

    // Sampling frequency (typically stored as period in ms)
    const std::int16_t periodMs = in.read<std::int16_t>();
    if (periodMs > 0)
        header.sFreq = 1000.0f / periodMs;
    else
        header.sFreq = 250.0f; // default

    header.dataOffset = 512; // typical header size

    Log::info("Tufts header: {} channels, {} samples, {:.1f} Hz", header.nChannels, header.nSamples, header.sFreq);
    return true;
}

//=============================================================================================================

static bool readElpFile(const std::string& filename, MatrixX3f& positions, StringList& names)
{
    std::ifstream in(mne::path_from_utf8(filename));
    if (!in) {
        Log::critical("Cannot open elp file: {}", filename);
        return false;
    }

    std::vector<Vector3f> posVec;
    std::string rawLine;
    while (std::getline(in, rawLine)) {
        const std::string_view line = mne::trimmed(rawLine);
        if (line.empty() || line.starts_with('#') || line.starts_with('%'))
            continue;
        const std::vector<std::string_view> parts = mne::split_whitespace(line);
        if (parts.size() >= 4) {
            names.emplace_back(parts[0]);
            // Positions are typically in spherical or Cartesian coords (mm)
            float x = tuftsToFloat(parts[1]).value_or(0.0f) * 0.001f;
            float y = tuftsToFloat(parts[2]).value_or(0.0f) * 0.001f;
            float z = tuftsToFloat(parts[3]).value_or(0.0f) * 0.001f;
            posVec.emplace_back(x, y, z);
        }
    }
    positions.resize(std::ssize(posVec), 3);
    for (int i = 0; i < std::ssize(posVec); i++) {
        positions.row(i) = posVec[i].transpose();
    }
    Log::info("Read {} electrode positions from {}", static_cast<int>(posVec.size()), filename);
    return true;
}

//=============================================================================================================

static bool readCalibration(const std::string& filename, VectorXf& cals)
{
    std::ifstream in(mne::path_from_utf8(filename));
    if (!in) {
        Log::critical("Cannot open calibration file: {}", filename);
        return false;
    }
    std::vector<float> calVec;
    std::string rawLine;
    while (std::getline(in, rawLine)) {
        const std::string_view line = mne::trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;
        const std::optional<float> val = tuftsToFloat(line);
        if (val && *val != 0.0f)
            calVec.push_back(*val);
    }
    cals.resize(std::ssize(calVec));
    for (int i = 0; i < std::ssize(calVec); i++)
        cals(i) = calVec[i];
    Log::info("Read {} calibration values", static_cast<int>(calVec.size()));
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Convert Tufts University EEG data to FIFF format.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"raw"}, "Tufts raw data file.", "file", {}});
    parser.add_option({{"cal"}, "Calibration file.", "file", {}});
    parser.add_option({{"elp"}, "Electrode position file.", "file", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.add_option({{"sfreq"}, "Override sampling frequency.", "Hz", {}});
    parser.process(argc, argv, "mne_tufts2fiff", PROGRAM_VERSION);

    const std::string rawFile = parser.value("raw");
    const std::string calFile = parser.value("cal");
    const std::string elpFile = parser.value("elp");
    const std::string outFile = parser.value("out");
    float sfreqOverride = parser.is_set("sfreq") ? tuftsToFloat(parser.value("sfreq")).value_or(0.0f) : 0.0f;

    if (rawFile.empty()) {
        Log::critical("Raw data file (--raw) is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("Output file (--out) is required.");
        return 1;
    }

    //
    // Read Tufts raw data
    //
    TuftsLittleEndianReader raw(rawFile);
    if (!raw.isOpen()) {
        Log::critical("Cannot open raw file: {}", rawFile);
        return 1;
    }

    TuftsHeader header;
    if (!readTuftsHeader(raw, header)) {
        Log::critical("Failed to read Tufts header.");
        return 1;
    }

    if (sfreqOverride > 0.0f)
        header.sFreq = sfreqOverride;

    // Read calibration if provided
    VectorXf cals;
    if (!calFile.empty()) {
        readCalibration(calFile, cals);
    }

    // Read electrode positions if provided
    MatrixX3f elpPositions;
    StringList elpNames;
    if (!elpFile.empty()) {
        readElpFile(elpFile, elpPositions, elpNames);
    }

    //
    // Create channel info
    //
    int nch = header.nChannels;
    std::vector<FiffChInfo> chs;
    for (int k = 0; k < nch; k++) {
        FiffChInfo ch;
        ch.scanNo = k + 1;
        ch.logNo = k + 1;
        ch.kind = FIFFV_EEG_CH;
        ch.range = 1.0f;
        ch.cal = (cals.size() > k) ? cals(k) : 1.0f;
        ch.chpos.coil_type = FIFFV_COIL_EEG;
        ch.unit = FIFF_UNIT_V;
        ch.unit_mul = FIFF_UNITM_NONE;

        if (k < std::ssize(elpNames)) {
            ch.ch_name = elpNames[k];
        } else {
            ch.ch_name = std::format("EEG{:03}", k + 1);
        }

        if (k < elpPositions.rows()) {
            ch.chpos.r0[0] = elpPositions(k, 0);
            ch.chpos.r0[1] = elpPositions(k, 1);
            ch.chpos.r0[2] = elpPositions(k, 2);
        }

        chs.push_back(ch);
    }

    //
    // Read raw data
    //
    raw.seek(static_cast<std::uint64_t>(header.dataOffset));
    int nSamples = header.nSamples;
    MatrixXf data(nch, nSamples);

    for (int s = 0; s < nSamples; s++) {
        for (int c = 0; c < nch; c++) {
            data(c, s) = static_cast<float>(raw.read<std::int16_t>());
        }
    }

    // Apply calibrations
    for (int c = 0; c < nch; c++) {
        if (cals.size() > c && cals(c) != 0.0f) {
            data.row(c) *= cals(c);
        }
    }

    //
    // Write FIFF output
    //
    auto stream = FiffStream::start_file(mne::path_from_utf8(outFile));
    if (!stream) {
        Log::critical("Cannot create output FIFF file.");
        return 1;
    }

    // Measurement info
    stream->start_block(FIFFB_MEAS);
    stream->start_block(FIFFB_MEAS_INFO);

    fiff_int_t nchan = nch;
    stream->write_int(FIFF_NCHAN, &nchan);
    float sfreq = header.sFreq;
    stream->write_float(FIFF_SFREQ, &sfreq);

    for (int k = 0; k < nch; k++) {
        stream->write_ch_info(chs[k]);
    }

    // Digitization points (if electrode positions provided)
    if (elpPositions.rows() > 0) {
        stream->start_block(FIFFB_ISOTRAK);
        for (int k = 0; k < elpPositions.rows(); k++) {
            FiffDigPoint dig;
            dig.kind = (k < 3) ? FIFFV_POINT_CARDINAL : FIFFV_POINT_EEG;
            dig.ident = (k < 3) ? (k + 1) : (k - 2);
            dig.r[0] = elpPositions(k, 0);
            dig.r[1] = elpPositions(k, 1);
            dig.r[2] = elpPositions(k, 2);
            dig.coord_frame = FIFFV_COORD_HEAD;
            stream->write_dig_point(dig);
        }
        stream->end_block(FIFFB_ISOTRAK);
    }

    stream->end_block(FIFFB_MEAS_INFO);

    // Raw data
    stream->start_block(FIFFB_RAW_DATA);

    // Write in 10-second buffers
    int bufSize = static_cast<int>(10.0f * header.sFreq);
    for (int start = 0; start < nSamples; start += bufSize) {
        int end = std::min(start + bufSize, nSamples);
        MatrixXf buf = data.block(0, start, nch, end - start);

        fiff_int_t firstSamp = start;
        stream->write_int(FIFF_FIRST_SAMPLE, &firstSamp);
        stream->write_float_matrix(FIFF_DATA_BUFFER, buf);
    }

    stream->end_block(FIFFB_RAW_DATA);
    stream->end_block(FIFFB_MEAS);
    stream->end_file();

    Log::info("Wrote FIFF: {} channels, {} samples at {:.1f} Hz to {}", nch, nSamples, header.sFreq, outFile);

    return 0;
}
