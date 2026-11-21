//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainflow_stream.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BrainFlow's rows as an acquisition stream, and BrainFlow's multicast streamer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflow_stream.h"

#include "../../detail/acq_shared_library.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <charconv>
#include <cmath>
#include <cstring>
#include <format>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr std::chrono::seconds kClockRefresh(1);

constexpr std::pair<int, std::string_view> kBoards[] = {
#include "brainflow_boards.inc"
};

/** A role of BrainFlow's rows; the first role of a row wins, as the ExG roles share rows. */
struct Role
{
    const char* key;
    const char* prefix; // channel name prefix; a single row is named prefix alone
    AcqChannelKind kind;
    AcqUnit unit;
    double scale;
};

constexpr Role kRoles[] = {
    {"eeg_channels", "EEG", AcqChannelKind::Eeg, AcqUnit::Volt, 1e-6},
    {"emg_channels", "EMG", AcqChannelKind::Emg, AcqUnit::Volt, 1e-6},
    {"ecg_channels", "ECG", AcqChannelKind::Ecg, AcqUnit::Volt, 1e-6},
    {"eog_channels", "EOG", AcqChannelKind::Eog, AcqUnit::Volt, 1e-6},
    {"accel_channels", "ACC", AcqChannelKind::Accelerometer, AcqUnit::Unknown, 1.0},
    {"gyro_channels", "GYRO", AcqChannelKind::Gyroscope, AcqUnit::Unknown, 1.0},
    {"eda_channels", "EDA", AcqChannelKind::Gsr, AcqUnit::Unknown, 1.0},
    {"temperature_channels", "TEMP", AcqChannelKind::Temperature, AcqUnit::DegreeCelsius, 1.0},
    {"ppg_channels", "PPG", AcqChannelKind::Misc, AcqUnit::Unknown, 1.0},
    {"optical_channels", "OPT", AcqChannelKind::Misc, AcqUnit::Unknown, 1.0},
    {"resistance_channels", "RES", AcqChannelKind::Misc, AcqUnit::Unknown, 1.0},
    {"magnetometer_channels", "MAG", AcqChannelKind::Misc, AcqUnit::Unknown, 1.0},
    {"rotation_channels", "ROT", AcqChannelKind::Misc, AcqUnit::Unknown, 1.0},
    {"analog_channels", "ANALOG", AcqChannelKind::Misc, AcqUnit::Unknown, 1.0},
    {"other_channels", "OTHER", AcqChannelKind::Misc, AcqUnit::Unknown, 1.0},
    {"package_num_channel", "PACKAGE", AcqChannelKind::Misc, AcqUnit::None, 1.0},
    {"battery_channel", "BATTERY", AcqChannelKind::Misc, AcqUnit::None, 1.0},
    {"timestamp_channel", "TIMESTAMP", AcqChannelKind::Timestamp, AcqUnit::Second, 1.0},
    {"marker_channel", "MARKER", AcqChannelKind::Marker, AcqUnit::None, 1.0},
};

std::optional<int> parse_id(std::string_view board)
{
    int id = 0;
    const auto [end, error] = std::from_chars(board.data(), board.data() + board.size(), id);
    return error == std::errc() && end == board.data() + board.size() ? std::optional<int>(id) : std::nullopt;
}

