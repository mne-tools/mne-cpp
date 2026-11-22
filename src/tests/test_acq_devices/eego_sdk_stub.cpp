//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     eego_sdk_stub.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    A stand-in for the eego SDK library: one amplifier with known samples, for testing the eego:// driver.
 *
 * Amplifier 7, serial STUB-1: four referential inputs and one bipolar input; 500 or 1000 Hz. Every read delivers
 * ten samples; sample k of input c is (c + 1) uV + k nV, the trigger word is 3 for samples 20 to 24, the counter
 * is k. The impedance of referential input c is (c + 1) kOhm.
 */

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstring>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(_WIN32)
#define STUB_EXPORT extern "C" __declspec(dllexport)
#else
#define STUB_EXPORT extern "C" __attribute__((visibility("default")))
#endif

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

// The SDK's structures, as its documentation declares them
struct AmplifierInfo
{
    int id;
    char serial[64];
};

struct ChannelInfo
{
    int index;
    int type;
};

constexpr int kAmplifier = 7;
constexpr int kEegStream = 1;
constexpr int kImpedanceStream = 2;
constexpr int kNotFound = -3;
constexpr int kIncorrectValue = -4;
constexpr int kReference = 0;
constexpr int kBipolar = 1;
constexpr int kTrigger = 5;
constexpr int kCounter = 6;
constexpr int kImpedanceReference = 7;
constexpr int kImpedanceGround = 8;
constexpr int kInputs = 5;
constexpr int kSamplesPerRead = 10;

const ChannelInfo kAmplifierChannels[kInputs] = {{0, kReference}, {1, kReference}, {2, kReference}, {3, kReference}, {4, kBipolar}};

bool g_open = false;
int g_sample = 0;

int copyChannels(const ChannelInfo* from, int count, ChannelInfo* to, int size)
{
    std::copy_n(from, std::min(count, size), to);
    return count;
}

int streamChannels(int stream, ChannelInfo* out, int size)
{
    if (stream == kEegStream) {
        const ChannelInfo channels[] = {{0, kReference}, {1, kReference}, {2, kReference}, {3, kReference}, {4, kBipolar}, {5, kTrigger}, {6, kCounter}};
        return copyChannels(channels, 7, out, size);
    }
    if (stream == kImpedanceStream) {
        const ChannelInfo channels[] = {{0, kReference}, {1, kReference}, {2, kReference}, {3, kReference}, {4, kImpedanceReference}, {5, kImpedanceGround}};
        return copyChannels(channels, 6, out, size);
    }
    return kNotFound;
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

STUB_EXPORT void eemagine_sdk_init()
{
}

STUB_EXPORT void eemagine_sdk_exit()
{
}

STUB_EXPORT int eemagine_sdk_get_version()
{
    return 57168;
}

STUB_EXPORT int eemagine_sdk_get_amplifiers_info(AmplifierInfo* info, int size)
{
    if (size > 0) {
        info[0].id = kAmplifier;
        std::strcpy(info[0].serial, "STUB-1");
    }
    return 1;
}

STUB_EXPORT int eemagine_sdk_open_amplifier(int id)
{
    if (id != kAmplifier) {
        return kNotFound;
    }
    g_open = true;
    return 0;
}

STUB_EXPORT int eemagine_sdk_close_amplifier(int id)
{
    g_open = false;
    return id == kAmplifier ? 0 : kNotFound;
}

STUB_EXPORT int eemagine_sdk_get_amplifier_type(int /*id*/, char* type, int size)
{
    std::strncpy(type, "EE-STUB", static_cast<std::size_t>(size));
    return 0;
}

STUB_EXPORT int eemagine_sdk_get_amplifier_channel_list(int /*id*/, ChannelInfo* out, int size)
{
    return copyChannels(kAmplifierChannels, kInputs, out, size);
}

STUB_EXPORT int eemagine_sdk_get_amplifier_sampling_rates_available(int /*id*/, int* rates, int size)
{
    const int all[] = {500, 1000};
    std::copy_n(all, std::min(2, size), rates);
    return 2;
}

STUB_EXPORT int eemagine_sdk_get_amplifier_reference_ranges_available(int /*id*/, double* ranges, int size)
{
    const double all[] = {1.0, 0.75};
    std::copy_n(all, std::min(2, size), ranges);
    return 2;
}

STUB_EXPORT int eemagine_sdk_get_amplifier_bipolar_ranges_available(int /*id*/, double* ranges, int size)
{
    if (size > 0) {
        ranges[0] = 4.0;
    }
    return 1;
}

STUB_EXPORT int eemagine_sdk_open_eeg_stream(int id, int rate, double /*reference*/, double /*bipolar*/, const ChannelInfo* /*channels*/, int /*count*/)
{
    if (!g_open || id != kAmplifier || (rate != 500 && rate != 1000)) {
        return kIncorrectValue;
    }
    g_sample = 0;
    return kEegStream;
}

STUB_EXPORT int eemagine_sdk_open_impedance_stream(int id, const ChannelInfo* /*channels*/, int /*count*/)
{
    return g_open && id == kAmplifier ? kImpedanceStream : kIncorrectValue;
}

STUB_EXPORT int eemagine_sdk_close_stream(int stream)
{
    return stream == kEegStream || stream == kImpedanceStream ? 0 : kNotFound;
}

STUB_EXPORT int eemagine_sdk_get_stream_channel_list(int stream, ChannelInfo* out, int size)
{
    return streamChannels(stream, out, size);
}

STUB_EXPORT int eemagine_sdk_get_stream_channel_count(int stream)
{
    return streamChannels(stream, nullptr, 0);
}

STUB_EXPORT int eemagine_sdk_prefetch(int stream)
{
    const int samples = stream == kEegStream ? kSamplesPerRead : 1;
    return samples * streamChannels(stream, nullptr, 0) * static_cast<int>(sizeof(double));
}

STUB_EXPORT int eemagine_sdk_get_data(int stream, double* buffer, int bytes)
{
    if (stream == kImpedanceStream) {
        const double ohms[] = {1e3, 2e3, 3e3, 4e3, 500.0, 600.0};
        std::memcpy(buffer, ohms, std::min<std::size_t>(sizeof(ohms), static_cast<std::size_t>(bytes)));
        return bytes;
    }
    const int width = streamChannels(kEegStream, nullptr, 0);
    for (int s = 0; s < bytes / static_cast<int>(sizeof(double)) / width; ++s, ++g_sample) {
        double* frame = buffer + s * width;
        for (int c = 0; c < kInputs; ++c) {
            frame[c] = (c + 1) * 1e-6 + g_sample * 1e-9;
        }
        frame[kInputs] = g_sample >= 20 && g_sample < 25 ? 3.0 : 0.0;
        frame[kInputs + 1] = g_sample;
    }
    return bytes;
}

STUB_EXPORT int eemagine_sdk_get_error_string(char* text, int size)
{
    std::strncpy(text, "stub error", static_cast<std::size_t>(size));
    return 0;
}
