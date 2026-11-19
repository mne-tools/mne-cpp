//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainflow_stub.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    A stand-in for BrainFlow's BoardController library with known samples, for testing brainflow://.
 *
 * Three boards: Synthetic (-1), Cyton (0), which opens only on serial_port COM-STUB, and Ganglion (1), which is
 * described but never opens, as a board this platform's BrainFlow is built without. All have 12 rows at
 * 250 Hz: package number 0, EEG 1-4 (also the EMG rows) named Fz,Cz,Pz,Oz, accelerometer 5-7, temperature 8,
 * battery 9, timestamp 10, marker 11. Samples arrive at 250 Hz from start_stream(); in sample k, EEG row r
 * holds r + k / 1000 uV, the timestamp is 1.7e9 + k / 250 s and the marker is 7 at k = 30. The version is
 * STUB_VERSION. Like BrainFlow, every input parameter must be in the JSON of a call.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>
#include <string_view>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(_WIN32)
#define STUB_EXPORT extern "C" __declspec(dllexport)
#else
#define STUB_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#ifndef STUB_VERSION
#define STUB_VERSION "5.12.1"
#endif

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr int kOk = 0;
constexpr int kUnableToOpenPort = 2;
constexpr int kBoardNotReady = 7;
constexpr int kInvalidArguments = 13;
constexpr int kUnsupportedBoard = 14;
constexpr int kGeneralError = 17;
constexpr double kRate = 250.0;

constexpr std::string_view kDescription = R"({"accel_channels":[5,6,7],"battery_channel":9,"eeg_channels":[1,2,3,4],)"
                                          R"("eeg_names":"Fz,Cz,Pz,Oz","emg_channels":[1,2,3,4],"marker_channel":11,"name":"{}",)"
                                          R"("num_rows":12,"package_num_channel":0,"sampling_rate":250,)"
                                          R"("temperature_channels":[8],"timestamp_channel":10})";

bool g_prepared = false;
bool g_streaming = false;
int g_sample = 0;
int g_buffered = 0;
std::chrono::steady_clock::rep g_started = 0; // ticks of start_stream()

/** @return Whether @p json has every key BrainFlow reads; it throws on a missing one. */
bool complete(const char* json)
{
    const std::string_view text(json == nullptr ? "" : json);
    for (const char* key : {"serial_port", "ip_protocol", "ip_port", "ip_port_aux", "ip_port_anc", "other_info", "mac_address", "ip_address",
                            "ip_address_aux", "ip_address_anc", "timeout", "serial_number", "file", "file_aux", "file_anc", "master_board"}) {
        if (text.find(std::string("\"") + key + "\":") == std::string_view::npos) {
            return false;
        }
    }
    return true;
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

STUB_EXPORT int get_version_board_controller(char* version, int* length, int max)
{
    const std::string_view text(STUB_VERSION);
    *length = static_cast<int>(std::min<std::size_t>(text.size(), static_cast<std::size_t>(max)));
    std::memcpy(version, text.data(), static_cast<std::size_t>(*length));
    return kOk;
}

STUB_EXPORT int get_board_descr(int board, int preset, char* description, int* length, int max)
{
    if (board < -1 || board > 1 || preset != 0) {
        return kUnsupportedBoard;
    }
    std::string text(kDescription);
    text.replace(text.find("{}"), 2, board == -1 ? "Synthetic" : board == 0 ? "Cyton"
                                                                            : "Ganglion");
    if (static_cast<int>(text.size()) > max) {
        return kInvalidArguments;
    }
    *length = static_cast<int>(text.size());
    std::ranges::copy(text, description);
    return kOk;
}

STUB_EXPORT int prepare_session(int board, const char* json)
{
    if (!complete(json)) {
        return kGeneralError;
    }
    if (board == 0 && std::string_view(json).find(R"("serial_port":"COM-STUB")") == std::string_view::npos) {
        return kUnableToOpenPort;
    }
    if (board != -1 && board != 0) {
        return kUnsupportedBoard;
    }
    g_prepared = true;
    return kOk;
}

STUB_EXPORT int start_stream(int buffer, const char* streamer, int /*board*/, const char* json)
{
    if (!complete(json) || streamer == nullptr || buffer <= 0) {
        return kInvalidArguments;
    }
    if (!g_prepared) {
        return kBoardNotReady;
    }
    g_streaming = true;
    g_sample = 0;
    g_buffered = 0;
    g_started = std::chrono::steady_clock::now().time_since_epoch().count();
    return kOk;
}

STUB_EXPORT int get_board_data_count(int preset, int* count, int /*board*/, const char* json)
{
    if (!complete(json) || preset != 0) {
        return kInvalidArguments;
    }
    if (!g_streaming) {
        return kBoardNotReady;
    }
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now().time_since_epoch() - std::chrono::steady_clock::duration(g_started);
    g_buffered = static_cast<int>(elapsed.count() * kRate) - g_sample;
    *count = g_buffered;
    return kOk;
}

STUB_EXPORT int get_board_data(int count, int preset, double* data, int /*board*/, const char* json)
{
    if (!complete(json) || preset != 0 || count > g_buffered) {
        return kInvalidArguments;
    }
    for (int s = 0; s < count; ++s, ++g_sample) {
        const auto at = [&](int row) -> double& {
            return data[row * count + s];
        };
        at(0) = g_sample % 256;
        for (int r = 1; r <= 4; ++r) {
            at(r) = r + g_sample / 1000.0;
        }
        for (int r = 5; r <= 7; ++r) {
            at(r) = 0.5;
        }
        at(8) = 36.6;
        at(9) = 95.0;
        at(10) = 1.7e9 + g_sample / 250.0;
        at(11) = g_sample == 30 ? 7.0 : 0.0;
    }
    g_buffered -= count;
    return kOk;
}

STUB_EXPORT int stop_stream(int /*board*/, const char* /*json*/)
{
    const bool was = g_streaming;
    g_streaming = false;
    return was ? kOk : kBoardNotReady;
}

STUB_EXPORT int release_session(int /*board*/, const char* /*json*/)
{
    g_prepared = false;
    g_streaming = false;
    return kOk;
}
