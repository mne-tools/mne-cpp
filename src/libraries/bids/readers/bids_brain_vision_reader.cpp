//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_brain_vision_reader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref BIDSLIB::BrainVisionReader — parses the BrainVision @c .vhdr / @c .vmrk / @c .eeg triplet behind @ref BIDSLIB::AbstractFormatReader.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_brain_vision_reader.h"
#include <base/byte_stream.h>
#include <base/endian.h>
#include <base/log.h>
#include <base/string_utils.h>

#include <fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <map>
#include <regex>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE LOCAL FUNCTIONS
//=============================================================================================================

namespace
{

constexpr std::string_view kMicroVolt = "\xC2\xB5V";

/** A marker date "YYYYMMDDhhmmssffffff" in UTC; empty for the all-zero placeholder or an invalid date. */
std::optional<std::chrono::sys_time<std::chrono::microseconds>> parseMarkerDate(std::string_view digits)
{
    using namespace std::chrono;
    const auto field = [digits](std::size_t pos, std::size_t count) {
        return BASELIB::toInt(digits.substr(pos, count)).value_or(-1);
    };
    const year_month_day date{year{field(0, 4)}, month(static_cast<unsigned>(field(4, 2))), day(static_cast<unsigned>(field(6, 2)))};
    const int hour = field(8, 2);
    const int minute = field(10, 2);
    const int second = field(12, 2);
    if (!date.ok() || hour > 23 || minute > 59 || second > 59) {
        return std::nullopt;
    }
    return sys_days{date} + hours(hour) + minutes(minute) + seconds(second) + microseconds(field(14, 6));
}

/** @return @p text with the BrainVision comma escape "\1" decoded. */
std::string decodeCommas(std::string_view text)
{
    std::string decoded;
    decoded.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size() && text[i + 1] == '1') {
            decoded.push_back(',');
            ++i;
        } else {
            decoded.push_back(text[i]);
        }
    }
    return decoded;
}

/** @return The integer in @p text, or @p fallback if @p text is not one. */
long toLong(std::string_view text, long fallback)
{
    text = BASELIB::trimmed(text);
    long value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    return error == std::errc() && end == text.data() + text.size() ? value : fallback;
}

/**
 * Calls @p entry(section, key, value) for every "key=value" line of a BrainVision header or marker file, in file
 * order, skipping the version line and comments and stopping at the free-form [Comment] section.
 */
template<typename Entry>
void forEachEntry(std::string_view text, Entry entry)
{
    std::string_view section;
    bool versionLine = true;
    while (!text.empty()) {
        const std::size_t newline = text.find('\n');
        const std::string_view line = BASELIB::trimmed(text.substr(0, newline));
        text = newline == std::string_view::npos ? std::string_view() : text.substr(newline + 1);
        if (std::exchange(versionLine, false) || line.empty() || line.front() == ';') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            if (BASELIB::equalsIgnoreCase(section, "comment")) {
                return;
            }
            continue;
        }
        const std::size_t eq = line.find('=');
        if (eq != std::string_view::npos && eq > 0 && !section.empty()) {
            entry(section, BASELIB::trimmed(line.substr(0, eq)), BASELIB::trimmed(line.substr(eq + 1)));
        }
    }
}

/** INI sections of a BrainVision header: section name -> key -> value. */
using Sections = std::map<std::string, std::map<std::string, std::string, std::less<>>, std::less<>>;

/** @return The value of @p key in @p section, or @p fallback. */
std::string_view value(const std::map<std::string, std::string, std::less<>>& section, std::string_view key, std::string_view fallback = {})
{
    const auto it = section.find(key);
    return it == section.end() ? fallback : std::string_view(it->second);
}

/** @return The little-endian value at @p bytes in host byte order. */
template<BASELIB::Swappable T>
T fromLittleEndian(const unsigned char* bytes)
{
    T value;
    std::memcpy(&value, bytes, sizeof(T));
    if constexpr (std::endian::native == std::endian::big) {
        value = BASELIB::byteSwap(value);
    }
    return value;
}

