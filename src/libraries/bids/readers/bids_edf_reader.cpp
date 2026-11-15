//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_edf_reader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::EDFReader — European Data Format (EDF / EDF+) parser behind @ref mne::AbstractFormatReader.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_edf_reader.h"
#include <base/byte_stream.h>
#include <base/log.h>
#include <base/string_utils.h>

#include <fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE LOCAL FUNCTIONS
//=============================================================================================================

namespace
{

/** Splits "a.b.c" (dots or dashes) into three fields; empty when the text has another shape. */
std::optional<std::array<std::string, 3>> splitTriple(std::string_view text, char separator)
{
    std::array<std::string, 3> fields;
    std::size_t start = 0;
    for (std::size_t k = 0; k < 3; ++k) {
        const std::size_t end = k < 2 ? text.find(separator, start) : text.size();
        if (end == std::string_view::npos) {
            return std::nullopt;
        }
        fields[k] = std::string(mne::trimmed(text.substr(start, end - start)));
        start = end + 1;
    }
    return fields;
}

/** A calendar date from day, month and year, or empty when it does not exist. */
std::optional<std::chrono::sys_days> civilDate(std::optional<int> day, std::optional<int> month, std::optional<int> year)
{
    if (!day || !month || !year) {
        return std::nullopt;
    }
    const std::chrono::year_month_day date{std::chrono::year{*year}, std::chrono::month(static_cast<unsigned>(*month)),
                                           std::chrono::day(static_cast<unsigned>(*day))};
    return date.ok() ? std::optional(std::chrono::sys_days{date}) : std::nullopt;
}

} // namespace

//=============================================================================================================