int row_of(const BrainFlowDescription& description, std::string_view role)
{
    const std::span<const int> rows = description.role(role);
    return rows.empty() ? -1 : rows.front();
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::optional<std::pair<int, BrainFlowDescription>> mne::detail::builtin_brainflow_board(std::string_view board)
{
    const std::optional<int> id = parse_id(board);
    const std::string key = brainflow_board_key(board);
    for (const auto& [known, json] : kBoards) {
        auto description = parse_brainflow_description(json);
        if (description && (id ? *id == known : brainflow_board_key(description->name) == key)) {
            return std::pair{known, std::move(*description)};
        }
    }
    return std::nullopt;
}

//=============================================================================================================

AcqExpected<std::pair<int, BrainFlowDescription>> mne::detail::brainflow_board(std::string_view board)
{
    if (auto builtin = builtin_brainflow_board(board)) {
        return std::move(*builtin);
    }
    auto library = BrainFlowLibrary::load(brainflow_library_candidates());
    if (!library) {
        return AcqError(AcqErrorCode::InvalidParameter,
                        std::format("the layout of BrainFlow board '{}' needs BrainFlow ({}); without it the known boards are "
                                    "synthetic, cyton, ganglion and cytondaisy",
                                    board, library.error().text()));
    }
    const std::optional<int> parsed = parse_id(board);
    auto id = parsed ? AcqExpected<int>(*parsed) : (*library)->find_board(board);
    if (!id) {
        return id.error();
    }
    auto description = (*library)->description(*id);
    if (!description) {
        return description.error();
    }
    return std::pair{*id, std::move(*description)};
}

//=============================================================================================================

std::vector<std::string> mne::detail::brainflow_library_candidates()
{
    std::vector<std::string> candidates = {acqEnvironment("MNE_CPP_BRAINFLOW_LIB")};
    if (const std::string pip = acqPythonPackageDir("brainflow", "lib"); !pip.empty()) {
        candidates.push_back(pip + "/" + acqSharedLibraryName("BoardController"));
    }
    candidates.push_back(acqSharedLibraryName("BoardController"));
    return candidates;
}

//=============================================================================================================

AcqStreamInfo mne::detail::brainflow_stream_info(const BrainFlowDescription& description, std::string sourceId)
{
    AcqStreamInfo info;
    info.name = "eeg";
    info.sourceId = std::move(sourceId);
    info.format = AcqSampleFormat::Float64;
    info.layout = AcqLayout::Interleaved;
    info.sfreq = description.sampling_rate;
    info.channels.resize(description.rows);
    std::vector<bool> named(description.rows, false);
    for (const Role& role : kRoles) {
        const std::span<const int> rows = description.role(role.key);
        const bool eeg = std::string_view(role.key) == "eeg_channels" && description.eeg_names.size() == rows.size();
        for (std::size_t r = 0; r < rows.size(); ++r) {
            const auto row = static_cast<std::size_t>(rows[r]);
            if (named[row]) {
                continue;
            }
            named[row] = true;
            AcqChannelInfo& channel = info.channels[row];
            channel.name = eeg ? description.eeg_names[r] : std::string(role.key).ends_with("_channel") ? role.prefix
                                                                                                        : std::format("{} {:03}", role.prefix, r + 1);
            channel.kind = role.kind;
            channel.unit = role.unit;
            channel.scale = role.scale;
        }
    }
    for (std::size_t row = 0; row < description.rows; ++row) {
        if (!named[row]) {
            info.channels[row].name = std::format("ROW {:03}", row);
            info.channels[row].kind = AcqChannelKind::Misc;
        }
    }
    return info;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainFlowRowWriter::BrainFlowRowWriter(AcqWriter& writer, const BrainFlowDescription& description)
: m_writer(&writer)
, m_rows(description.rows)
, m_sfreq(description.sampling_rate)
{
    if (const int row = row_of(description, "timestamp_channel"); row >= 0) {
        m_timestamp_row = static_cast<std::size_t>(row);
    }
    if (const int row = row_of(description, "marker_channel"); row >= 0) {
        m_marker_row = static_cast<std::size_t>(row);
    }
}

//=============================================================================================================

AcqError BrainFlowRowWriter::commit(const AcqWriteView& view, const double* data, std::size_t samples, std::size_t sampleStride, std::size_t rowStride)
{
    std::size_t done = 0;
    for (std::size_t p = 0; p < view.partCount(); ++p) {
        const AcqWritePart& part = view.part(p);
        for (std::size_t s = 0; s < part.samples; ++s, ++done) {
            const double* sample = data + done * sampleStride;
            if (rowStride == 1) {
                std::memcpy(part.at(0, s), sample, m_rows * sizeof(double));
                continue;
            }
            for (std::size_t r = 0; r < m_rows; ++r) {
                std::memcpy(part.at(r, s), sample + r * rowStride, sizeof(double));
            }
        }
    }
    const std::uint64_t first = m_writer->writePosition();
    if (m_marker_row) {
        for (std::size_t s = 0; s < samples; ++s) {
            const double marker = data[s * sampleStride + *m_marker_row * rowStride];
            if (marker != 0.0) {
                if (AcqError e = m_writer->pushEvent({first + s, 0, static_cast<std::int32_t>(std::lround(marker))}); !e.ok()) {
                    return e;
                }
            }
        }
    }
    m_writer->commit(samples);
    if (m_timestamp_row) {
        publishClock(first + samples - 1, data[(samples - 1) * sampleStride + *m_timestamp_row * rowStride]);
    }
    return {};
}

//=============================================================================================================

void BrainFlowRowWriter::publishClock(std::uint64_t sample, double timestamp)
{
    const auto steady = std::chrono::steady_clock::now();
    if (m_clocked && steady - m_last_clock < kClockRefresh) {
        return;
    }
    m_clocked = true;
    m_last_clock = steady;
    const auto steadyNs = std::chrono::duration_cast<std::chrono::nanoseconds>(steady.time_since_epoch()).count();
    const auto utcNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    AcqClockModel clock;
    clock.sfreq = m_sfreq;
    clock.anchorSample = sample;
    clock.utcOffsetNs = utcNs - steadyNs;
    clock.anchorHostNs = std::llround(timestamp * 1e9) - clock.utcOffsetNs;
    clock.source = AcqClockSource::Device;
    m_writer->setClock(clock);
}

//=============================================================================================================

AcqExpected<BrainFlowStreamer> BrainFlowStreamer::open(const BrainFlowDescription& description, std::string address, std::uint16_t port,
                                                       std::size_t batch)
{
    auto socket = mne::UdpSocket::bind("0.0.0.0", 0);
    if (!socket) {
        return AcqError(AcqErrorCode::DeviceIo, "cannot open a UDP socket: " + socket.error().message());
    }
    BrainFlowStreamer streamer;
    streamer.m_socket = std::move(*socket);
    streamer.m_address = std::move(address);
    streamer.m_port = port;
    streamer.m_rows = description.rows;
    streamer.m_batch = std::max<std::size_t>(batch, 1);
    const std::span<const int> eeg = description.role("eeg_channels");
    streamer.m_eeg_rows.assign(eeg.begin(), eeg.end());
    streamer.m_package_row = row_of(description, "package_num_channel");
    streamer.m_timestamp_row = row_of(description, "timestamp_channel");
    streamer.m_marker_row = row_of(description, "marker_channel");
    streamer.m_datagram.reserve(streamer.m_batch * streamer.m_rows);
    return streamer;
}

//=============================================================================================================

AcqError BrainFlowStreamer::push(std::span<const double> eeg, double marker, double timestamp)
{
    const std::size_t at = m_datagram.size();
    m_datagram.resize(at + m_rows, 0.0);
    double* row = m_datagram.data() + at;
    for (std::size_t c = 0; c < std::min(eeg.size(), m_eeg_rows.size()); ++c) {
        row[m_eeg_rows[c]] = eeg[c];
    }
    if (m_package_row >= 0) {
        row[m_package_row] = static_cast<double>(m_sample % 256);
    }
    if (m_timestamp_row >= 0) {
        row[m_timestamp_row] = timestamp;
    }
    if (m_marker_row >= 0) {
        row[m_marker_row] = marker;
    }
    ++m_sample;
    if (m_datagram.size() < m_batch * m_rows) {
        return {};
    }
    const std::error_code error = m_socket.send_to(std::as_bytes(std::span(m_datagram)), m_address, m_port);
    m_datagram.clear();
    if (error) {
        return {AcqErrorCode::DeviceIo, std::format("cannot send to {}:{}: {}", m_address, m_port, error.message())};
    }
    return {};
}