int bytesPerValue(BVBinaryFormat format)
{
    return format == BVBinaryFormat::INT_16 ? 2 : 4;
}

} // namespace

//=============================================================================================================
// BrainVisionChannelInfo
//=============================================================================================================

FiffChInfo BrainVisionChannelInfo::to_fiff_ch_info() const
{
    FiffChInfo info;
    info.scanNo = channelNumber;
    info.logNo = channelNumber;

    // Detect channel type from name
    const std::string lower = BASELIB::toLower(name);
    const auto has = [&lower](std::string_view part) {
        return lower.find(part) != std::string::npos;
    };
    if (has("ecog")) {
        info.kind = FIFFV_ECOG_CH;
    } else if (has("seeg")) {
        info.kind = FIFFV_SEEG_CH;
    } else if (has("eog") || lower == "heogl" || lower == "heogr" || lower == "veogb") {
        info.kind = FIFFV_EOG_CH;
    } else if (has("ecg") || has("ekg")) {
        info.kind = FIFFV_ECG_CH;
    } else if (has("emg")) {
        info.kind = FIFFV_EMG_CH;
    } else if (lower == "sti 014" || has("stim")) {
        info.kind = FIFFV_STIM_CH;
    } else {
        // Like mne read_raw_brainvision: voltage channels are EEG, anything else (ARU, µS, °C, ...) is misc
        info.kind = BrainVisionReader::is_voltage_unit(unit) ? FIFFV_EEG_CH : FIFFV_MISC_CH;
    }

    info.unit = FIFF_UNIT_NONE;
    info.unit_mul = FIFF_UNITM_NONE;
    if (BrainVisionReader::is_voltage_unit(unit)) {
        info.unit = FIFF_UNIT_V;
        if (unit == "nV") {
            info.unit_mul = FIFF_UNITM_N;
        } else if (unit == kMicroVolt || unit == "uV") {
            info.unit_mul = FIFF_UNITM_MU;
        } else if (unit == "mV") {
            info.unit_mul = FIFF_UNITM_M;
        }
    }

    // Calibration: resolution is the scaling factor from raw → physical units
    info.cal = resolution;
    info.range = 1.0f;
    info.ch_name = name;

    return info;
}

//=============================================================================================================
// BrainVisionReader
//=============================================================================================================

BrainVisionReader::BrainVisionReader() = default;

BrainVisionReader::~BrainVisionReader() = default;

//=============================================================================================================

bool BrainVisionReader::open(const std::filesystem::path& path)
{
    m_dataFile.reset();
    m_markers.clear();
    m_measDate.reset();
    if (!parse_header(path)) {
        return false;
    }

    m_dataFile = BASELIB::FileByteStream::open(m_dataPath, BASELIB::FileByteStream::Mode::Read);
    if (!m_dataFile) {
        BASELIB::Log::warning("[BrainVisionReader::open] Could not open data file: {}", BASELIB::utf8FromPath(m_dataPath));
        return false;
    }
    m_sampleCount = static_cast<long>(m_dataFile->size() / (static_cast<std::uint64_t>(bytesPerValue(m_binaryFormat)) * static_cast<std::uint64_t>(m_numChannels)));

    if (!m_markerPath.empty()) {
        parse_markers(m_markerPath);
    }
    return true;
}

//=============================================================================================================

