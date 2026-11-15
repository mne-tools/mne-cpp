//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbamp_driver_win32.cpp
 * @author   Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    The g.USBamp API (gUSBamp.dll, installed with the amplifier's driver) behind GusbampTransport.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbamp_transport.h"

#include "../../detail/acq_shared_library.h"

#include <windows.h>

#include <array>
#include <format>

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

// Structures and entry points of the API, as its documentation declares them
struct GND
{
    BOOL GND1;
    BOOL GND2;
    BOOL GND3;
    BOOL GND4;
};

struct REF
{
    BOOL ref1;
    BOOL ref2;
    BOOL ref3;
    BOOL ref4;
};

struct CHANNEL
{
    UCHAR Channel[16];
};

using POPENDEVICE = HANDLE(__stdcall*)(int iPortNumber);
using POPENDEVICEEX = HANDLE(__stdcall*)(LPSTR lpSerial);
using PCLOSEDEVICE = BOOL(__stdcall*)(HANDLE* hDevice);
using PGETDATA = BOOL(__stdcall*)(HANDLE hDevice, BYTE* pData, DWORD dwSzBuffer, OVERLAPPED* ov);
using PSETBUFFERSIZE = BOOL(__stdcall*)(HANDLE hDevice, WORD wBufferSize);
using PSETSAMPLERATE = BOOL(__stdcall*)(HANDLE hDevice, WORD wSampleRate);
using PSTART = BOOL(__stdcall*)(HANDLE hDevice);
using PSTOP = BOOL(__stdcall*)(HANDLE hDevice);
using PSETCHANNELS = BOOL(__stdcall*)(HANDLE hDevice, UCHAR* ucChannels, UCHAR ucSizeChannels);
using PRESETTRANSFER = BOOL(__stdcall*)(HANDLE hDevice);
using PGETSERIAL = BOOL(__stdcall*)(HANDLE hDevice, LPSTR lpstrSerial, UINT uiSize);
using PENABLETRIGGERLINE = BOOL(__stdcall*)(HANDLE hDevice, BOOL bEnable);
using PGETIMPEDANCE = BOOL(__stdcall*)(HANDLE hDevice, UCHAR Channel, double* Impedance);
using PSETBANDPASS = BOOL(__stdcall*)(HANDLE hDevice, UCHAR ucChannel, int index);
using PSETNOTCH = BOOL(__stdcall*)(HANDLE hDevice, UCHAR ucChannel, int index);
using PSETMODE = BOOL(__stdcall*)(HANDLE hDevice, UCHAR ucMode);
using PSETGROUND = BOOL(__stdcall*)(HANDLE hDevice, GND CommonGround);
using PSETREFERENCE = BOOL(__stdcall*)(HANDLE hDevice, REF CommonReference);
using PSETBIPOLAR = BOOL(__stdcall*)(HANDLE hDevice, CHANNEL bipoChannel);
using PENABLESC = BOOL(__stdcall*)(HANDLE hDevice, BOOL bEnable);
using PSETSLAVE = BOOL(__stdcall*)(HANDLE hDevice, BOOL bSlave);
using PGETLASTERROR = BOOL(__stdcall*)(WORD* wErrorCode, char* pLastError);

constexpr UCHAR kModeNormal = 0;
constexpr int kPorts = 16;          // USB ports the API enumerates
constexpr std::size_t kQueue = 4;   // transfers queued per unit, so none is lost while one is copied
constexpr DWORD kStopWaitMs = 1000; // for pending transfers when stopping

/** The API's entry points. */
struct Api
{
    AcqSharedLibrary library;
    POPENDEVICE openDevice = nullptr;
    POPENDEVICEEX openDeviceEx = nullptr;
    PCLOSEDEVICE closeDevice = nullptr;
    PGETDATA getData = nullptr;
    PSETBUFFERSIZE setBufferSize = nullptr;
    PSETSAMPLERATE setSampleRate = nullptr;
    PSTART start = nullptr;
    PSTOP stop = nullptr;
    PSETCHANNELS setChannels = nullptr;
    PRESETTRANSFER resetTransfer = nullptr;
    PGETSERIAL getSerial = nullptr;
    PENABLETRIGGERLINE enableTriggerLine = nullptr;
    PGETIMPEDANCE getImpedance = nullptr;
    PSETBANDPASS setBandPass = nullptr;
    PSETNOTCH setNotch = nullptr;
    PSETMODE setMode = nullptr;
    PSETGROUND setGround = nullptr;
    PSETREFERENCE setReference = nullptr;
    PSETBIPOLAR setBipolar = nullptr;
    PENABLESC enableSc = nullptr;
    PSETSLAVE setSlave = nullptr;
    PGETLASTERROR getLastError = nullptr;

    /** @return The API's last error message after @p what. */
    [[nodiscard]] AcqError error(const std::string& what) const
    {
        WORD code = 0;
        std::array<char, 1024> text{};
        if (getLastError != nullptr && getLastError(&code, text.data())) {
            return {AcqErrorCode::DeviceIo, std::format("{}: {} (g.USBamp error {})", what, text.data(), code)};
        }
        return {AcqErrorCode::DeviceIo, what};
    }
};

template<typename Function>
AcqError resolve(const AcqSharedLibrary& library, Function& function, const char* name)
{
    function = reinterpret_cast<Function>(library.symbol(name));
    if (function == nullptr) {
        return {AcqErrorCode::BackendUnavailable, std::format("the g.USBamp API lacks {}; update the amplifier's driver", name)};
    }
    return {};
}

AcqExpected<std::shared_ptr<const Api>> loadApi()
{
    // Only from the system directory, where the driver installs it, so no other copy can be planted
    char system[MAX_PATH] = {};
    const UINT length = GetSystemDirectoryA(system, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return AcqError(AcqErrorCode::BackendUnavailable, "the Windows system directory is unknown");
    }
    auto library = AcqSharedLibrary::load({std::string(system) + "\\gUSBamp.dll"});
    if (!library) {
        return AcqError(AcqErrorCode::BackendUnavailable,
                        "the g.USBamp API is not installed; install the amplifier's driver (" + library.error().text() + ")");
    }
    auto api = std::make_shared<Api>();
    api->library = std::move(*library);
    const AcqSharedLibrary& l = api->library;
    for (const AcqError& e :
         {resolve(l, api->openDevice, "GT_OpenDevice"), resolve(l, api->openDeviceEx, "GT_OpenDeviceEx"),
          resolve(l, api->closeDevice, "GT_CloseDevice"), resolve(l, api->getData, "GT_GetData"),
          resolve(l, api->setBufferSize, "GT_SetBufferSize"), resolve(l, api->setSampleRate, "GT_SetSampleRate"),
          resolve(l, api->start, "GT_Start"), resolve(l, api->stop, "GT_Stop"), resolve(l, api->setChannels, "GT_SetChannels"),
          resolve(l, api->resetTransfer, "GT_ResetTransfer"), resolve(l, api->getSerial, "GT_GetSerial"),
          resolve(l, api->enableTriggerLine, "GT_EnableTriggerLine"), resolve(l, api->getImpedance, "GT_GetImpedance"),
          resolve(l, api->setBandPass, "GT_SetBandPass"), resolve(l, api->setNotch, "GT_SetNotch"), resolve(l, api->setMode, "GT_SetMode"),
          resolve(l, api->setGround, "GT_SetGround"), resolve(l, api->setReference, "GT_SetReference"),
          resolve(l, api->setBipolar, "GT_SetBipolar"), resolve(l, api->enableSc, "GT_EnableSC"), resolve(l, api->setSlave, "GT_SetSlave"),
          resolve(l, api->getLastError, "GT_GetLastError")}) {
        if (!e.ok()) {
            return e;
        }
    }
    return std::shared_ptr<const Api>(std::move(api));
}

/** One opened unit with its queued transfers. */
struct Unit
{
    std::string serial;
    HANDLE handle = nullptr;
    std::array<std::vector<std::byte>, kQueue> buffers;
    std::array<OVERLAPPED, kQueue> overlapped{};
};

class GusbampDriver final : public GusbampTransport
{
public:
    explicit GusbampDriver(std::shared_ptr<const Api> api)
    : m_api(std::move(api))
    {
    }

    ~GusbampDriver() override
    {
        stop();
        for (Unit& unit : m_units) {
            m_api->closeDevice(&unit.handle);
        }
    }

    GusbampDriver(const GusbampDriver&) = delete;
    GusbampDriver& operator=(const GusbampDriver&) = delete;

    AcqError open(std::span<const std::string> serials)
    {
        for (const std::string& serial : serials) {
            std::string name = serial; // the API takes a mutable string
            HANDLE handle = m_api->openDeviceEx(name.data());
            if (handle == nullptr) {
                return {AcqErrorCode::DeviceNotFound, std::format("no g.USBamp {} is connected", serial)};
            }
            Unit& unit = m_units.emplace_back();
            unit.serial = serial;
            unit.handle = handle;
        }
        return {};
    }

    std::vector<std::string> serials() const override
    {
        std::vector<std::string> out;
        for (const Unit& unit : m_units) {
            out.push_back(unit.serial);
        }
        return out;
    }

    std::string description() const override
    {
        std::string text = "g.USBamp";
        for (std::size_t u = 0; u < m_units.size(); ++u) {
            text += (u == 0 ? " " : ", ") + m_units[u].serial + (u == 0 && m_units.size() > 1 ? " (master)" : "");
        }
        return text;
    }

    AcqError start(const GusbampSetup& setup) override
    {
        m_setup = setup;
        for (std::size_t u = 0; u < m_units.size(); ++u) {
            if (AcqError e = configure(m_units[u], u); !e.ok()) {
                return e;
            }
        }
        // The slaves start first, so the master's clock reaches every unit; then the transfers are queued
        for (std::size_t n = 0; n < m_units.size(); ++n) {
            const std::size_t u = (n + 1) % m_units.size();
            Unit& unit = m_units[u];
            for (std::size_t q = 0; q < kQueue; ++q) {
                unit.buffers[q].assign(setup.transfer_bytes(u), std::byte{0});
                unit.overlapped[q] = {};
                unit.overlapped[q].hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
                if (unit.overlapped[q].hEvent == nullptr) {
                    stop();
                    return {AcqErrorCode::OutOfMemory, "no event for a g.USBamp transfer"};
                }
            }
            if (!m_api->start(unit.handle)) {
                stop();
                return m_api->error("g.USBamp " + unit.serial + " did not start");
            }
            m_started = true;
            for (std::size_t q = 0; q < kQueue; ++q) {
                if (!queue(unit, q)) {
                    stop();
                    return m_api->error("g.USBamp " + unit.serial + " refused a transfer");
                }
            }
        }
        m_next = 0;
        m_held = false;
        m_transfers.assign(m_units.size(), {});
        return {};
    }

    void stop() override
    {
        if (!m_started) {
            return;
        }
        for (Unit& unit : m_units) {
            for (OVERLAPPED& overlapped : unit.overlapped) {
                if (overlapped.hEvent != nullptr) {
                    WaitForSingleObject(overlapped.hEvent, kStopWaitMs);
                    CloseHandle(overlapped.hEvent);
                    overlapped.hEvent = nullptr;
                }
            }
            m_api->stop(unit.handle);
            m_api->resetTransfer(unit.handle);
        }
        m_started = false;
    }

    AcqExpected<std::span<const std::span<const std::byte>>> read(std::chrono::milliseconds timeout) override
    {
        if (m_held) {
            // The previous transfer has been copied: queue its buffers again
            for (Unit& unit : m_units) {
                if (!queue(unit, m_next)) {
                    return m_api->error("g.USBamp " + unit.serial + " refused a transfer");
                }
            }
            m_next = (m_next + 1) % kQueue;
            m_held = false;
        }
        for (std::size_t u = 0; u < m_units.size(); ++u) {
            Unit& unit = m_units[u];
            const DWORD waited = WaitForSingleObject(unit.overlapped[m_next].hEvent, static_cast<DWORD>(timeout.count()));
            if (waited == WAIT_TIMEOUT) {
                return AcqError(AcqErrorCode::Timeout);
            }
            DWORD bytes = 0;
            if (waited != WAIT_OBJECT_0 || !GetOverlappedResult(unit.handle, &unit.overlapped[m_next], &bytes, FALSE)) {
                return AcqError(AcqErrorCode::DeviceIo, std::format("the transfer of g.USBamp {} failed (error {})", unit.serial, GetLastError()));
            }
            if (bytes != unit.buffers[m_next].size()) {
                return AcqError(AcqErrorCode::DeviceIo,
                                std::format("g.USBamp {} delivered {} of {} bytes", unit.serial, bytes, unit.buffers[m_next].size()));
            }
            m_transfers[u] = unit.buffers[m_next];
        }
        m_held = true;
        return std::span<const std::span<const std::byte>>(m_transfers);
    }

    AcqExpected<std::vector<double>> impedances(std::span<const std::uint8_t> inputs) override
    {
        if (m_started) {
            return AcqError(AcqErrorCode::InvalidState, "the g.USBamp measures impedances only while stopped");
        }
        std::vector<double> ohms;
        ohms.reserve(m_units.size() * inputs.size());
        for (const Unit& unit : m_units) {
            for (const std::uint8_t input : inputs) {
                double value = 0.0;
                if (!m_api->getImpedance(unit.handle, input, &value)) {
                    return m_api->error(std::format("g.USBamp {} did not measure input {}", unit.serial, input));
                }
                ohms.push_back(value);
            }
        }
        return ohms;
    }

private:
    /** Raw signals: no filters, unipolar, no common ground or reference; the master is the first unit. */
    AcqError configure(const Unit& unit, std::size_t index)
    {
        std::vector<UCHAR> inputs(m_setup.inputs.begin(), m_setup.inputs.end());
        const bool master = index == 0;
        const std::string name = "g.USBamp " + unit.serial;
        if (!m_api->setMode(unit.handle, kModeNormal) || !m_api->setSlave(unit.handle, master ? FALSE : TRUE) ||
            !m_api->setSampleRate(unit.handle, m_setup.rate) || !m_api->setBufferSize(unit.handle, m_setup.scans) ||
            !m_api->setChannels(unit.handle, inputs.data(), static_cast<UCHAR>(inputs.size())) ||
            !m_api->enableTriggerLine(unit.handle, m_setup.trigger && master ? TRUE : FALSE) || !m_api->enableSc(unit.handle, FALSE) ||
            !m_api->setBipolar(unit.handle, CHANNEL{}) || !m_api->setReference(unit.handle, REF{}) || !m_api->setGround(unit.handle, GND{})) {
            return m_api->error(name + " refused the setup");
        }
        for (const UCHAR input : inputs) {
            if (!m_api->setBandPass(unit.handle, input, -1) || !m_api->setNotch(unit.handle, input, -1)) {
                return m_api->error(name + " refused to turn off its filters");
            }
        }
        return {};
    }

    bool queue(Unit& unit, std::size_t slot)
    {
        std::vector<std::byte>& buffer = unit.buffers[slot];
        return m_api->getData(unit.handle, reinterpret_cast<BYTE*>(buffer.data()), static_cast<DWORD>(buffer.size()), &unit.overlapped[slot]) !=
            FALSE;
    }

    std::shared_ptr<const Api> m_api;
    std::vector<Unit> m_units; // master first
    GusbampSetup m_setup;
    bool m_started = false;
    std::size_t m_next = 0; // queue slot of the next transfer
    bool m_held = false;    // the slot's buffers were handed out by read() and are not queued
    std::vector<std::span<const std::byte>> m_transfers;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqExpected<std::vector<std::string>> mne::detail::list_gusbamp_units()
{
    auto api = loadApi();
    if (!api) {
        return api.error();
    }
    std::vector<std::string> serials;
    for (int port = 0; port < kPorts; ++port) {
        HANDLE handle = (*api)->openDevice(port);
        if (handle == nullptr) {
            continue;
        }
        std::array<char, 16> serial{};
        if ((*api)->getSerial(handle, serial.data(), static_cast<UINT>(serial.size()))) {
            serials.emplace_back(serial.data());
        }
        (*api)->closeDevice(&handle);
    }
    return serials;
}

//=============================================================================================================

AcqExpected<std::unique_ptr<GusbampTransport>> mne::detail::open_gusbamp_driver(std::span<const std::string> serials)
{
    auto api = loadApi();
    if (!api) {
        return api.error();
    }
    std::vector<std::string> wanted(serials.begin(), serials.end());
    if (wanted.empty()) {
        auto units = list_gusbamp_units();
        if (!units) {
            return units.error();
        }
        if (units->empty()) {
            return AcqError(AcqErrorCode::DeviceNotFound, "no g.USBamp is connected");
        }
        wanted.push_back(units->front());
    }
    auto driver = std::make_unique<GusbampDriver>(std::move(*api));
    if (AcqError e = driver->open(wanted); !e.ok()) {
        return e;
    }
    return std::unique_ptr<GusbampTransport>(std::move(driver));
}
