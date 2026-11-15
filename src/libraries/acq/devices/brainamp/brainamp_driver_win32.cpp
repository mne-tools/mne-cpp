//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainamp_driver_win32.cpp
 * @author   Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    The BrainAmp Windows driver (USB adapter or ISA/PCI host adapter) behind BrainAmpTransport.
 *
 * Talks to the kernel driver through DeviceIoControl with the codes and BA_SETUP layout of Brain Products'
 * BrainAmpIoCtl.h, which is not redistributable and is found through MNE_BRAINAMP_SDK_DIR.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainamp_transport.h"

#include <windows.h>

#include <BrainAmpIoCtl.h>

#include <algorithm>
#include <cstring>
#include <format>
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

AcqError ioError(const char* what)
{
    return {AcqErrorCode::DeviceIo, std::format("BrainAmp {} failed, Windows error {}", what, ::GetLastError())};
}

class BrainAmpDriver final : public BrainAmpTransport
{
public:
    BrainAmpDriver(HANDLE device, bool usb, DWORD version)
    : m_device(device)
    , m_usb(usb)
    , m_version(version)
    {
    }

    ~BrainAmpDriver() override
    {
        stop();
        ::CloseHandle(m_device);
    }

    BrainAmpDriver(const BrainAmpDriver&) = delete;
    BrainAmpDriver& operator=(const BrainAmpDriver&) = delete;

    std::string description() const override
    {
        return std::format("{} adapter, driver {}.{:02}.{:04}", m_usb ? "USB" : "ISA/PCI", m_version / 1000000,
                           (m_version % 1000000) / 10000, m_version % 10000);
    }

    std::array<BrainAmpAmplifier, 4> amplifiers() override
    {
        USHORT types[4] = {};
        DWORD returned = 0;
        std::array<BrainAmpAmplifier, 4> slots{};
        if (::DeviceIoControl(m_device, IOCTL_BA_AMPLIFIER_TYPE, nullptr, 0, types, sizeof(types), &returned, nullptr)) {
            for (std::size_t i = 0; i < slots.size(); ++i) {
                slots[i] = static_cast<BrainAmpAmplifier>(types[i]);
            }
        }
        return slots;
    }

    AcqError setup(const BrainAmpSetup& setup) override
    {
        BA_SETUP ba;
        std::memset(&ba, 0, sizeof(ba));
        ba.nChannels = static_cast<decltype(ba.nChannels)>(setup.channels.size());
        for (std::size_t i = 0; i < setup.channels.size(); ++i) {
            ba.nChannelList[i] = static_cast<std::remove_cvref_t<decltype(ba.nChannelList[0])>>(setup.channels[i]);
            ba.nResolution[i] = static_cast<std::remove_cvref_t<decltype(ba.nResolution[0])>>(setup.resolution);
            ba.n250Hertz[i] = setup.lowPass250Hz ? 1 : 0;
            ba.nDCCoupling[i] = setup.dcCoupling ? 1 : 0;
        }
        ba.nPoints = static_cast<decltype(ba.nPoints)>(setup.points);
        ba.nHoldValue = 0; // marker word without trigger
        DWORD returned = 0;
        if (!::DeviceIoControl(m_device, IOCTL_BA_SETUP, &ba, sizeof(ba), nullptr, 0, &returned, nullptr)) {
            return ioError("setup");
        }
        USHORT pullUp = 0; // trigger inputs are active high
        if (!::DeviceIoControl(m_device, IOCTL_BA_DIGITALINPUT_PULL_UP, &pullUp, sizeof(pullUp), nullptr, 0, &returned, nullptr)) {
            return ioError("trigger input set-up");
        }
        return {};
    }

    AcqError start() override
    {
        LONG type = 1; // impedance measurement is 0, data acquisition 1
        DWORD returned = 0;
        if (!::DeviceIoControl(m_device, IOCTL_BA_START, &type, sizeof(type), nullptr, 0, &returned, nullptr)) {
            return ioError("start");
        }
        m_running = true;
        return {};
    }

    void stop() override
    {
        if (m_running) {
            DWORD returned = 0;
            ::DeviceIoControl(m_device, IOCTL_BA_STOP, nullptr, 0, nullptr, 0, &returned, nullptr);
            m_running = false;
        }
    }

    AcqExpected<bool> read(std::span<std::int16_t> block) override
    {
        LONG state = 0;
        DWORD returned = 0;
        if (!::DeviceIoControl(m_device, IOCTL_BA_ERROR_STATE, nullptr, 0, &state, sizeof(state), &returned, nullptr)) {
            return ioError("error state query");
        }
        if (state != 0) {
            return AcqError(AcqErrorCode::DeviceIo, std::format("BrainAmp reports acquisition error {}", state));
        }
        const auto bytes = static_cast<DWORD>(block.size_bytes());
        if (!::ReadFile(m_device, block.data(), bytes, &returned, nullptr)) {
            return ioError("read");
        }
        return returned == bytes;
    }

private:
    HANDLE m_device;
    bool m_usb;
    DWORD m_version;
    bool m_running = false;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqExpected<std::unique_ptr<BrainAmpTransport>> mne::detail::openBrainAmpDriver()
{
    constexpr DWORD kFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH;
    bool usb = true;
    HANDLE device = ::CreateFileA(DEVICE_USB, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, kFlags, nullptr);
    if (device == INVALID_HANDLE_VALUE) {
        usb = false;
        device = ::CreateFileA(DEVICE_PCI, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, kFlags, nullptr);
    }
    if (device == INVALID_HANDLE_VALUE) {
        return AcqError(AcqErrorCode::DeviceNotFound, "no BrainAmp USB adapter and no ISA/PCI adapter found");
    }
    DWORD version = 0;
    DWORD returned = 0;
    ::DeviceIoControl(device, IOCTL_BA_DRIVERVERSION, nullptr, 0, &version, sizeof(version), &returned, nullptr);
    return std::unique_ptr<BrainAmpTransport>(std::make_unique<BrainAmpDriver>(device, usb, version));
}