bool BrainVisionReader::parse_header(const std::filesystem::path& vhdrPath)
{
    const BASELIB::Expected<std::string> text = BASELIB::read_text_file(vhdrPath);
    if (!text) {
        BASELIB::Log::warning("[BrainVisionReader::parse_header] Could not open header: {}", BASELIB::utf8FromPath(vhdrPath));
        return false;
    }

    const std::string_view firstLine = BASELIB::trimmed(std::string_view(*text).substr(0, text->find('\n')));
    static const std::regex versionPattern("Brain ?Vision( Core| V-Amp)? Data( Exchange)? Header File,? Version [12]\\.0", std::regex::icase);
    if (!std::regex_search(firstLine.begin(), firstLine.end(), versionPattern)) {
        BASELIB::Log::warning("[BrainVisionReader::parse_header] Unrecognized header version: {}", firstLine);
        return false;
    }

    Sections sections;
    forEachEntry(*text, [&sections](std::string_view section, std::string_view key, std::string_view entry) {
        sections[std::string(section)].insert_or_assign(std::string(key), std::string(entry));
    });

    // "Common infos" is the NeurOne spelling
    auto common = sections.find("Common Infos");
    if (common == sections.end()) {
        common = sections.find("Common infos");
    }
    if (common == sections.end() || common->second.empty()) {
        BASELIB::Log::warning("[BrainVisionReader::parse_header] Missing [Common Infos] section");
        return false;
    }
    const auto& commonInfos = common->second;

    // Data and marker files are relative to the .vhdr
    const std::string_view dataFile = value(commonInfos, "DataFile");
    if (dataFile.empty()) {
        BASELIB::Log::warning("[BrainVisionReader::parse_header] No DataFile specified");
        return false;
    }
    const std::filesystem::path dir = std::filesystem::absolute(vhdrPath).parent_path();
    m_dataPath = dir / BASELIB::pathFromUtf8(dataFile);
    const std::string_view markerFile = value(commonInfos, "MarkerFile");
    m_markerPath = markerFile.empty() ? std::filesystem::path() : dir / BASELIB::pathFromUtf8(markerFile);

    const std::string_view dataFormat = value(commonInfos, "DataFormat", "BINARY");
    if (!BASELIB::equalsIgnoreCase(dataFormat, "BINARY")) {
        BASELIB::Log::warning("[BrainVisionReader::parse_header] Only BINARY data is supported, not {}", dataFormat);
        return false;
    }

    m_orientation = BASELIB::equalsIgnoreCase(value(commonInfos, "DataOrientation", "MULTIPLEXED"), "VECTORIZED") ? BVOrientation::VECTORIZED : BVOrientation::MULTIPLEXED;

    m_numChannels = BASELIB::toInt(value(commonInfos, "NumberOfChannels", "0")).value_or(0);
    if (m_numChannels <= 0) {
        BASELIB::Log::warning("[BrainVisionReader::parse_header] Invalid channel count: {}", m_numChannels);
        return false;
    }

    // Sampling interval in microseconds
    const double samplingInterval = BASELIB::toDouble(value(commonInfos, "SamplingInterval", "0")).value_or(0.0);
    m_sfreq = samplingInterval > 0.0 ? static_cast<float>(1.0e6 / samplingInterval) : 0.0f;

    static const std::map<std::string, std::string, std::less<>> none;
    const auto section = [&sections](std::string_view name) -> const std::map<std::string, std::string, std::less<>>& {
        const auto it = sections.find(name);
        return it == sections.end() ? none : it->second;
    };

    const std::string_view binaryFormat = value(section("Binary Infos"), "BinaryFormat", "INT_16");
    if (BASELIB::equalsIgnoreCase(binaryFormat, "INT_32")) {
        m_binaryFormat = BVBinaryFormat::INT_32;
    } else if (BASELIB::equalsIgnoreCase(binaryFormat, "IEEE_FLOAT_32")) {
        m_binaryFormat = BVBinaryFormat::IEEE_FLOAT_32;
    } else {
        m_binaryFormat = BVBinaryFormat::INT_16;
    }

    // Ch<N>=<Name>,<Reference>,<Resolution>,<Unit>
    const auto& channelInfos = section("Channel Infos");
    m_channels.clear();
    m_channels.reserve(static_cast<std::size_t>(m_numChannels));
    for (int i = 1; i <= m_numChannels; ++i) {
        BrainVisionChannelInfo ch;
        ch.channelNumber = i - 1;
        ch.unit = kMicroVolt;
        const std::string_view entry = value(channelInfos, "Ch" + std::to_string(i));
        if (entry.empty()) {
            ch.name = "Ch" + std::to_string(i);
        } else {
            const std::vector<std::string> parts = BASELIB::split(entry, ',');
            ch.name = decodeCommas(parts[0]);
            if (parts.size() >= 2) {
                ch.reference = decodeCommas(parts[1]);
            }
            if (parts.size() >= 3 && !parts[2].empty()) {
                ch.resolution = static_cast<float>(BASELIB::toDouble(parts[2]).value_or(0.0));
            }
            if (parts.size() >= 4 && !parts[3].empty()) {
                ch.unit = parts[3];
            }
        }
        m_channels.push_back(std::move(ch));
    }
    return true;
}

