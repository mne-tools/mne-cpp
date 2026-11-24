//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     May 2008
 * @brief    Implements mne_eximia2fiff: convert eXimia EEG data to FIFF format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_dig_point.h>
#include <mne/fiff/fiff_dig_point_set.h>
#include <mne/fiff/fiff_info.h>
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

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{

// The fixed eXimia montage of mne.io.read_raw_eximia: 64 multiplexed int16 channels at 1450 Hz
constexpr int kChannels = 64;
constexpr double kSfreq = 1450.0;
constexpr double kTriggerCal = 0.0015259021896696422;
constexpr double kEogCal = 0.3814755474174106;
constexpr double kEegCal = 0.07629510948348212;

StringList channelNames()
{
    StringList names{"GateIn", "Trig1", "Trig2", "EOG"};
    for (const std::string& name : mne::split("Fp1 Fpz Fp2 AF1 AFz AF2 F7 F3 F1 Fz F2 F4 F8 FT9 FT7 FC5 FC3 FC1 FCz FC2 FC4 FC6 FT8 FT10 "
                                              "T7 C5 C3 C1 Cz C2 C4 C6 T8 TP9 TP7 CP5 CP3 CP1 CPz CP2 CP4 CP6 TP8 TP10 "
                                              "P9 P7 P3 P1 Pz P2 P4 P8 P10 PO3 POz PO4 O1 Oz O2 Iz",
                                              ' '))
        names.push_back(name);
    return names;
}

// The text as a float, or nothing if it is not a number or out of float range
std::optional<float> toFloat(std::string_view text)
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

FiffInfo eximiaInfo()
{
    FiffInfo info;
    info.sfreq = kSfreq;
    info.nchan = kChannels;
    const StringList names = channelNames();
    for (int c = 0; c < kChannels; ++c) {
        FiffChInfo ch;
        ch.ch_name = names[c];
        ch.scanNo = c + 1;
        ch.logNo = c + 1;
        ch.unit = FIFF_UNIT_V;
        ch.range = 1.0f;
        if (c < 3) {
            ch.kind = FIFFV_STIM_CH;
            ch.cal = static_cast<float>(kTriggerCal);
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else if (c == 3) {
            ch.kind = FIFFV_EOG_CH;
            ch.cal = static_cast<float>(kEogCal);
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else {
            ch.kind = FIFFV_EEG_CH;
            ch.cal = static_cast<float>(kEegCal);
            ch.chpos.coil_type = FIFFV_COIL_EEG;
        }
        info.chs.push_back(ch);
        info.ch_names.push_back(ch.ch_name);
    }
    return info;
}

// eXimia digitizer text, as MNE-C's mne_eximia2fiff reads it: "Scalp" lines naming Left, Nasion or Right are
// the fiducials and "Point ... Original" lines the EEG electrodes (number in the fifth column); x y z in mm
bool readDigitizer(const std::string& path, FiffDigPointSet& points)
{
    std::ifstream in(mne::path_from_utf8(path));
    if (!in) {
        Log::critical("Cannot open digitizer file {}", path);
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.ends_with('\r'))
            line.pop_back();
        const std::vector<std::string_view> f = mne::split_whitespace(line);
        FiffDigPoint p;
        if (line.contains("Scalp")) {
            p.kind = FIFFV_POINT_CARDINAL;
            p.ident = line.contains("Left") ? FIFFV_POINT_LPA
                : line.contains("Nasion")   ? FIFFV_POINT_NASION
                : line.contains("Right")    ? FIFFV_POINT_RPA
                                            : 0;
        } else if (line.contains("Point") && line.contains("Original") && f.size() >= 5) {
            p.kind = FIFFV_POINT_EEG;
            p.ident = mne::to_int(f[4]).value_or(0);
        }
        if (p.ident == 0 || f.size() < 3)
            continue;
        bool ok = true;
        for (int k = 0; k < 3; ++k) {
            const std::optional<float> value = toFloat(f[k]);
            ok = ok && value.has_value();
            p.r[k] = value.value_or(0.0f) * 1e-3f;
        }
        if (!ok) {
            Log::critical("Malformed digitizer line in {}: {}", path, line);
            return false;
        }
        p.coord_frame = FIFFV_COORD_UNKNOWN;
        points.push_back(p);
    }
    if (!points.transform_to_head()) {
        Log::critical("{} lacks the Left, Nasion and Right fiducials needed for head coordinates", path);
        return false;
    }
    return true;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Convert eXimia EEG (.nxe) data to FIFF format, as mne.io.read_raw_eximia reads it.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"nxe"}, "eXimia .nxe data file.", "file", {}});
    parser.add_option({{"dig"}, "eXimia digitizer text file (fiducials and EEG electrodes).", "file", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_eximia2fiff", PROGRAM_VERSION);

    const std::string nxeFile = parser.value("nxe");
    const std::string outFile = parser.value("out");
    if (nxeFile.empty()) {
        Log::critical("eXimia .nxe file (--nxe) is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("Output file (--out) is required.");
        return 1;
    }

    std::ifstream nxe(mne::path_from_utf8(nxeFile), std::ios::binary);
    if (!nxe) {
        Log::critical("Cannot open {}", nxeFile);
        return 1;
    }
    const std::vector<char> bytes((std::istreambuf_iterator<char>(nxe)), std::istreambuf_iterator<char>());
    const std::int64_t frame = kChannels * static_cast<std::int64_t>(sizeof(std::int16_t));
    const std::int64_t nSamples = std::ssize(bytes) / frame;
    if (nSamples == 0) {
        Log::critical("{} holds no complete sample of {} channels", nxeFile, kChannels);
        return 1;
    }
    if (std::ssize(bytes) % frame != 0)
        Log::warning("{} ends with a partial sample; the file is likely truncated", nxeFile);

    FiffInfo info = eximiaInfo();
    if (parser.is_set("dig") && !readDigitizer(parser.value("dig"), info.dig))
        return 1;

    Eigen::MatrixXd data(kChannels, nSamples);
    for (std::int64_t s = 0; s < nSamples; ++s) {
        for (int c = 0; c < kChannels; ++c) {
            std::int16_t value = 0;
            std::memcpy(&value, bytes.data() + (s * kChannels + c) * static_cast<std::int64_t>(sizeof(std::int16_t)), sizeof(value));
            if constexpr (std::endian::native == std::endian::big)
                value = mne::byte_swap(value);
            data(c, s) = value * double(info.chs[c].cal);
        }
    }

    Eigen::RowVectorXd cals;
    auto stream = FiffStream::start_writing_raw(mne::path_from_utf8(outFile), info, cals);
    if (!stream) {
        Log::critical("Cannot open output: {}", outFile);
        return 1;
    }
    constexpr std::int64_t kBuffer = 10000;
    for (std::int64_t start = 0; start < nSamples; start += kBuffer) {
        const std::int64_t n = std::min(kBuffer, nSamples - start);
        if (!stream->write_raw_buffer(data.middleCols(start, n), cals)) {
            Log::critical("Writing {} failed", outFile);
            return 1;
        }
    }
    stream->finish_writing_raw();

    Log::info("Converted {}: {} channels, {} samples at {:.0f} Hz -> {}", mne::utf8_from_path(mne::path_from_utf8(nxeFile).filename()), kChannels, static_cast<long long>(nSamples), kSfreq, outFile);
    return 0;
}
