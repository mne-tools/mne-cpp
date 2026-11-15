//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eego_driver.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    The eego SDK library (eego-SDK.dll, libeego-SDK.so) behind EegoTransport, loaded at run time.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eego_transport.h"

#include "../../detail/acq_shared_library.h"

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <mutex>
#include <thread>
#include <type_traits>

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

// Structures of the SDK's C interface, as its documentation declares them
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

constexpr int kMaxChannels = 1024;
constexpr int kMaxAmplifiers = 16;

/** The C entry points of the SDK. */
struct Sdk
{
    AcqSharedLibrary library;
    void (*init)() = nullptr;
    void (*exit)() = nullptr;
    int (*amplifiersInfo)(AmplifierInfo*, int) = nullptr;
    int (*openAmplifier)(int) = nullptr;
    int (*closeAmplifier)(int) = nullptr;
    int (*amplifierType)(int, char*, int) = nullptr;
    int (*amplifierChannels)(int, ChannelInfo*, int) = nullptr;
    int (*rates)(int, int*, int) = nullptr;
    int (*referenceRanges)(int, double*, int) = nullptr;
    int (*bipolarRanges)(int, double*, int) = nullptr;
    int (*openEegStream)(int, int, double, double, const ChannelInfo*, int) = nullptr;
    int (*openImpedanceStream)(int, const ChannelInfo*, int) = nullptr;
    int (*closeStream)(int) = nullptr;
    int (*streamChannels)(int, ChannelInfo*, int) = nullptr;
    int (*prefetch)(int) = nullptr;
    int (*getData)(int, double*, int) = nullptr;
    int (*errorString)(char*, int) = nullptr;

    ~Sdk()
    {
        if (exit != nullptr) {
            exit();
        }
    }

    /** @return The SDK's last error after @p what. */
    [[nodiscard]] AcqError error(const std::string& what, int code) const
    {
        std::array<char, 512> text{};
        errorString(text.data(), static_cast<int>(text.size()));
        return {AcqErrorCode::DeviceIo, std::format("{}: {} (eego error {})", what, text.data(), code)};
    }
};

AcqExpected<std::shared_ptr<const Sdk>> loadSdk(const std::vector<std::string>& candidates)
{
    // The SDK is initialised once per process while any amplifier or listing uses it
    static std::mutex mutex;
    static std::map<std::vector<std::string>, std::weak_ptr<const Sdk>> loaded;
    const std::lock_guard<std::mutex> lock(mutex);
    if (auto existing = loaded[candidates].lock()) {
        return existing;
    }
    auto library = AcqSharedLibrary::load(candidates);
    if (!library) {
        return AcqError(AcqErrorCode::BackendUnavailable, "the eego SDK " + library.error().text() + "; install it, pass its path as sdk=, or use simulate=1 for the simulator");
    }
    auto sdk = std::make_shared<Sdk>();
    sdk->library = std::move(*library);
    std::string missing;
    const auto bind = [&](auto& function, const char* name) {
        function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(sdk->library.symbol(name));
        if (function == nullptr) {
            missing += (missing.empty() ? "" : ", ") + std::string(name);
        }
    };
    bind(sdk->init, "eemagine_sdk_init");
    bind(sdk->exit, "eemagine_sdk_exit");
    bind(sdk->amplifiersInfo, "eemagine_sdk_get_amplifiers_info");
    bind(sdk->openAmplifier, "eemagine_sdk_open_amplifier");
    bind(sdk->closeAmplifier, "eemagine_sdk_close_amplifier");
    bind(sdk->amplifierType, "eemagine_sdk_get_amplifier_type");
    bind(sdk->amplifierChannels, "eemagine_sdk_get_amplifier_channel_list");
    bind(sdk->rates, "eemagine_sdk_get_amplifier_sampling_rates_available");
    bind(sdk->referenceRanges, "eemagine_sdk_get_amplifier_reference_ranges_available");
    bind(sdk->bipolarRanges, "eemagine_sdk_get_amplifier_bipolar_ranges_available");
    bind(sdk->openEegStream, "eemagine_sdk_open_eeg_stream");
    bind(sdk->openImpedanceStream, "eemagine_sdk_open_impedance_stream");
    bind(sdk->closeStream, "eemagine_sdk_close_stream");
    bind(sdk->streamChannels, "eemagine_sdk_get_stream_channel_list");
    bind(sdk->prefetch, "eemagine_sdk_prefetch");
    bind(sdk->getData, "eemagine_sdk_get_data");
    bind(sdk->errorString, "eemagine_sdk_get_error_string");
    if (!missing.empty()) {
        const AcqError error(AcqErrorCode::UnsupportedVersion, sdk->library.path() + " lacks " + missing + "; update the eego SDK");
        sdk->exit = nullptr; // never initialised
        return error;
    }
    sdk->init();
    loaded[candidates] = sdk;
    return std::shared_ptr<const Sdk>(std::move(sdk));
}