//=============================================================================================================

bool BrainVisionReader::parse_markers(const std::filesystem::path& vmrkPath)
{
    const BASELIB::Expected<std::string> text = BASELIB::read_text_file(vmrkPath);
    if (!text) {
        BASELIB::Log::warning("[BrainVisionReader::parse_markers] Could not open marker file: {}", BASELIB::utf8FromPath(vmrkPath));
        return false;
    }

    // Mk<N>=<Type>,<Description>,<Position>,<Duration>,<Channel>[,<Date>]
    bool measDateFound = false;
    forEachEntry(*text, [this, &measDateFound](std::string_view section, std::string_view, std::string_view entry) {
        if (section != "Marker Infos") {
            return;
        }
        const std::vector<std::string> parts = BASELIB::split(entry, ',');
        if (parts.size() < 5) {
            return;
        }
        BrainVisionMarker marker;
        marker.type = decodeCommas(parts[0]);
        marker.description = decodeCommas(parts[1]);
        marker.position = toLong(parts[2], 0) - 1; // 1-based in the file
        marker.duration = toLong(parts[3], 0);
        marker.channel = static_cast<int>(toLong(parts[4], 0));

        // Like mne, the recording start is the date of the first New Segment marker that has one
        const bool hasDate = parts.size() == 6 && parts[5].size() == 20 && std::ranges::all_of(parts[5], [](char c) { return c >= '0' && c <= '9'; });
        if (hasDate) {
            marker.date = parseMarkerDate(parts[5]);
            if (marker.type == "New Segment" && !std::exchange(measDateFound, true)) {
                m_measDate = marker.date;
            }
        }
        m_markers.push_back(std::move(marker));
    });
    return true;
}

//=============================================================================================================

float BrainVisionReader::unit_scale(std::string_view unit)
{
    // mne.io.brainvision _unit_dict; units it does not know are taken as-is (factor 1)
    if (unit == kMicroVolt || unit == "uV" || unit == "\xC2\xB5S" || unit == "uS") {
        return 1.0e-6f;
    }
    if (unit == "mV") {
        return 1.0e-3f;
    }
    if (unit == "nV") {
        return 1.0e-9f;
    }
    return 1.0f;
}

//=============================================================================================================

bool BrainVisionReader::is_voltage_unit(std::string_view unit)
{
    return unit == "V" || unit == "mV" || unit == kMicroVolt || unit == "uV" || unit == "nV";
}

//=============================================================================================================

FIFFLIB::FiffInfo BrainVisionReader::info() const
{
    FiffInfo info;
    info.nchan = static_cast<int>(m_channels.size());
    info.sfreq = m_sfreq;
    info.chs.reserve(m_channels.size());
    info.ch_names.reserve(m_channels.size());
    for (const BrainVisionChannelInfo& ch : m_channels) {
        info.chs.push_back(ch.to_fiff_ch_info());
        info.ch_names.push_back(info.chs.back().ch_name);
    }
    if (m_measDate) {
        info.set_meas_date(*m_measDate);
    }
    return info;
}

