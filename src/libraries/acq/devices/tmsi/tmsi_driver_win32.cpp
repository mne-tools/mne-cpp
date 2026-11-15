//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     tmsi_driver_win32.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     September, 2013
 * @brief    The TMSi SDK (TMSiSDK.dll, installed with the amplifier's driver) behind TmsiTransport.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi_transport.h"

#include "../../detail/acq_shared_library.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <thread>

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

// Structures and entry points of the SDK, as its documentation declares them
struct SIGNAL_FORMAT
{
    ULONG Size;
    ULONG Elements;
    ULONG Type;
    ULONG SubType;
    ULONG Format;
    ULONG Bytes;
    FLOAT UnitGain;
    FLOAT UnitOffSet;
    ULONG UnitId;
    LONG UnitExponent;
    WCHAR Name[40];
    ULONG Port;
    WCHAR PortName[40];
    ULONG SerialNumber;
};

struct FRONTENDINFO
{
    unsigned short NrOfChannels;
    unsigned short SampleRateSetting;
    unsigned short Mode;
    unsigned short maxRS232;
    unsigned long Serial;
    unsigned short NrExg;
    unsigned short NrAux;
    unsigned short HwVersion;
    unsigned short SwVersion;
    unsigned short RecBufSize;
    unsigned short SendBufSize;
    unsigned short NrOfSwChannels;
    unsigned short BaseSf;
    unsigned short Power;
    unsigned short Check;
};

enum TMSiConnectionType
{
    TMSiConnectionUndefined = 0,
    TMSiConnectionFiber,
    TMSiConnectionBluetooth,
    TMSiConnectionUSB,
    TMSiConnectionWifi,
    TMSiConnectionNetwork
};

using POPEN = BOOLEAN(__stdcall*)(void* Handle, const char* DeviceLocator);
using PCLOSE = BOOL(__stdcall*)(HANDLE Handle);
using PSTART = BOOLEAN(__stdcall*)(HANDLE Handle);
using PSTOP = BOOLEAN(__stdcall*)(HANDLE Handle);
using PGETSIGNALFORMAT = SIGNAL_FORMAT*(__stdcall*)(HANDLE Handle, char* FrontEndName);
using PSETSIGNALBUFFER = BOOLEAN(__stdcall*)(HANDLE Handle, PULONG SampleRate, PULONG BufferSize);
using PGETSAMPLES = LONG(__stdcall*)(HANDLE Handle, PULONG SampleBuffer, ULONG Size);
using PFREE = BOOL(__stdcall*)(VOID* Memory);
using PLIBRARYINIT = HANDLE(__stdcall*)(TMSiConnectionType ConnectionType, int* ErrorCode);
using PLIBRARYEXIT = int(__stdcall*)(HANDLE Handle);
using PGETDEVICELIST = char**(__stdcall*)(HANDLE Handle, int* NrOfFrontEnds);
using PGETFRONTENDINFO = BOOLEAN(__stdcall*)(HANDLE Handle, FRONTENDINFO* FrontEndInfo);
using PSETREFCALCULATION = BOOLEAN(__stdcall*)(HANDLE Handle, int OnOrOff);
using PSETMEASURINGMODE = BOOLEAN(__stdcall*)(HANDLE Handle, ULONG Mode, int Value);
using PGETERRORCODE = int(__stdcall*)(HANDLE Handle);

constexpr ULONG kModeNormal = 0;
constexpr ULONG kModeImpedance = 3; // MEASURE_MODE_IMPEDANCE_EX
constexpr int kImpedanceLimit5k = 1;

std::string narrow(const WCHAR* text, std::size_t size)
{
    std::string out;
    for (std::size_t i = 0; i < size && text[i] != 0; ++i) {
        out.push_back(text[i] < 128 ? static_cast<char>(text[i]) : '?');
    }
    return out;
}

class TmsiDriver final : public TmsiTransport
{
public:
    ~TmsiDriver() override
    {
        if (m_handle != nullptr) {
            m_stop(m_handle);
            m_close(m_handle);
            m_libraryExit(m_handle);
        }
    }

    AcqError open()
    {
        // Only from the system directory, where the driver installs it, so no other copy can be planted
        char system[MAX_PATH] = {};
        const UINT length = GetSystemDirectoryA(system, MAX_PATH);
        if (length == 0 || length >= MAX_PATH) {
            return {AcqErrorCode::BackendUnavailable, "the Windows system directory is unknown"};
        }
        auto library = AcqSharedLibrary::load({std::string(system) + (sizeof(void*) == 8 ? "\\TMSiSDK.dll" : "\\TMSiSDK32bit.dll")});
        if (!library) {
            return {AcqErrorCode::BackendUnavailable, "the TMSi SDK is not installed; install the amplifier's driver (" + library.error().text() + ")"};
        }
        m_library = std::move(*library);
        for (AcqError e : {load(m_open, "Open"), load(m_close, "Close"), load(m_start, "Start"), load(m_stop, "Stop"),
                           load(m_getSignalFormat, "GetSignalFormat"), load(m_setSignalBuffer, "SetSignalBuffer"),
                           load(m_getSamples, "GetSamples"), load(m_free, "Free"), load(m_libraryInit, "LibraryInit"),
                           load(m_libraryExit, "LibraryExit"), load(m_getDeviceList, "GetDeviceList"),
                           load(m_getFrontEndInfo, "GetFrontEndInfo"), load(m_setRefCalculation, "SetRefCalculation"),
                           load(m_setMeasuringMode, "SetMeasuringMode"), load(m_getErrorCode, "GetErrorCode")}) {
            if (!e.ok()) {
                return e;
            }
        }

        int error = 0;
        HANDLE handle = m_libraryInit(TMSiConnectionUSB, &error);
        if (handle == nullptr || handle == INVALID_HANDLE_VALUE || error != 0) {
            return {AcqErrorCode::DeviceIo, std::format("the TMSi SDK did not start (error {})", error)};
        }
        int devices = 0;
        char** list = m_getDeviceList(handle, &devices);
        if (list == nullptr || devices == 0) {
            m_libraryExit(handle);
            return {AcqErrorCode::DeviceNotFound, "no TMSi amplifier is connected over USB"};
        }
        if (!m_open(handle, list[0])) {
            m_libraryExit(handle);
            return {AcqErrorCode::DeviceIo, "the TMSi amplifier did not open"};
        }
        m_handle = handle;
        m_stop(m_handle); // in case a previous session did not stop it

        FRONTENDINFO frontEnd{};
        const bool known = m_getFrontEndInfo(m_handle, &frontEnd) != 0;
        SIGNAL_FORMAT* format = m_getSignalFormat(m_handle, nullptr);
        if (format == nullptr) {
            return {AcqErrorCode::DeviceIo, "the TMSi amplifier reports no channels"};
        }
        m_description = narrow(format->PortName, std::size(format->PortName)) + " " + std::to_string(format->SerialNumber);
        if (known) {
            m_description += std::format(", hardware {}, firmware {}", frontEnd.HwVersion, frontEnd.SwVersion);
        }
        for (ULONG i = 0; i < format->Elements; ++i) {
            const SIGNAL_FORMAT& f = format[i];
            m_channels.push_back({narrow(f.Name, std::size(f.Name)), static_cast<TmsiChannelType>(f.Type), static_cast<TmsiUnit>(f.UnitId),
                                  f.UnitGain, f.UnitOffSet, static_cast<int>(f.UnitExponent)});
        }
        m_free(format);
        return {};
    }

    std::string description() const override
    {
        return m_description;
    }

    std::vector<TmsiChannel> channels() const override
    {
        return m_channels;
    }

    AcqExpected<double> setup(const TmsiSetup& setup) override
    {
        m_setup = setup;
        if (!m_setMeasuringMode(m_handle, kModeNormal, 0)) {
            return error("the TMSi amplifier refused normal mode");
        }
        if (!m_setRefCalculation(m_handle, setup.commonAverage ? 1 : 0)) {
            return error("the TMSi amplifier refused the reference");
        }
        return setRate();
    }

    AcqError start() override
    {
        return m_start(m_handle) ? AcqError() : error("the TMSi amplifier did not start");
    }

    void stop() override
    {
        m_stop(m_handle);
    }

    AcqExpected<std::size_t> read(std::span<std::int32_t> frames) override
    {
        const LONG bytes = m_getSamples(m_handle, reinterpret_cast<PULONG>(frames.data()), static_cast<ULONG>(frames.size_bytes()));
        if (bytes < 0) {
            return error("reading the TMSi amplifier failed");
        }
        return static_cast<std::size_t>(bytes) / (m_channels.size() * sizeof(std::int32_t));
    }

    AcqExpected<std::vector<double>> impedances(std::chrono::milliseconds timeout) override
    {
        if (!m_setMeasuringMode(m_handle, kModeImpedance, kImpedanceLimit5k)) {
            return error("the TMSi amplifier refused impedance mode");
        }
        auto restore = [this] {
            m_stop(m_handle);
            m_setMeasuringMode(m_handle, kModeNormal, 0);
        };
        if (auto rate = setRate(); !rate || !m_start(m_handle)) {
            restore();
            return rate ? error("the TMSi amplifier did not start") : rate.error();
        }
        // In impedance mode each sample holds the ExG impedances in ohms
        std::vector<std::int32_t> sample(m_channels.size() * 16);
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (true) {
            auto got = read(sample);
            if (!got) {
                restore();
                return got.error();
            }
            if (*got > 0) {
                break;
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                restore();
                return AcqError(AcqErrorCode::Timeout, "the TMSi amplifier sent no impedances");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        restore();
        std::vector<double> ohms;
        for (std::size_t c = 0; c < m_channels.size(); ++c) {
            if (m_channels[c].type == TmsiChannelType::Exg) {
                ohms.push_back(static_cast<double>(sample[c]));
            }
        }
        return ohms;
    }

private:
    template<typename Function>
    AcqError load(Function& function, const char* name)
    {
        function = reinterpret_cast<Function>(m_library.symbol(name));
        if (function == nullptr) {
            return {AcqErrorCode::BackendUnavailable, std::format("the TMSi SDK lacks {}; update the amplifier's driver", name)};
        }
        return {};
    }

    AcqExpected<double> setRate()
    {
        auto milliHertz = static_cast<ULONG>(std::lround(m_setup.rate * 1000.0));
        ULONG buffer = 0xFFFFFFFF; // the largest buffer the SDK allows
        if (!m_setSignalBuffer(m_handle, &milliHertz, &buffer)) {
            return error("the TMSi amplifier refused the sampling rate");
        }
        return milliHertz / 1000.0;
    }

    AcqError error(const std::string& what) const
    {
        return {AcqErrorCode::DeviceIo, std::format("{} (SDK error {})", what, m_getErrorCode(m_handle))};
    }

    AcqSharedLibrary m_library;
    HANDLE m_handle = nullptr;
    std::string m_description;
    std::vector<TmsiChannel> m_channels;
    TmsiSetup m_setup;

    POPEN m_open = nullptr;
    PCLOSE m_close = nullptr;
    PSTART m_start = nullptr;
    PSTOP m_stop = nullptr;
    PGETSIGNALFORMAT m_getSignalFormat = nullptr;
    PSETSIGNALBUFFER m_setSignalBuffer = nullptr;
    PGETSAMPLES m_getSamples = nullptr;
    PFREE m_free = nullptr;
    PLIBRARYINIT m_libraryInit = nullptr;
    PLIBRARYEXIT m_libraryExit = nullptr;
    PGETDEVICELIST m_getDeviceList = nullptr;
    PGETFRONTENDINFO m_getFrontEndInfo = nullptr;
    PSETREFCALCULATION m_setRefCalculation = nullptr;
    PSETMEASURINGMODE m_setMeasuringMode = nullptr;
    PGETERRORCODE m_getErrorCode = nullptr;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqExpected<std::unique_ptr<TmsiTransport>> mne::detail::openTmsiDriver()
{
    auto driver = std::make_unique<TmsiDriver>();
    if (AcqError e = driver->open(); !e.ok()) {
        return e;
    }
    return std::unique_ptr<TmsiTransport>(std::move(driver));
}
