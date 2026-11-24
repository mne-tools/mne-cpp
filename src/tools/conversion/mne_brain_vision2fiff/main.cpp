//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2007
 * @brief    Convert BrainVision EEG files (.vhdr/.vmrk/.eeg) to FIFF format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_ch_info.h>
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
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// BrainVision header structures
//=============================================================================================================

struct BVChannelInfo
{
    std::string name;
    std::string refName;
    double resolution; // microvolts per bit
    std::string unit;
};

struct BVMarker
{
    std::string type;
    std::string description;
    int position; // sample index
    int duration; // in samples
    int channel;  // 0 = all channels
};

struct BVHeader
{
    // Common Infos
    std::string dataFile;
    std::string markerFile;
    std::string dataFormat; // BINARY or ASCII
    int dataOrientation;    // 0 = MULTIPLEXED, 1 = VECTORIZED
    int numberOfChannels;
    double samplingInterval; // in microseconds

    // Binary Format
    std::string binaryFormat; // INT_16, UINT_16, IEEE_FLOAT_32

    // Channel info
    std::vector<BVChannelInfo> channels;

    // Markers
    std::vector<BVMarker> markers;
};

//=============================================================================================================

static bool parseBVHeader(const std::string& vhdrPath, BVHeader& hdr)
{
    std::ifstream file(mne::path_from_utf8(vhdrPath));
    if (!file) {
        Log::critical("Cannot open header file: {}", vhdrPath);
        return false;
    }

    std::string currentSection;
    int chIdx = 0;

    hdr.dataOrientation = 0; // default: multiplexed
    hdr.numberOfChannels = 0;
    hdr.samplingInterval = 1000.0; // 1 ms default
    hdr.binaryFormat = "INT_16";

    std::string rawLine;
    while (std::getline(file, rawLine)) {
        const std::string line(mne::trimmed(rawLine));

        // Skip empty lines and comments
        if (line.empty() || line.starts_with(';'))
            continue;

        // Section header
        if (line.starts_with('[') && line.ends_with(']')) {
            currentSection = line.substr(1, line.length() - 2);
            chIdx = 0;
            continue;
        }

        // Key=value
        const std::size_t eqPos = line.find('=');
        if (eqPos == std::string::npos)
            continue;

        const std::string key(mne::trimmed(std::string_view(line).substr(0, eqPos)));
        const std::string value(mne::trimmed(std::string_view(line).substr(eqPos + 1)));

        if (currentSection == "Common Infos") {
            if (key == "DataFile")
                hdr.dataFile = value;
            else if (key == "MarkerFile")
                hdr.markerFile = value;
            else if (key == "DataFormat")
                hdr.dataFormat = value;
            else if (key == "DataOrientation") {
                hdr.dataOrientation = mne::iequals(value, "VECTORIZED") ? 1 : 0;
            } else if (key == "NumberOfChannels")
                hdr.numberOfChannels = mne::to_int(value).value_or(0);
            else if (key == "SamplingInterval")
                hdr.samplingInterval = mne::to_double(value).value_or(0.0);
        } else if (currentSection == "Binary Infos") {
            if (key == "BinaryFormat")
                hdr.binaryFormat = value;
        } else if (currentSection == "Channel Infos") {
            // Format: ChN=name,refName,resolution,unit
            const StringList parts = mne::split(value, ',');
            BVChannelInfo ch;
            ch.name = (parts.size() > 0) ? std::string(mne::trimmed(parts[0])) : std::format("Ch{}", chIdx + 1);
            ch.refName = (parts.size() > 1) ? std::string(mne::trimmed(parts[1])) : std::string();
            ch.resolution = (parts.size() > 2) ? mne::to_double(parts[2]).value_or(0.0) : 1.0;
            ch.unit = (parts.size() > 3) ? std::string(mne::trimmed(parts[3])) : "µV";
            if (ch.resolution == 0.0)
                ch.resolution = 1.0;
            hdr.channels.push_back(ch);
            chIdx++;
        }
    }

    if (hdr.numberOfChannels == 0)
        hdr.numberOfChannels = static_cast<int>(hdr.channels.size());

    // Fill missing channel definitions
    while (std::ssize(hdr.channels) < hdr.numberOfChannels) {
        BVChannelInfo ch;
        ch.name = std::format("Ch{}", hdr.channels.size() + 1);
        ch.resolution = 1.0;
        ch.unit = "µV";
        hdr.channels.push_back(ch);
    }

    return true;
}