std::vector<EegoChannel> toChannels(const ChannelInfo* channels, int count)
{
    std::vector<EegoChannel> out;
    out.reserve(static_cast<std::size_t>(count));
    for (int c = 0; c < count; ++c) {
        out.push_back({channels[c].index, static_cast<EegoChannelType>(channels[c].type)});
    }
    return out;
}

std::vector<ChannelInfo> toInfo(const std::vector<EegoChannel>& channels, bool referenceOnly)
{
    std::vector<ChannelInfo> out;
    for (const EegoChannel& channel : channels) {
        const bool input = channel.type == EegoChannelType::Reference || (!referenceOnly && channel.type == EegoChannelType::Bipolar);
        if (input) {
            out.push_back({channel.index, static_cast<int>(channel.type)});
        }
    }
    return out;
}

template<typename T>
std::vector<T> list(int (*query)(int, T*, int), int amplifier)
{
    std::array<T, 64> values{};
    const int n = query(amplifier, values.data(), static_cast<int>(values.size()));
    return n > 0 ? std::vector<T>(values.begin(), values.begin() + std::min<int>(n, static_cast<int>(values.size()))) : std::vector<T>();
}

class EegoDriver final : public EegoTransport
{
public:
    EegoDriver(std::shared_ptr<const Sdk> sdk, int amplifier, std::string serial)
    : m_sdk(std::move(sdk))
    , m_amplifier(amplifier)
    , m_serial(std::move(serial))
    {
    }

    ~EegoDriver() override
    {
        stop();
        m_sdk->closeAmplifier(m_amplifier);
    }

    EegoDriver(const EegoDriver&) = delete;
    EegoDriver& operator=(const EegoDriver&) = delete;

    AcqError open()
    {
        if (const int e = m_sdk->openAmplifier(m_amplifier); e < 0) {
            return m_sdk->error("eego amplifier " + m_serial + " did not open", e);
        }
        std::array<char, 64> type{};
        m_sdk->amplifierType(m_amplifier, type.data(), static_cast<int>(type.size()));
        type.back() = '\0';
        m_description = std::string(type.data()) + " " + m_serial;
        std::array<ChannelInfo, kMaxChannels> channels{};
        const int count = m_sdk->amplifierChannels(m_amplifier, channels.data(), kMaxChannels);
        if (count < 0) {
            return m_sdk->error("eego amplifier " + m_serial + " reports no channels", count);
        }
        m_inputs = toChannels(channels.data(), std::min(count, kMaxChannels));
        m_channels = m_inputs;
        m_channels.push_back({count, EegoChannelType::Trigger});
        m_channels.push_back({count + 1, EegoChannelType::SampleCounter});
        return {};
    }

    std::string description() const override
    {
        return m_description;
    }

    std::vector<EegoChannel> channels() const override
    {
        return m_channels;
    }

    std::vector<int> rates() const override
    {
        return list(m_sdk->rates, m_amplifier);
    }

    std::vector<double> reference_ranges() const override
    {
        return list(m_sdk->referenceRanges, m_amplifier);
    }

    std::vector<double> bipolar_ranges() const override
    {
        return list(m_sdk->bipolarRanges, m_amplifier);
    }

    AcqError start(const EegoSetup& setup) override
    {
        const std::vector<ChannelInfo> inputs = toInfo(m_inputs, false);
        const int stream = m_sdk->openEegStream(m_amplifier, setup.rate, setup.reference_range, setup.bipolar_range, inputs.data(),
                                                static_cast<int>(inputs.size()));
        if (stream < 0) {
            return m_sdk->error("eego amplifier " + m_serial + " refused the stream", stream);
        }
        m_stream = stream;
        // The stream must deliver the inputs, then trigger and counter, as described when opening
        std::array<ChannelInfo, kMaxChannels> channels{};
        const int count = m_sdk->streamChannels(m_stream, channels.data(), kMaxChannels);
        std::vector<EegoChannel> types = toChannels(channels.data(), std::max(0, std::min(count, kMaxChannels)));
        const auto sameType = [](const EegoChannel& a, const EegoChannel& b) {
            return a.type == b.type;
        };
        if (!std::equal(types.begin(), types.end(), m_channels.begin(), m_channels.end(), sameType)) {
            stop();
            return {AcqErrorCode::UnsupportedFormat, std::format("eego amplifier {} streams {} channels in an unexpected order", m_serial, count)};
        }
        return {};
    }

    void stop() override
    {
        if (m_stream >= 0) {
            m_sdk->closeStream(m_stream);
            m_stream = -1;
        }
    }

    AcqExpected<std::span<const double>> read() override
    {
        auto got = fetch(m_stream);
        if (!got) {
            return got.error();
        }
        return std::span<const double>(m_samples.data(), *got);
    }