//=============================================================================================================

Eigen::MatrixXf BrainVisionReader::read_raw_segment(long first, long last) const
{
    if (!m_dataFile) {
        BASELIB::Log::warning("[BrainVisionReader::read_raw_segment] File not open");
        return MatrixXf();
    }
    if (first < 0 || first >= m_sampleCount || last < 0 || last > m_sampleCount || last <= first) {
        BASELIB::Log::warning("[BrainVisionReader::read_raw_segment] Invalid range: {} - {}", first, last);
        return MatrixXf();
    }

    const long numSamples = last - first;
    const auto valueBytes = static_cast<std::size_t>(bytesPerValue(m_binaryFormat));
    const auto channels = static_cast<std::size_t>(m_numChannels);
    std::vector<float> scales(channels);
    for (std::size_t ch = 0; ch < channels; ++ch) {
        scales[ch] = m_channels[ch].resolution * unit_scale(m_channels[ch].unit);
    }
    const auto decode = [this](const unsigned char* bytes) -> float {
        switch (m_binaryFormat) {
            case BVBinaryFormat::INT_16:
                return static_cast<float>(fromLittleEndian<std::int16_t>(bytes));
            case BVBinaryFormat::INT_32:
                return static_cast<float>(fromLittleEndian<std::int32_t>(bytes));
            case BVBinaryFormat::IEEE_FLOAT_32:
                return fromLittleEndian<float>(bytes);
        }
        return 0.0f;
    };

    MatrixXf result(m_numChannels, numSamples);
    std::vector<unsigned char> buffer;
    if (m_orientation == BVOrientation::MULTIPLEXED) {
        // [ch1_t1, ch2_t1, ..., chN_t1, ch1_t2, ...]
        buffer.resize(static_cast<std::size_t>(numSamples) * channels * valueBytes);
        m_dataFile->seek(static_cast<std::uint64_t>(first) * channels * valueBytes);
        m_dataFile->read(buffer.data(), buffer.size());
        const unsigned char* bytes = buffer.data();
        for (Index s = 0; s < numSamples; ++s) {
            for (std::size_t ch = 0; ch < channels; ++ch, bytes += valueBytes) {
                result(static_cast<Index>(ch), s) = decode(bytes) * scales[ch];
            }
        }
    } else {
        // [ch1_t1, ch1_t2, ..., ch1_tM, ch2_t1, ...]
        buffer.resize(static_cast<std::size_t>(numSamples) * valueBytes);
        for (std::size_t ch = 0; ch < channels; ++ch) {
            m_dataFile->seek((static_cast<std::uint64_t>(ch) * static_cast<std::uint64_t>(m_sampleCount) + static_cast<std::uint64_t>(first)) * valueBytes);
            m_dataFile->read(buffer.data(), buffer.size());
            const unsigned char* bytes = buffer.data();
            for (Index s = 0; s < numSamples; ++s, bytes += valueBytes) {
                result(static_cast<Index>(ch), s) = decode(bytes) * scales[ch];
            }
        }
    }
    return result;
}

//=============================================================================================================

long BrainVisionReader::sample_count() const
{
    return m_sampleCount;
}

//=============================================================================================================

float BrainVisionReader::sfreq() const
{
    return m_sfreq;
}

//=============================================================================================================

int BrainVisionReader::channel_count() const
{
    return m_numChannels;
}

//=============================================================================================================

std::string_view BrainVisionReader::format_name() const
{
    return "BrainVision";
}

//=============================================================================================================

bool BrainVisionReader::supports_extension(std::string_view extension) const
{
    return BASELIB::equalsIgnoreCase(extension, ".vhdr") || BASELIB::equalsIgnoreCase(extension, ".ahdr");
}