//=============================================================================================================
// Parse BrainVision .vmrk marker file
//=============================================================================================================

static bool parseBVMarkers(const std::string& vmrkPath, BVHeader& hdr)
{
    std::ifstream file(mne::path_from_utf8(vmrkPath));
    if (!file) {
        Log::info("Warning: Cannot open marker file: {}", vmrkPath);
        return false;
    }

    std::string currentSection;

    std::string rawLine;
    while (std::getline(file, rawLine)) {
        const std::string line(mne::trimmed(rawLine));
        if (line.empty() || line.starts_with(';'))
            continue;

        if (line.starts_with('[') && line.ends_with(']')) {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }

        if (currentSection == "Marker Infos") {
            const std::size_t eqPos = line.find('=');
            if (eqPos == std::string::npos)
                continue;

            const std::string_view value = mne::trimmed(std::string_view(line).substr(eqPos + 1));
            const StringList parts = mne::split(value, ',');
            if (parts.size() >= 4) {
                BVMarker mkr;
                mkr.type = std::string(mne::trimmed(parts[0]));
                mkr.description = std::string(mne::trimmed(parts[1]));
                mkr.position = mne::to_int(parts[2]).value_or(0);
                mkr.duration = mne::to_int(parts[3]).value_or(0);
                mkr.channel = (parts.size() > 4) ? mne::to_int(parts[4]).value_or(0) : 0;
                hdr.markers.push_back(mkr);
            }
        }
    }

    return true;
}

//=============================================================================================================
// Read BrainVision binary data
//=============================================================================================================

/** Reads a little-endian value at @p offset of @p bytes; 0 past the end, like QDataStream. */
template<typename T>
static T readBVLittleEndian(const std::vector<char>& bytes, std::size_t offset)
{
    T value{};
    if (offset + sizeof(T) <= bytes.size()) {
        std::memcpy(&value, bytes.data() + offset, sizeof(T));
        if constexpr (std::endian::native == std::endian::big)
            value = mne::byte_swap(value);
    }
    return value;
}

//=============================================================================================================