    AcqExpected<std::vector<double>> impedances(std::chrono::milliseconds timeout) override
    {
        if (m_stream >= 0) {
            return AcqError(AcqErrorCode::InvalidState, "the eego amplifier measures impedances only while stopped");
        }
        const std::vector<ChannelInfo> inputs = toInfo(m_inputs, true);
        const int stream = m_sdk->openImpedanceStream(m_amplifier, inputs.data(), static_cast<int>(inputs.size()));
        if (stream < 0) {
            return m_sdk->error("eego amplifier " + m_serial + " refused the impedance measurement", stream);
        }
        std::array<ChannelInfo, kMaxChannels> channels{};
        const int count = std::min(m_sdk->streamChannels(stream, channels.data(), kMaxChannels), kMaxChannels);
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        auto got = fetch(stream);
        while (got && *got == 0 && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            got = fetch(stream);
        }
        m_sdk->closeStream(stream);
        if (!got) {
            return got.error();
        }
        if (*got < static_cast<std::size_t>(std::max(count, 1))) {
            return AcqError(AcqErrorCode::Timeout, "the eego amplifier sent no impedances");
        }
        // Each sample holds the ohms of the referential inputs, then of the reference and ground electrodes
        std::vector<double> ohms;
        for (int c = 0; c < count; ++c) {
            if (static_cast<EegoChannelType>(channels[static_cast<std::size_t>(c)].type) == EegoChannelType::Reference) {
                ohms.push_back(m_samples[static_cast<std::size_t>(c)]);
            }
        }
        return ohms;
    }

private:
    /** @return Values read from @p stream into m_samples, 0 if none are buffered. */
    AcqExpected<std::size_t> fetch(int stream)
    {
        const int bytes = m_sdk->prefetch(stream);
        if (bytes < 0) {
            return m_sdk->error("reading eego amplifier " + m_serial + " failed", bytes);
        }
        m_samples.resize(std::max(m_samples.size(), static_cast<std::size_t>(bytes) / sizeof(double)));
        if (bytes == 0) {
            return std::size_t(0);
        }
        const int got = m_sdk->getData(stream, m_samples.data(), bytes);
        if (got < 0) {
            return m_sdk->error("reading eego amplifier " + m_serial + " failed", got);
        }
        return static_cast<std::size_t>(got) / sizeof(double);
    }

    std::shared_ptr<const Sdk> m_sdk;
    int m_amplifier;
    std::string m_serial;
    std::string m_description;
    std::vector<EegoChannel> m_inputs;   // the amplifier's channels
    std::vector<EegoChannel> m_channels; // the EEG stream's channels
    int m_stream = -1;
    std::vector<double> m_samples; // the last read
};

AcqExpected<std::vector<AmplifierInfo>> amplifiers(const Sdk& sdk)
{
    std::array<AmplifierInfo, kMaxAmplifiers> found{};
    const int n = sdk.amplifiersInfo(found.data(), kMaxAmplifiers);
    if (n < 0) {
        return sdk.error("the eego SDK cannot list the amplifiers", n);
    }
    std::vector<AmplifierInfo> out(found.begin(), found.begin() + std::min(n, kMaxAmplifiers));
    for (AmplifierInfo& amplifier : out) {
        amplifier.serial[sizeof(amplifier.serial) - 1] = '\0';
    }
    return out;
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqExpected<std::vector<std::string>> mne::detail::list_eego_amplifiers(const std::vector<std::string>& library)
{
    auto sdk = loadSdk(library);
    if (!sdk) {
        return sdk.error();
    }
    auto found = amplifiers(**sdk);
    if (!found) {
        return found.error();
    }
    std::vector<std::string> serials;
    for (const AmplifierInfo& amplifier : *found) {
        serials.emplace_back(amplifier.serial);
    }
    return serials;
}

//=============================================================================================================

AcqExpected<std::unique_ptr<EegoTransport>> mne::detail::open_eego_driver(const std::vector<std::string>& library, const std::string& serial)
{
    auto sdk = loadSdk(library);
    if (!sdk) {
        return sdk.error();
    }
    auto found = amplifiers(**sdk);
    if (!found) {
        return found.error();
    }
    const auto it = std::find_if(found->begin(), found->end(), [&](const AmplifierInfo& a) { return serial.empty() || serial == a.serial; });
    if (it == found->end()) {
        return AcqError(AcqErrorCode::DeviceNotFound, serial.empty() ? "no eego amplifier is connected" : "no eego amplifier " + serial + " is connected");
    }
    auto driver = std::make_unique<EegoDriver>(std::move(*sdk), it->id, it->serial);
    if (AcqError e = driver->open(); !e.ok()) {
        return e;
    }
    return std::unique_ptr<EegoTransport>(std::move(driver));
}