std::optional<std::chrono::sys_time<std::chrono::microseconds>> EDFReader::parse_start_date(std::string_view recordingId,
                                                                                            std::string_view startDate,
                                                                                            std::string_view startTime)
{
    using namespace std::chrono;
    // EDF+ "Startdate dd-MMM-yyyy ..." carries all four digits of the year and wins over the header date
    std::optional<sys_days> date;
    const std::vector<std::string> recording = mne::split(mne::trimmed(recordingId), ' ');
    if (recording.size() == 5) {
        if (const auto fields = splitTriple(recording[1], '-')) {
            static constexpr std::array<std::string_view, 12> months{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                                                     "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
            const auto month = std::ranges::find_if(months, [&](std::string_view name) { return mne::equalsIgnoreCase(name, (*fields)[1]); });
            if (month != months.end()) {
                date = civilDate(mne::toInt((*fields)[0]), static_cast<int>(month - months.begin()) + 1, mne::toInt((*fields)[2]));
            }
        }
    }
    // Otherwise the two-digit header year: 85-99 is 1985-1999, 00-84 is 2000-2084
    if (!date) {
        if (const auto fields = splitTriple(startDate, '.')) {
            std::optional<int> year = mne::toInt((*fields)[2]);
            if (year) {
                *year += *year < 85 ? 2000 : 1900;
            }
            date = civilDate(mne::toInt((*fields)[0]), mne::toInt((*fields)[1]), year);
        }
    }
    if (!date) {
        return std::nullopt;
    }
    sys_time<microseconds> time = *date;
    if (const auto fields = splitTriple(startTime, '.')) {
        const auto hour = mne::toInt((*fields)[0]);
        const auto minute = mne::toInt((*fields)[1]);
        const auto second = mne::toInt((*fields)[2]);
        if (hour && minute && second) {
            time += hours(*hour) + minutes(*minute) + seconds(*second);
        }
    }
    return time;
}

//=============================================================================================================
// EDFChannelInfo
//=============================================================================================================

namespace
{

// mne read_raw_edf stim_channel="auto"
bool isAutoStimLabel(std::string_view label)
{
    return mne::equalsIgnoreCase(label, "status") || mne::equalsIgnoreCase(label, "trigger");
}

// EDF header fields are space-padded Latin-1
std::string headerText(std::string_view field)
{
    return mne::latin1_to_utf8(mne::trimmed(field));
}

} // namespace

//=============================================================================================================

FiffChInfo EDFChannelInfo::to_fiff_ch_info() const
{
    FiffChInfo info;

    info.scanNo = channelNumber;
    info.logNo = channelNumber;

    const std::string lower = mne::toLower(label);
    const auto has = [&lower](std::string_view part) {
        return lower.find(part) != std::string::npos;
    };
    if (!isMeasurement) {
        info.kind = has("stim") ? FIFFV_STIM_CH : FIFFV_MISC_CH;
    } else if (isAutoStimLabel(label)) {
        info.kind = FIFFV_STIM_CH;
    } else if (has("ecog")) {
        info.kind = FIFFV_ECOG_CH;
    } else if (has("seeg")) {
        info.kind = FIFFV_SEEG_CH;
    } else if (has("eeg")) {
        info.kind = FIFFV_EEG_CH;
    } else if (has("meg")) {
        info.kind = FIFFV_MEG_CH;
    } else if (has("ecg")) {
        info.kind = FIFFV_ECG_CH;
    } else if (has("eog")) {
        info.kind = FIFFV_EOG_CH;
    } else if (has("emg")) {
        info.kind = FIFFV_EMG_CH;
    } else {
        info.kind = FIFFV_EEG_CH;
    }

    // Samples are converted to SI by to_si(), so voltages are stored in V
    info.unit = (info.kind != FIFFV_STIM_CH && physicalDimension.ends_with('V')) ? FIFF_UNIT_V : FIFF_UNIT_NONE;
    info.unit_mul = FIFF_UNITM_NONE;

    info.cal = 1.0f;
    info.range = 1.0f;
    info.ch_name = label;

    return info;
}

//=============================================================================================================

float EDFChannelInfo::to_si() const
{
    // mne read_raw_edf: µV (micro sign, Greek mu, Shift-JIS mu) and uV are 1e-6, mV 1e-3, anything else 1
    if (isAutoStimLabel(label)) {
        return 1.0f;
    }
    const std::string_view d = physicalDimension;
    if (d == "\xC2\xB5V" || d == "\xCE\xBCV" || d == "\xC2\x83\xC3\x8AV" || d == "uV") {
        return 1.0e-6f;
    }
    if (d == "mV") {
        return 1.0e-3f;
    }
    return 1.0f;
}

//=============================================================================================================
// EDFReader
//=============================================================================================================

EDFReader::EDFReader() = default;

EDFReader::~EDFReader() = default;

//=============================================================================================================

bool EDFReader::open(const std::filesystem::path& path)
{
    m_file = mne::FileByteStream::open(path, mne::FileByteStream::Mode::Read);
    if (!m_file) {
        mne::Log::warning("[EDFReader::open] Could not open file: {}", mne::utf8FromPath(path));
        return false;
    }

    // Like mne, the extension decides between EDF (16 bit) and BDF (24 bit)
    m_bytesPerSample = mne::equalsIgnoreCase(mne::utf8FromPath(path.extension()), ".bdf") ? 3 : 2;
    if (!parse_header()) {
        mne::Log::warning("[EDFReader::open] Not an EDF/BDF header: {}", mne::utf8FromPath(path));
        m_file.reset();
        return false;
    }
    return true;
}

//=============================================================================================================

bool EDFReader::parse_header()
{
    // Fixed part: version 8, patient 80, recording 80, start date 8, start time 8, header bytes 8, reserved 44,
    // record count 8, record duration 8, signal count 4
    constexpr std::size_t fixedBytes = 256;
    constexpr std::size_t bytesPerSignal = 256;
    std::string header(fixedBytes, '\0');
    if (!m_file->seek(0) || m_file->read(header.data(), fixedBytes) != fixedBytes) {
        return false;
    }
    std::size_t cursor = 0;
    const auto field = [&header, &cursor](std::size_t width) {
        const std::string_view value = std::string_view(header).substr(cursor, width);
        cursor += width;
        return value;
    };

    field(8);  // version
    field(80); // patient
    const std::string_view recordingId = field(80);
    const std::string_view startDate = field(8);
    const std::string_view startTime = field(8);
    m_measDate = parse_start_date(recordingId, startDate, startTime);
    m_numBytesInHeader = mne::toInt(field(8)).value_or(0);
    field(44); // reserved
    m_numDataRecords = mne::toInt(field(8)).value_or(0);
    const auto recordDuration = static_cast<float>(mne::toDouble(field(8)).value_or(0.0));
    const int numChannels = mne::toInt(field(4)).value_or(0);
    if (numChannels <= 0 || static_cast<std::uint64_t>(numChannels) * bytesPerSignal + fixedBytes > m_file->size()) {
        return false;
    }

    // Per-signal fields, each stored for all signals in turn: label 16, transducer 80, dimension 8, physical min 8,
    // physical max 8, digital min 8, digital max 8, prefiltering 80, samples per record 8, reserved 32
    const auto count = static_cast<std::size_t>(numChannels);
    header.resize(count * bytesPerSignal);
    if (m_file->read(header.data(), header.size()) != header.size()) {
        return false;
    }
    cursor = 0;
    m_allChannels.assign(count, EDFChannelInfo{});
    const auto column = [&](std::size_t width, auto assign) {
        for (EDFChannelInfo& ch : m_allChannels) {
            assign(ch, field(width));
        }
    };
    column(16, [](EDFChannelInfo& ch, std::string_view v) { ch.label = headerText(v); });
    column(80, [](EDFChannelInfo& ch, std::string_view v) { ch.transducerType = headerText(v); });
    column(8, [](EDFChannelInfo& ch, std::string_view v) { ch.physicalDimension = headerText(v); });
    column(8, [](EDFChannelInfo& ch, std::string_view v) { ch.physicalMin = static_cast<float>(mne::toDouble(v).value_or(0.0)); });
    column(8, [](EDFChannelInfo& ch, std::string_view v) { ch.physicalMax = static_cast<float>(mne::toDouble(v).value_or(0.0)); });
    column(8, [](EDFChannelInfo& ch, std::string_view v) { ch.digitalMin = mne::toInt(v).value_or(0); });
    column(8, [](EDFChannelInfo& ch, std::string_view v) { ch.digitalMax = mne::toInt(v).value_or(0); });
    column(80, [](EDFChannelInfo& ch, std::string_view v) { ch.prefiltering = headerText(v); });
    column(8, [](EDFChannelInfo& ch, std::string_view v) { ch.samplesPerRecord = mne::toInt(v).value_or(0); });

    if (m_numBytesInHeader != static_cast<long>(fixedBytes + count * bytesPerSignal)) {
        mne::Log::warning("[EDFReader::parse_header] Header byte count mismatch: read {} expected {}", fixedBytes + count * bytesPerSignal, m_numBytesInHeader);
    }

    m_numBytesPerDataRecord = 0;
    long maxSamplesPerRecord = 0;
    for (std::size_t i = 0; i < count; ++i) {
        EDFChannelInfo& ch = m_allChannels[i];
        ch.channelNumber = static_cast<int>(i);
        ch.sampleCount = ch.samplesPerRecord * m_numDataRecords;
        ch.frequency = recordDuration > 0.0f ? static_cast<float>(ch.samplesPerRecord) / recordDuration : 0.0f;
        m_numBytesPerDataRecord += ch.samplesPerRecord * m_bytesPerSample;
        maxSamplesPerRecord = std::max(maxSamplesPerRecord, ch.samplesPerRecord);
    }

    // Measurement channels are those with the highest sample rate
    m_measChannels.clear();
    for (EDFChannelInfo& ch : m_allChannels) {
        ch.isMeasurement = ch.samplesPerRecord == maxSamplesPerRecord;
        if (ch.isMeasurement) {
            m_measChannels.push_back(ch);
        }
    }
    return true;
}

//=============================================================================================================

FiffInfo EDFReader::info() const
{
    FiffInfo info;
    info.nchan = static_cast<int>(m_measChannels.size());
    info.chs.reserve(m_measChannels.size());
    info.ch_names.reserve(m_measChannels.size());
    for (const EDFChannelInfo& ch : m_measChannels) {
        info.chs.push_back(ch.to_fiff_ch_info());
        info.ch_names.push_back(info.chs.back().ch_name);
    }
    info.sfreq = sfreq();
    if (m_measDate) {
        info.set_meas_date(*m_measDate);
    }
    return info;
}

//=============================================================================================================

MatrixXf EDFReader::read_raw_segment(long first, long last) const
{
    if (!m_file) {
        mne::Log::warning("[EDFReader::read_raw_segment] File not open");
        return MatrixXf();
    }

    const long total = sample_count();
    if (first < 0 || first >= total || last < 0 || last > total) {
        mne::Log::warning("[EDFReader::read_raw_segment] Index out of bounds: {} - {}", first, last);
        return MatrixXf();
    }
    const long numSamples = last - first;
    const long perRecord = m_measChannels.front().samplesPerRecord;
    if (numSamples <= 0 || perRecord <= 0) {
        return MatrixXf();
    }

    // Read the data records that hold the segment in one go
    const long firstRecord = first / perRecord;
    const long offsetInRecord = first % perRecord;
    const long numRecords = (offsetInRecord + numSamples + perRecord - 1) / perRecord;
    const auto recordBytes = static_cast<std::size_t>(m_numBytesPerDataRecord);
    std::vector<unsigned char> records(static_cast<std::size_t>(numRecords) * recordBytes);
    m_file->seek(static_cast<std::uint64_t>(m_numBytesInHeader) + static_cast<std::uint64_t>(firstRecord) * recordBytes);
    if (m_file->read(records.data(), records.size()) != records.size()) {
        mne::Log::warning("[EDFReader::read_raw_segment] File ends before sample {}", last);
    }

    // Channels are interleaved per record; demultiplex and scale the measurement channels
    MatrixXf result(static_cast<Index>(m_measChannels.size()), numSamples);
    Index row = 0;
    long channelOffset = 0;
    for (const EDFChannelInfo& ch : m_allChannels) {
        if (ch.isMeasurement) {
            const double digRange = static_cast<double>(ch.digitalMax - ch.digitalMin);
            const double physRange = static_cast<double>(ch.physicalMax) - ch.physicalMin;
            const double si = ch.to_si();
            const bool stim = isAutoStimLabel(ch.label);
            for (long s = 0; s < numSamples; ++s) {
                const long sample = offsetInRecord + s;
                const unsigned char* bytes = records.data() + static_cast<std::size_t>(sample / perRecord) * recordBytes + static_cast<std::size_t>((channelOffset + sample % perRecord) * m_bytesPerSample);
                // Little-endian signed integers: 16 bit (EDF) or 24 bit (BDF)
                int raw = bytes[0] | (bytes[1] << 8);
                if (m_bytesPerSample == 3) {
                    raw |= bytes[2] << 16;
                    raw = (raw & 0x800000) ? raw - 0x1000000 : raw;
                } else {
                    raw = static_cast<std::int16_t>(raw);
                }
                const double physical = (raw - ch.digitalMin) / digRange * physRange + ch.physicalMin;
                // mne read_raw_bdf keeps the raw stim value; both readers keep its low 17 bits
                if (stim) {
                    result(row, s) = static_cast<float>(static_cast<int>(m_bytesPerSample == 3 ? raw : physical) & 0x1FFFF);
                } else {
                    result(row, s) = static_cast<float>(physical * si);
                }
            }
            ++row;
        }
        channelOffset += ch.samplesPerRecord;
    }
    return result;
}

//=============================================================================================================

long EDFReader::sample_count() const
{
    return m_measChannels.empty() ? 0 : m_measChannels.front().sampleCount;
}

//=============================================================================================================

float EDFReader::sfreq() const
{
    return m_measChannels.empty() ? 0.0f : m_measChannels.front().frequency;
}

//=============================================================================================================

int EDFReader::channel_count() const
{
    return static_cast<int>(m_measChannels.size());
}

//=============================================================================================================

std::string_view EDFReader::format_name() const
{
    return "EDF";
}

//=============================================================================================================

bool EDFReader::supports_extension(std::string_view extension) const
{
    return mne::equalsIgnoreCase(extension, ".edf") || mne::equalsIgnoreCase(extension, ".bdf");
}