static bool readBVData(const std::string& dataPath, const BVHeader& hdr, MatrixXd& data)
{
    std::ifstream file(mne::path_from_utf8(dataPath), std::ios::binary);
    if (!file) {
        Log::critical("Cannot open data file: {}", dataPath);
        return false;
    }
    const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    int nChan = hdr.numberOfChannels;
    const std::int64_t fileSize = std::ssize(bytes);
    int bytesPerSample;

    if (hdr.binaryFormat == "IEEE_FLOAT_32")
        bytesPerSample = 4;
    else // INT_16, UINT_16
        bytesPerSample = 2;

    // Multiplexed: samples are interleaved [ch0_s0, ch1_s0, ..., chN_s0, ch0_s1, ...]
    // Vectorized: all samples for ch0, then ch1, etc.
    const std::int64_t nSamples = fileSize / (nChan * bytesPerSample);

    Log::info("Data: {} channels x {} samples ({})", nChan, nSamples, hdr.binaryFormat);

    data.resize(nChan, nSamples);
    std::size_t offset = 0;
    const auto readValue = [&]() {
        double val = 0.0;
        if (hdr.binaryFormat == "IEEE_FLOAT_32") {
            val = static_cast<double>(readBVLittleEndian<float>(bytes, offset));
        } else if (hdr.binaryFormat == "INT_16") {
            val = static_cast<double>(readBVLittleEndian<std::int16_t>(bytes, offset));
        } else { // UINT_16
            val = static_cast<double>(readBVLittleEndian<std::uint16_t>(bytes, offset));
        }
        offset += static_cast<std::size_t>(bytesPerSample);
        return val;
    };

    if (hdr.dataOrientation == 0) {
        // Multiplexed
        for (std::int64_t s = 0; s < nSamples; ++s) {
            for (int c = 0; c < nChan; ++c)
                data(c, s) = readValue() * hdr.channels[c].resolution;
        }
    } else {
        // Vectorized
        for (int c = 0; c < nChan; ++c) {
            for (std::int64_t s = 0; s < nSamples; ++s)
                data(c, s) = readValue() * hdr.channels[c].resolution;
        }
    }

    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Convert BrainVision EEG files (.vhdr/.vmrk/.eeg) to FIFF format.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"vhdr"}, "Input BrainVision header file (.vhdr).", "file", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_brain_vision2fiff", PROGRAM_VERSION);

    const std::string vhdrFile = parser.value("vhdr");
    const std::string outFile = parser.value("out");

    if (vhdrFile.empty()) {
        Log::critical("--vhdr is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Parse header
    BVHeader hdr;
    if (!parseBVHeader(vhdrFile, hdr))
        return 1;

    const std::filesystem::path baseDir = std::filesystem::absolute(mne::path_from_utf8(vhdrFile)).parent_path();

    Log::info("BrainVision header: {} channels, sfreq={:.1f} Hz, format={}", hdr.numberOfChannels, 1e6 / hdr.samplingInterval, hdr.binaryFormat);

    // Parse markers if available
    if (!hdr.markerFile.empty()) {
        const std::string vmrkPath = mne::utf8_from_path(baseDir / mne::path_from_utf8(hdr.markerFile));
        parseBVMarkers(vmrkPath, hdr);
        Log::info("Markers: {} events", static_cast<long long>(hdr.markers.size()));
    }

    // Read binary data
    const std::string dataPath = mne::utf8_from_path(baseDir / mne::path_from_utf8(hdr.dataFile));
    MatrixXd data;
    if (mne::iequals(hdr.dataFormat, "BINARY")) {
        if (!readBVData(dataPath, hdr, data))
            return 1;
    } else {
        Log::critical("ASCII data format not supported.");
        return 1;
    }

    // Convert resolution from µV to V
    // BrainVision resolution is typically in µV, FIFF expects V
    data *= 1e-6;

    // Build FiffInfo
    double sfreq = 1e6 / hdr.samplingInterval; // samplingInterval is in µs
    int nChan = hdr.numberOfChannels;

    FiffInfo info;
    info.sfreq = sfreq;
    info.nchan = nChan;
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    info.meas_date[0] = static_cast<fiff_int_t>(now.time_since_epoch().count());
    info.meas_date[1] = 0;

    for (int c = 0; c < nChan; ++c) {
        FiffChInfo ch;
        ch.ch_name = hdr.channels[c].name;
        ch.scanNo = c + 1;
        ch.logNo = c + 1;

        // Classify channel type from name
        const std::string nameLow = mne::to_lower(ch.ch_name);
        if (nameLow.contains("eog")) {
            ch.kind = FIFFV_EOG_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else if (nameLow.contains("ecg")) {
            ch.kind = FIFFV_ECG_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else if (nameLow.contains("emg")) {
            ch.kind = FIFFV_EMG_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else if (nameLow.starts_with("sti") || nameLow.starts_with("trig")) {
            ch.kind = FIFFV_STIM_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else {
            ch.kind = FIFFV_EEG_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_EEG;
        }

        ch.cal = 1.0;
        ch.range = 1.0;
        info.chs.push_back(ch);
        info.ch_names.push_back(ch.ch_name);
    }

    // Write FIFF
    Eigen::RowVectorXd cals;
    auto stream = FiffStream::start_writing_raw(mne::path_from_utf8(outFile), info, cals);
    if (!stream) {
        Log::critical("Cannot open output: {}", outFile);
        return 1;
    }

    // Write data in chunks
    const int nSamples = static_cast<int>(data.cols());
    int chunkSize = 10000;
    for (int start = 0; start < nSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, nSamples);
        MatrixXd chunk = data.block(0, start, nChan, end - start);
        stream->write_raw_buffer(chunk);
    }

    stream->finish_writing_raw();
    Log::info("Written FIFF: {} ({} channels, {} samples, {:.1f} Hz)", outFile, nChan, nSamples, sfreq);

    return 0;
}
